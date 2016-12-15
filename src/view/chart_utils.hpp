#ifndef CHART_UTILS_HPP
#define CHART_UTILS_HPP

#include <mutex>
#include <vector>

#include <QObject>

#include "2d.hpp"
#include "../model/proc.hpp"


template <typename ChartSeriesType, class DistType>
class DiffHistogramCollector
{
public:
  using point_cloud=distspctr::bbox_npoint_cloud<PointCluster, coord_type, 2>;
private:
  using histogram_type=distspctr::fixedl_histogram<coord_type>;
  using filler_type=
    distspctr::histogram_filler<
      coord_type, 2, point_cloud,
      DiffHistogramCollector<ChartSeriesType, DistType>
    >
  ;
protected:

  DiffHistogramCollector(
    const point_cloud& baseline, const point_cloud& experimental,
    size_t histogramSlots=100
  ) :
    baseline_(baseline), experimental_(experimental), histo_slots_(histogramSlots),
    baseline_data_(), experimental_data_(), diff_(), lock_(),
    baseline_filler_(), experimental_filler_()
  {
    assert(histogramSlots>0);
    const p2d &blineMin=baseline.bbox_min(), &blineMax=baseline.bbox_max();
    assert(blineMin.isApprox(experimental.bbox_min(), 1e-5));
    assert(blineMax.isApprox(experimental.bbox_max(), 1e-5));

    coord_type diagLen=this->baseline_.diag_len();
    std::shared_ptr<histogram_type> dummy=
        std::make_shared<histogram_type>(this->histo_slots_, 0, diagLen)
    ;
    this->computeData(dummy, this->baseline_data_);
    this->computeData(dummy, this->experimental_data_);
    this->computeData(dummy, this->diff_);
  }

  void stopBaselineUpdate() {
    if(this->baseline_filler_) {
      this->baseline_filler_->stop();
      this->baseline_filler_.reset();
    }
  }

  void stopExperimentalUpdate() {
    if(this->experimental_filler_) {
      this->experimental_filler_->stop();
      this->experimental_filler_.reset();
    }
  }

  void triggerBaselineUpdate(
    DistType& distance, double progressTickPercent,
    size_t maxDistanceCount=std::numeric_limits<size_t>::max()
  ) {
    if(this->baseline_filler_) {
      this->baseline_filler_->stop();
    }
    // until the new thread is properly started, refuse
    // to handle another request for change
    std::unique_lock<std::mutex> barrier(this->lock_);
    std::shared_ptr<histogram_type> histogram=
        std::make_shared<histogram_type>(
          this->histo_slots_, 0, this->baseline_.diag_len()
        )
    ;
    this->baseline_filler_=std::make_shared<filler_type>(histogram);
    this->baseline_filler_->start(
        this->baseline_, distance, this,
        progressTickPercent, maxDistanceCount
    );
  }

  void triggerExperimentalUpdate(
    DistType& distance, double progressTickPercent,
    size_t maxDistanceCount=std::numeric_limits<size_t>::max()
  ) {
    if(this->experimental_filler_) {
      this->experimental_filler_->stop();
    }
    // until the new thread is properly started, refuse
    // to handle another request for change
    std::unique_lock<std::mutex> barrier(this->lock_);
    std::shared_ptr<histogram_type> histogram=
        std::make_shared<histogram_type>(
          this->histo_slots_, 0, this->experimental_.diag_len()
        )
    ;
    this->experimental_filler_=std::make_shared<filler_type>(histogram);
    this->experimental_filler_->start(
          this->experimental_, distance, this,
          progressTickPercent, maxDistanceCount
    );
  }

  virtual void processUpdate(std::shared_ptr<distspctr::histogram<coord_type>> hist, std::vector<QPointF>& dest) {
    std::vector<QPointF> res;
    this->computeData(hist, res);
    dest.resize(res.size());
    this->diff_.resize(res.size());
    for(size_t i=0; i<this->histo_slots_; i++) {
      dest[i]=res[i];
      const QPointF& baseP=this->baseline_data_.at(i);
      this->diff_[i]={
        baseP.x(),
        this->experimental_data_[i].y()-baseP.y()
      };
    }
  }

public:

