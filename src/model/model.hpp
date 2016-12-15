/* 
 * File:   model.hpp
 * Author: acolomitchi
 *
 * Created on 25 November 2016, 1:57 PM
 */

#ifndef MODEL_HPP
#define MODEL_HPP

#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>
#include <stdexcept>

#include <mutex>

#include <Eigen/Dense>

namespace Eigen {

// binary operations mixing float and double will have the results in double

template<typename BinaryOp>
struct ScalarBinaryOpTraits<float,double,BinaryOp> { typedef double ReturnType;  };
template<typename BinaryOp>
struct ScalarBinaryOpTraits<double,float,BinaryOp> { typedef double ReturnType;  };

} // namespace Eigen

namespace distspctr {

template <typename C, size_t DIM=2>
using npoint = Eigen::template Matrix<C, 1, DIM, Eigen::RowMajor>;

template <typename C, size_t DIM=2>
struct ident_transform {
  npoint<C,DIM> map(const npoint<C,DIM>& p) const {
    return p;
  }
};



// trailing typename... added for future extensions (e.g. an Observer pattern specialization)
template <typename C, size_t DIM=2, typename ...>
class npoint_grp;

template <typename C, size_t DIM> 
class npoint_grp<C, DIM> {
  std::vector<npoint<C, DIM>> points_;
public:
  using point_type=npoint<C,DIM>;

  npoint_grp() = default;
  virtual ~npoint_grp() = default;

  virtual bool add(const point_type& p) {
    this->points_.push_back(p);
    return true;
  }
  bool erase( size_t pos )  {
    bool ret=(pos<this->points_.size());
    if(ret) {
      auto it=this->points_.begin()+pos;
      this->points_.erase(it);
    }
    return ret;
  }
  
  bool erase( size_t first, size_t last )  {
    bool ret=
         (first<this->points_.size())
      && (last<this->points_.size())
      && (first<last)
    ;
    if(ret) {
      this->points_.erase(
        this->points_.begin()+first,
        this->points_.begin()+last
      );
    }
    return ret;
  }
  
  void clear() {
    this->points_.clear();
  }
  
  bool empty() const {
    return this->points_.empty();
  }
  
  size_t size() const {
    return this->points_.size();
  }
  
  const point_type& operator[](size_t i) const {
    return this->points_.at(i);
  }

  const point_type& at(size_t i) const {
    return this->points_.at(i);
  }

  virtual point_type operator()(size_t i) const {
    return this->points_.at(i);
  }

};

template <
  typename C, size_t DIM,
  class Transform
>
class npoint_grp<C, DIM, Transform> : public npoint_grp<C, DIM> {
public:
  npoint_grp() : npoint_grp<C,DIM>(), trn_(nullptr) { }
  npoint_grp(const Transform* trn) : npoint_grp<C,DIM>(), trn_(trn) { }

  virtual ~npoint_grp() = default;

  virtual npoint<C,DIM> operator()(size_t i) const {
    npoint<C,DIM> ret=(*this)[i];
    if( this->trn_ ) {
      ret=this->trn_->map(ret);
    }
    return ret;
  }

  const Transform* transform() const { return this->trn_; }

  const Transform* transform(const Transform* o) {
    auto ret=this->trn_;
    this->trn_=o;
    return ret;
  }

private:
  const Transform* trn_;
};

// The supplier class is expected to provide:
// - a `size_t size() const` method
// - a `npoint<C,DIM> operator(size_t i) const`, returning a copy
//   of the point stored at position `i`
template <
  class Supplier,
  typename C, size_t DIM=2,
  typename index_type=uint32_t
>
class bound_npoint_cloud {
private:
  struct suppl_entry {
    const Supplier* supplier_;
    mutable std::vector<index_type> indices_; // within bound indices
    mutable bool inited_;

    suppl_entry(const Supplier* supplier) :
      supplier_(supplier), indices_(), inited_(false)
    {
      if(supplier->size()>std::numeric_limits<index_type>::max()) {
        throw std::invalid_argument("Too many points, can't index it");
      }
    }
  };

public:
  bound_npoint_cloud() = default;

