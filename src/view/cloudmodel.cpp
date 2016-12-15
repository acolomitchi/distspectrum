#include "cloudmodel.hpp"

#include "pointcloudview.hpp"

CloudModel::CloudModel(QObject *parent) :
  QObject(parent), clusters_(),
  cloud_({0.0f, 0.0f}, {1.0f, 1.0f}),
  selection_(nullptr)
{

}

PointCluster *CloudModel::createCluster(
  size_t population, const PointCluster::normal_dist_data *normalData
) {
  PointCluster* ret=new PointCluster(this, population, normalData);
  emit this->pointsPrechange(this);
  this->clusters_.push_back(ret);
  this->cloud_.add(ret);
  emit this->clusterAdded(ret);
  emit this->pointsChanged(this);
  this->setSelection(ret);
  return ret;
}

void CloudModel::removeCluster(PointCluster *cluster) {
  int ix=cluster ? this->clusters_.indexOf((cluster)) : -1;
  if(ix>=0) {
    PointCluster* myCopy=const_cast<PointCluster*>(cluster);
    emit this->pointsPrechange(this);
    this->clusters_.removeAt(ix);
    this->cloud_.remove(cluster);
    if(cluster==this->selection_) {
      this->setSelection(nullptr);
    }
    emit this->clusterRemoved(cluster);
    emit this->pointsChanged(this);
    myCopy->deleteLater();
  }
}

void CloudModel::setSelection(PointCluster* cluster) {
  if(
    cluster!=this->selection_ &&
    (!cluster || this->clusters_.contains(cluster))
  ) {
    this->selection_=cluster;
    emit selectionChanged(this->selection_);
  }
}

void CloudModel::clusterPointsUpdated(PointCluster *cluster) {
  if( cluster && this->clusters_.contains(cluster)) {
    emit this->pointsPrechange(this);
    emit this->pointsChanged(this);
  }
}