  virtual ~DiffHistogramCollector() {}

  size_t experimentalSeries(ChartSeriesType& dest, qreal& progPct, qreal* min=0, qreal* max=0) const {
    progPct=this->experimental_progress_;
    return this->toSeries(this->experimental_data_, dest, min, max);
  }

  size_t baselineSeries(ChartSeriesType& dest, qreal& progPct, qreal* min=0, qreal* max=0) const {
    progPct=this->baseline_progress_;
    return this->toSeries(this->baseline_data_, dest, min, max);
  }

  size_t diffSeries(ChartSeriesType& dest, qreal* min=0, qreal* max=0) const {
    return this->toSeries(this->diff_, dest, min, max);
  }

  // those two would benefit from a wrapper class
  void partial_progress(
    std::shared_ptr<distspctr::histogram<coord_type>> hist,
    size_t progress, size_t total_dists
  ) {
    std::unique_lock<std::mutex> barrier(this->lock_);
    if(
         this->baseline_filler_
      && hist==this->baseline_filler_->get_histogram()
    ) {
      this->baseline_progress_=progress/double(total_dists);
      this->processUpdate(hist, this->baseline_data_);
    }
    if(
         this->experimental_filler_
      && hist==this->experimental_filler_->get_histogram()
    ){
      this->experimental_progress_=progress/double(total_dists);
      this->processUpdate(hist, this->experimental_data_);
    }
  }

  void done(std::shared_ptr<distspctr::histogram<coord_type>> hist) {
    std::unique_lock<std::mutex> barrier(this->lock_);
    if(
         this->baseline_filler_
      && hist==this->baseline_filler_->get_histogram()
    ) {
      this->baseline_progress_=1.0;
      this->processUpdate(hist, this->baseline_data_);
    }
    if(
         this->experimental_filler_
      && hist==this->experimental_filler_->get_histogram()
    ) {
      this->experimental_progress_=1.0;
      this->processUpdate(hist, this->experimental_data_);
    }
  }


private:

  size_t toSeries(
    const std::vector<QPointF>& src, ChartSeriesType& dest,
    qreal* min=0, qreal* max=0
  ) const {
    std::unique_lock<std::mutex> barrier(this->lock_);
    if(min) *min=std::numeric_limits<qreal>::max();
    if(max) *max=std::numeric_limits<qreal>::min();
    size_t len=src.size();
    if(len) {
      if(len>1) {
        dest.blockSignals(true);
        dest.clear();
        for(size_t i=0; i<len-1; i++) {
          const QPointF& p=src[i];
          dest << p;
          if(min) *min=std::min(*min, p.y());
          if(max) *max=std::max(*max, p.y());
        }
        dest.blockSignals(false);
      }
      dest << src[len-1];
      if(min) *min=std::min(*min, src[len-1].y());
      if(max) *max=std::max(*max, src[len-1].y());
    }
    else {
      dest.clear();
    }
    return len;
  }

  void computeData(std::shared_ptr<distspctr::histogram<coord_type>> hist, std::vector<QPointF>& dest) {
    size_t sampleCount=hist->total_count();
    for(size_t i=0; i<this->histo_slots_; i++) {
      dest.emplace_back(
        hist->slot_min(i),
        sampleCount ? hist->slot_count(i)/double(sampleCount) : 0
      );
    }
    dest.emplace_back(hist->slot_max(this->histo_slots_-1), 0.0f);
  }

  const point_cloud& baseline_;
  const point_cloud& experimental_;
  size_t histo_slots_;
  std::vector<QPointF> baseline_data_;
  double baseline_progress_;
  std::vector<QPointF> experimental_data_;
  double experimental_progress_;
  std::vector<QPointF> diff_;
  mutable std::mutex lock_;

  std::shared_ptr<filler_type> baseline_filler_;
  std::shared_ptr<filler_type> experimental_filler_;

};


#endif // CHART_UTILS_HPP