  virtual ~bound_npoint_cloud() = default;

  void add(const Supplier* supplier) {
    if(supplier) {
      std::unique_lock<std::mutex> barrier(this->entries_lock_);
      if(this->index_of(supplier)>=this->suppliers_.size()) {
        // not already in
        this->suppliers_.push_back(supplier);
      }
    }
  }

  void remove(const Supplier* supplier) {
    if(supplier) {
      std::unique_lock<std::mutex> barrier(this->entries_lock_);
      size_t ix=this->index_of(supplier);
      if(ix<this->suppliers_.size()) {
        this->suppliers_.erase(this->suppliers_.begin()+ix);
      }
    }
  }

  virtual bool within_bounds(const npoint<C,DIM>& p) const=0;

  size_t size() const {
    size_t ret=0;
    std::unique_lock<std::mutex> barrier(this->entries_lock_);
    for(auto e : this->suppliers_) {
      for(size_t i=0; i<e->size(); i++) {
        auto p=e->operator()(i);
        if(this->within_bounds(p)) {
          ret++;
        }
      }
    }
    return ret;
  }


  // Container must provide `push_back(const npoint<C,DIM>&)` method
  template <typename Container> void points_copy(Container& dest) const {
    std::unique_lock<std::mutex> barrier(this->entries_lock_);
    for(auto const e : this->suppliers_) {
      this->copy_supplier_points(e, dest);
    }
  }

  // Container must provide `push_back(const npoint<C,DIM>&)` method
  template <class Container>
  void supplier_points(const Supplier* supplier, Container& dest) const
  {
    if(supplier) {
      std::unique_lock<std::mutex> barrier(this->entries_lock_);
      size_t ix=this->index_of(supplier);
      auto const & e=this->suppliers_[ix];
      this->copy_supplier_points(e, dest);
    }
  }

  size_t supplier_count() const {
    return this->suppliers_.size();
  }

  const Supplier* supplier(size_t i) const {
    return this->suppliers_.at(i);
  }


protected:

  template <class Container>
  void copy_supplier_points(const Supplier* e, Container& dest) const {
    if(e) {
      for(size_t i=0; i<e->size(); i++) {
        auto p=e->operator()(i);
        if(this->within_bounds(p)) {
          dest.push_back(p);
        }
      }
    }
  }


  size_t index_of(const Supplier* supplier) const {
    for(size_t i=0; i<this->suppliers_.size(); i++) {
      const suppl_entry& e=this->suppliers_[i];
      if(e.supplier_==supplier) {
        return i;
      }
    }
    return size_t(-1);
  }


private:
  std::vector<const Supplier*> suppliers_;
  mutable std::mutex entries_lock_;
};

template <
  class Supplier,
  typename C, size_t DIM=2,
  typename index_type=uint32_t
>
class bbox_npoint_cloud : public bound_npoint_cloud<Supplier, C, DIM, index_type> {
public:
  bbox_npoint_cloud(
    const npoint<C,DIM>& boxMins,
    const npoint<C,DIM>& boxMaxes
  ) : bbox_min_(), bbox_max_()
  {
    for(size_t i=0; i<DIM; i++) {
      C min=boxMins(i), max=boxMaxes(i);
      if(min>max) {
        std::swap(min, max);
      }
      this->bbox_min_(i)=min;
      this->bbox_max_(i)=max;
    }
  }

  virtual ~bbox_npoint_cloud() = default;

  virtual bool within_bounds(const npoint<C,DIM>& p) const {
    bool ret=true;
    for(size_t i=0; ret && i<DIM; i++) {
      C v=p(i);
      C min=this->bbox_min_(i), max=this->bbox_max_(i);
      ret = (min<=v && v<=max);
    }
    return ret;
  }

  const npoint<C,DIM>& bbox_min() const {
    return this->bbox_min_;
  }

  const npoint<C,DIM>& bbox_max() const {
    return this->bbox_max_;
  }

