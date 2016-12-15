#include <QRunnable>
#include <QThreadPool>
#include "l2xyhistogramcollector.hpp"

#define UPDATE_PCT 0.05

L2XYHistogramCollector::L2XYHistogramCollector(
  const CloudModel& experimental, const CloudModel& baseline,
  QObject* parent, size_t histogramSlots, size_t maxDistCount
)
  : QObject(parent),
    DiffHistogramCollector<QXYSeries,  l2dist>(
      baseline.cloud_source(), experimental.cloud_source(),
      histogramSlots
    ),
    dist_(), max_dists_samples_(maxDistCount)
{
  auto pstPrechange=&CloudModel::pointsPrechange;
  QObject::connect(
      &baseline, pstPrechange,
      [&](CloudModel*) {
         this->stopBaselineUpdate();
      }
  );
  QObject::connect(
      &baseline, pstPrechange,
      [&](CloudModel*) {
         this->stopExperimentalUpdate();
      }
  );

  auto ptsChange=&CloudModel::pointsChanged;
  QObject::connect(
      &baseline, ptsChange,
      [&](CloudModel*) {
         this->triggerBaselineUpdate(this->dist_, UPDATE_PCT, this->max_dists_samples_);
      }
  );
  QObject::connect(
      &experimental, ptsChange,
      [&](CloudModel*) {
         this->triggerExperimentalUpdate(this->dist_, UPDATE_PCT, this->max_dists_samples_);
      }
  );
}

void L2XYHistogramCollector::setMaxDistSamples(size_t maxDistSamples) {
  // FXIME delete write
  std::cout << "new maxDistSample: " << maxDistSamples
            << " " << " old:" << max_dists_samples_
            << std::endl
  ;
  if(maxDistSamples!=this->max_dists_samples_) {
    this->max_dists_samples_=maxDistSamples;
    this->triggerBaselineUpdate(this->dist_, UPDATE_PCT, this->max_dists_samples_);
    this->triggerExperimentalUpdate(this->dist_, UPDATE_PCT, this->max_dists_samples_);
  }
}


void L2XYHistogramCollector::processUpdate(
  std::shared_ptr<distspctr::histogram<coord_type>> hist, std::vector<QPointF>& dest
) {
  DiffHistogramCollector<QXYSeries, l2dist>::processUpdate(hist, dest);
  // the filling of the Q...Series need to happen on the GI tread
  // Here we are on a different thread (an std:: one), so send in the notification
  // async to force a QueuedConnection style.
  class Notifier : public QRunnable {
  public:
    Notifier(L2XYHistogramCollector* sender) : QRunnable(), sender_(sender) { }

    virtual void run() {
      emit this->sender_->updated(this->sender_);
    }
  private:
    L2XYHistogramCollector* sender_;
  };
  Notifier* notifier=new Notifier(this);
  QThreadPool::globalInstance()->start(notifier, 1);
}
