#ifndef CONTROLLERFORM_HPP
#define CONTROLLERFORM_HPP

#include <QWidget>
#include <QVector>
#include "cloudmodel.hpp"
#include "clustersettings.hpp"

#include "l2xyhistogramcollector.hpp"
#include "../mainwindow.hpp"

namespace Ui {
class ControllerForm;
}

class ControllerForm : public QWidget
{
  Q_OBJECT

public:
  explicit ControllerForm(QWidget *parent = 0);
  ~ControllerForm();

  CloudModel* getExperimentalCloud() const {
    return this->edited_model_;
  }

  size_t fillExperimentalSeries(QXYSeries& dest, qreal& progPct, qreal *min=0, qreal *max=0) const;
  size_t fillBaselineSeries(QXYSeries& dest, qreal& progPct, qreal *min=0, qreal *max=0) const;
  size_t fillDiffSeries(QXYSeries& dest, qreal *min=0, qreal *max=0) const;

signals:
  void hasSeriesUpdates(const ControllerForm* thizz);

protected:

  void editedClusterCreated(PointCluster* cluster);

  void editedClusterRemoved(const PointCluster* cluster);

private:

  void initClouds();

  void initiateClusterCreation() {
    if(this->edited_model_) {
      // TODO default cluster params
      PointCluster::normal_dist_data normalData(0.3, 2.0);
      this->edited_model_->createCluster(1000, &normalData);
    }
  }

  void updateSampledDistUi();

  Ui::ControllerForm *ui;

  CloudModel  *edited_model_, *baseline_model_;

  // TODO abstract it further and wrap it as a generic class to allow
  // distance kernel change
  L2XYHistogramCollector* histogram_collector_;

  QVector<ClusterSettings*> cluster_editors_;

 };

#endif // CONTROLLERFORM_HPP