  C diag_len() const {
    return (this->bbox_max_-this->bbox_min_).norm();
  }

private:
  npoint<C,DIM> bbox_min_;
  npoint<C,DIM> bbox_max_;
};

template <typename C>
class histogram {
protected:
  mutable std::vector<size_t> buckets_;

  histogram(const histogram& other) = default;

  histogram& operator=(const histogram<C>& other) = default;
public:
  histogram(size_t numBuckets) : buckets_(numBuckets, 0) {
    assert(numBuckets>0);
  }
  virtual ~histogram() { }
  
  size_t num_slots() const {
    return this->buckets_.size();
  }
  
  virtual bool add_sample(const C& val)=0;
  
  virtual C min_sample_value() const =0;
  
  virtual C max_sample_value() const =0;
  
  virtual C slot_center(size_t slotIx) const {
    long double min=static_cast<long double>(this->min_sample_value());
    long double max=static_cast<long double>(this->max_sample_value());
    long double delta=(max-min)/this->num_slots();
    return static_cast<C>(delta*(0.5+slotIx));
  }

  virtual C slot_min(size_t slotIx) const {
    long double min=static_cast<long double>(this->min_sample_value());
    long double max=static_cast<long double>(this->max_sample_value());
    long double delta=(max-min)/this->num_slots();
    return static_cast<C>(delta*slotIx);
  }

  virtual C slot_max(size_t slotIx) const {
    long double min=static_cast<long double>(this->min_sample_value());
    long double max=static_cast<long double>(this->max_sample_value());
    long double delta=(max-min)/this->num_slots();
    return static_cast<C>(delta*(slotIx+1));
  }

  size_t slot_count(size_t slotIx) const {
    return this->buckets_.at(slotIx);
  }
  
  virtual size_t total_count() const {
    size_t ret=0;
    for(size_t c : this->buckets_) {
      ret+=c;
    }
    return ret;
  }
  
  virtual void clear() {
    std::fill(this->buckets_.begin(), this->buckets_.end(), 0);
  }
};

template <typename C>
class fixedl_histogram : public histogram<C> {
  C min_;
  C max_;
  size_t total_samples_;
  std::vector<C> thresholds_; // start value of the buckets in creasing order
public:
  fixedl_histogram(size_t slotCount, C min, C max) :
    histogram<C>(slotCount),
    min_(min), max_(max), total_samples_(0), thresholds_(slotCount)
  {
    if(min_>max_) {
      std::swap(min_, max_);
    }
    for(size_t i=0; i<slotCount; i++) {
      long double x=static_cast<long double>(i)/slotCount;
      thresholds_[i]=static_cast<C>((1-x)*min_+x*max_);
    }
  }

  fixedl_histogram(const fixedl_histogram& other) =default;

  fixedl_histogram& operator=(const fixedl_histogram<C>& other) = default;
    
  virtual ~fixedl_histogram() { }
  
  virtual bool add_sample(const C& val) {
    bool ret=(val>=this->min_ && val<=this->max_);
    if(ret) {
      // binary search the index of the bucket this sample should be counted against
      size_t thrLen=this->thresholds_.size();
      size_t b=0, e=thrLen-1;
      while(b<=e && e<thrLen) { // e may underflow in e=mid-1
        size_t mid=(b+e)/2;
        if(val>this->thresholds_[mid]) {
          b=mid+1;
        }
        else {
          e=mid-1;
        }
      }
       // count everything above the max against last bucket
      b=(b>=thrLen-1) ? thrLen-1 : b;
      this->buckets_[b]++;
      this->total_samples_++;
    }
    return ret;
  }
  
  virtual C min_sample_value() const {
    return this->min_;
  }
  
  virtual C max_sample_value() const {
    return this->max_;
  }
  
  virtual size_t total_count() const {
    return this->total_samples_;
  }
  virtual void clear() {
    histogram<C>::clear();
    this->total_samples_=0;
  }
};

} // namespace distspctr

#endif /* MODEL_HPP */

