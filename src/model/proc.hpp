
/* 
 * File:   logic.hpp
 * Author: acolomitchi
 *
 * Created on 27 November 2016, 12:07 PM
 */

#ifndef PROC_HPP
#define PROC_HPP

#include <limits>
#include <numeric>
#include <random>
#include <type_traits>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>

#include <typeinfo>

#include "model.hpp"

namespace distspctr {

namespace detail {

// SFINAE check for a method named `update`
template <typename, typename T>
struct has_update {
  static_assert(
    std::integral_constant<T, false>::value,
    "Second template parameter needs to be a function type."
  );
};

template <typename X, typename Ret, typename... Args>
struct has_update<X, Ret(Args...)> {
private:
  template <typename T>
  static constexpr auto check(T*) ->
    typename std::is_same<
      decltype(std::declval<T>().update(std::declval<Args>()...)),
      Ret
    >::type;
  template<typename>
  static constexpr std::false_type check(...);
  
  using response=decltype(check<X>(static_cast<X*>(nullptr)));
public:
  static constexpr bool value=response::value;
};

template <typename dist_class, typename Supplier>
struct is_observer_dist {
public:
  static constexpr bool value =
     has_update<dist_class, void(const Supplier*)>::value;
};

template <class dist_type, class Supplier, bool>
struct dist_type_updater_base;

template <class dist_type, class Supplier>
struct dist_type_updater_base<dist_type, Supplier, true> {
  static void update_dist(const Supplier* src, dist_type& dist_calculator) {
    dist_calculator.update(src);
  }

  static void update_dist(const Supplier& src, dist_type& dist_calculator) {
    dist_calculator.update(&src);
  }

};

template <class dist_type, class Supplier>
struct dist_type_updater_base<dist_type, Supplier, false> {
  static void update_dist(const Supplier* , dist_type&) {
    // do nothing
  }

  static void update_dist(const Supplier& , dist_type&) {
    // do nothing
  }

};


template <class dist_type, class Supplier>
struct dist_type_updater
    : public dist_type_updater_base<
        dist_type, Supplier,
        is_observer_dist<dist_type, Supplier>::value
      >
{
};


template <typename T> struct is_dereferencable_base
{
  using type=void;
  static constexpr bool value=false;
};

template <typename T> struct is_dereferencable_base<T*> : public std::true_type
{
  using type=T;
  static constexpr bool value=true;
};

template <typename T>
struct is_dereferencable_base<std::shared_ptr<T>> : public std::true_type
{
  using type=T;
  static constexpr bool value=true;
};

template <typename T>
struct is_dereferencable_base<std::weak_ptr<T>> : public std::true_type
{
  using type=T;
  static constexpr bool value=true;
};

template <typename T>
struct dereferencable :
    public is_dereferencable_base<
      typename std::remove_reference<
        typename std::remove_cv<T>::type
      >
    >
{
private:
  using nocv=typename std::remove_cv<T>::type;
  using noref=typename std::remove_reference<nocv>::type;
public:
  using type=typename is_dereferencable_base<noref>::type;
  static constexpr bool value=is_dereferencable_base<noref>::value;

  static type* lock(type* x) { return x; };

  static std::shared_ptr<type> lock(std::shared_ptr<type>& x) { return x; }
  static std::shared_ptr<type> lock(const std::shared_ptr<type>& x) { return x; };

  static std::shared_ptr<type> lock(std::weak_ptr<type>& x) { return x.lock(); }
  static std::shared_ptr<type> lock(const std::weak_ptr<type>& x) { return x.lock(); }

};

template <typename C, size_t DIM>
struct vec_adaptor : public std::vector<npoint<C, DIM>> {
public:
  using points_vec = std::vector<npoint<C, DIM>>;

  vec_adaptor() = default;
  vec_adaptor(const vec_adaptor& )=default;
  vec_adaptor(vec_adaptor&& o) : points_vec(o) { }

  vec_adaptor& operator=(const vec_adaptor& ) = default;
  vec_adaptor& operator=(vec_adaptor&& o) {
    return points_vec::operator =(o);
  }

