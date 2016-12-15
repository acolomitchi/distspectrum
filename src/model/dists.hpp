/* 
 * File:   dists.hpp
 * Author: acolomitchi
 *
 * Created on 26 November 2016, 10:00 AM
 */

#ifndef DISTS_HPP
#define DISTS_HPP

#include <chrono>
#include <random>

#include <Eigen/Dense>

#include "model.hpp"

namespace distspctr {

template <typename Coord, size_t DIM>
class l2 {
public:
  Coord operator()(const npoint<Coord, DIM> &p0, const npoint<Coord, DIM> &p1) const {
    npoint<Coord, DIM> diff=p0-p1;
    Coord ret=diff.norm();
    return ret;
  }
};

// Supplier must provide `operator(size_t i) const` with a result
//  assignable to `npoint<internal_type,DIM>`
template <
  typename Coord, size_t DIM, class Supplier,
  typename internal_type=double
>
class mahalanobis {
public:
  mahalanobis() : supplier_(nullptr), dirty_(false), covar_inv_(), means_()
  {
    this->covar_inv_.setIdentity();
    this->means_.setZero();
  }

  void update(const Supplier* src) {
    if( src ) {
      this->supplier_=src;
      this->dirty_=true;
    }
  }

  Coord operator()(const npoint<Coord, DIM> &p0, const npoint<Coord, DIM> &p1) const {
    if(this->dirty_) {
      this->computeMatrix();
    }
    point_type diff=p0-p1;
    Coord ret=std::sqrt((diff*this->covar_inv_)*diff.transpose());
    return ret;
  }

private:
  using point_type=npoint<internal_type, DIM>;
  using matrix_type=Eigen::Matrix<internal_type, DIM, DIM>;
  const Supplier* supplier_;
  mutable bool dirty_;
  mutable matrix_type covar_inv_;
  mutable point_type  means_;

  void computeMatrix() const {
    static std::mt19937 rng;
    static bool rngInited=false;

    if(this->supplier_ && this->supplier_->size()>1) {
      point_type sample0;
      size_t len=this->supplier_->size();

      means_.setZero();
      point_type sample;
      for(size_t i=0; i<len; i++) {
        sample=(*supplier_)(i);
        means_+=sample;
      }
      means_ /= static_cast<internal_type>(len);

      point_type variance;
      matrix_type covariance;
      covariance.setZero();

      for(size_t i=0; i<len; i++) {
        sample=(*supplier_)(i);
        variance=sample0-means_;
        for(size_t r=0; r<DIM; r++) {
          for(size_t c=r; c<DIM; c++) {
            covariance(r,c)+=variance(r)*variance(c);
          }
        }
      }
      covariance/=internal_type(len-1);
      //    for(size_t i=DIM-1; i<DIM; i--) {
      //      for(size_t j=0; j<i; j++) {
      //        covariance(i,j)=covariance(j, i);
      //      }
      //    }
      matrix_type identity;
      identity.setIdentity();
      this->covar_inv_=
        covariance.template selfadjointView<Eigen::Upper>()
                   .ldlt()
                   .solve(identity)
      ;
      // std::cout << this->covar_inv_ << std::endl;
    }
    else { // degrade to identity
      this->covar_inv_.setIdentity();
      if(1==this->supplier_->size()) {
        this->supplier_->copy(0, this->means_);
      }
      else {
        this->means_.setZero();
      }
    }
  }
};

} // namespace distspctr

#endif /* DISTS_HPP */