  const npoint<C, DIM>& operator()(size_t i) const {
    return points_vec::at(i);
  }
};


} // namespace detail

// src - operator()(size_t i) to get the point at position i and size()
//       with the result type assignable to a npoint<C,DIM>
// DistCalc - dist_type operator()(const npoint<C,DIM>&, const npoint<C,DIM>) const
// dest - a return of `false` signals "stop computations, I'll not listen anymore"
template <
  typename C, size_t DIM,
  class PointSupplier, class DistCalc,
  typename dist_type=double
> void compute_distances(
  const PointSupplier& src,
  std::function<bool(dist_type)> dest, DistCalc& calc,
  size_t max_dist_count=std::numeric_limits<size_t>::max()
)
// may throw() whatever the Supplier or dest throws.
{
  size_t plen=src.size();
  if(plen>1) {
    detail::dist_type_updater<DistCalc, PointSupplier>::update_dist(src, calc);
    size_t maxDistCount=(plen*(plen-1)) >> 1;
    if(maxDistCount>max_dist_count) {
      // sampled
      std::mt19937 rng;
      rng.seed(std::chrono::high_resolution_clock::now().time_since_epoch().count());
      std::uniform_int_distribution<size_t> distrib(0, plen-1);
      for(size_t count=0; count < max_dist_count; count++) {
        size_t i=distrib(rng), j=distrib(rng);
        while(i==j) j=distrib(rng);
        npoint<C,DIM> first=src(i), second=src(j);
        if( !dest(calc(first,second)) ) {
          break;
        }
      }
    }
    else { // exhaustive
      for(size_t i=0; i<plen; i++) {
        npoint<C,DIM> first=src(i);
        for(size_t j=i+1; j<plen; j++) {
          npoint<C,DIM> second=src(j);
          if( !dest(calc(first,second)) ) {
            return;
          }
        }
      }
    }
  }
}

// PointSupplier - size() and `void points_copy(Container& dest)`
//                 with a Container class providing the `push_back(const npoint<CoordType, DIM>& point).
// DistCalculator - dist_type operator()(const npoint<C,DIM>&, const npoint<C,DIM>) const
// Observer - void partial_progress(shared_ptr<histogram<CoordType>>, size_t progress, size_t total)
//             invoked at every observerProgressTick
//          - void done(shared_ptr<histogram<CoordType>>) - invoked when done
template <
  typename CoordType, size_t DIM,
  class PointSupplier, class Observer
>
class histogram_filler {
public:
  // if/when the observer get recycled, the thread stops at the next observerProgressTick
  // unless stopped earlier by calling stop

  // creates a std::weak_prt<Observer> from the provided shared_ptr one
  histogram_filler(
    const std::shared_ptr<histogram<CoordType>>& toFill
  ) :
    histogram_(toFill),
    eager_stop_flag_(false), done_(false), exec_()
  {
    assert(toFill);
  }

  ~histogram_filler() {
    this->stop();
  }

  template <class DistCalculator, class ObserverPtr>
  void start(const PointSupplier& src, const DistCalculator& dist,
             ObserverPtr observer,
             double observerProgressTickPct,
             size_t maxDists=std::numeric_limits<size_t>::max()
  ) {
    while(!this->done_ && this->exec_.joinable()) {
      // if it's not joinable, it may be just inited .
      this->stop();
    }
    this->histogram_->clear();
    this->eager_stop_flag_.store(false);
    this->done_=false;
    this->startThreadProper(src, dist, observer, observerProgressTickPct, maxDists);
  }

  void stop() {
    // signal stop
    this->eager_stop_flag_.store(true);
    // wait for the thread to stop - should happen at the next cycle
    if(this->exec_.joinable()) {
      this->exec_.join();
    }
  }

  bool stopped() const {
    return this->eager_stop_flag_.load();
  }

  std::shared_ptr<histogram<CoordType>> get_histogram() const {
    return this->histogram_;
  }

private:

  template <class DistCalculator, class ObserverPtr> void startThreadProper(
    const PointSupplier& src,
    DistCalculator& distCalc,
    ObserverPtr observer,
    double observerProgressTickPct,
    size_t maxDists
  )
  {
    static_assert(
      detail::dereferencable<ObserverPtr>::value &&
      std::is_same<Observer, typename detail::dereferencable<ObserverPtr>::type>::value,
      "The observer param must be dereferencable to the `Observer` type"
    );
    detail::vec_adaptor<CoordType, DIM> points;
    src.points_copy(points);
    size_t len=points.size();
    if(len) {
      if(maxDists>len*(len-1)/2) {
        // more distances than what can be obtained from the available points
        maxDists=len*(len-1)/2;
      }
      size_t observerProgressTick=
          observerProgressTickPct>0
        ? maxDists*observerProgressTickPct
        : std::numeric_limits<size_t>::max()
      ;
      size_t progressSoFar=0;
      size_t nextNotification=observerProgressTick;
      auto collector=[
        progressSoFar, nextNotification, maxDists, observerProgressTick,
        observer, this
      ] (CoordType reported) mutable {
        bool ret=(maxDists>progressSoFar);
        if(!ret) {
          return false;
        }
        progressSoFar++;
        this->histogram_->add_sample(reported);
        nextNotification--;
        bool expectedStopFlag=false;
        if(maxDists<=progressSoFar) {
          // last notification
          auto notification=detail::dereferencable<ObserverPtr>::lock(observer);
          // to update the Observer, we need:
          // - the observer must still be valid (if provided as weak_ptr for example)
          // - the stop flag was not raised
          // The compare_exchange_strong will be false if the stop_flag is not the expected false
          //  (comparison doesn't succeed) - as we are using the strong version, no spurrios falses.
          // If compare_exchange_strong is false (expectedStopFlag is true) which means
          //   the stop flag was raised before
          // If hasChange is true and expectedStopFlag is also true, it means
          //   even if the stop flag was not raised before, the Observer is no longer valid
          // One on top of the other, we can proceed with notifying only if
          //   the expectedStopFlag is still false
          expectedStopFlag=false;
          this->eager_stop_flag_.compare_exchange_strong(expectedStopFlag, !notification);
          if(!expectedStopFlag) {
            notification->done(this->histogram_);
          }
          this->done_=true;
          return false; // no other distances expected or will be accepted
        }
        else if(! nextNotification ) { // must notify the observer on partial progress
          expectedStopFlag=false;
          auto notification=detail::dereferencable<ObserverPtr>::lock(observer);
          this->eager_stop_flag_.compare_exchange_strong(expectedStopFlag, !notification);
          if(!expectedStopFlag) {
            notification->partial_progress(this->histogram_, progressSoFar, maxDists);
          }
          nextNotification=observerProgressTick;
        }
        // check again the flag
        expectedStopFlag=false;
        this->eager_stop_flag_.compare_exchange_strong(expectedStopFlag, false);
        ret= !expectedStopFlag;
        if(expectedStopFlag) {  // prematurely stopped
          this->done_=true;
        }
        return ret;
      };
      std::function<bool(CoordType)> coll(collector);
      auto threadFunc= [=]() mutable {
        try {
          compute_distances<
            CoordType, DIM,
            detail::vec_adaptor<CoordType, DIM>, DistCalculator
          >(points, coll, distCalc, maxDists);
        }
        catch(...) {
          this->eager_stop_flag_.store(true);
          this->done_=true;
        }
      };
      this->exec_=std::thread(threadFunc);
    }
    else { // no points in the supplier: job done before starting it
      // but we still nee to create another thread for reporting
      // the thread-start and result reportng are protected by a
      // unique lock (non-reentrant)
      auto reporter=[this, observer]() {
        auto notification=detail::dereferencable<ObserverPtr>::lock(observer);
        bool expectedStopFlag=false;
        this->eager_stop_flag_.compare_exchange_strong(expectedStopFlag, !notification);
        if(!expectedStopFlag) {
          notification->done(this->histogram_);
        }
        this->done_=true;
      };
      this->exec_=std::thread(reporter);
    }
  }

  std::shared_ptr<histogram<CoordType>> histogram_;
  std::atomic<bool> eager_stop_flag_;
  bool done_;
  std::thread exec_;
};


} // namespace distspctr

#endif /* PROC_HPP */

