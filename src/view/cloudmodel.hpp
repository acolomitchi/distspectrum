#ifndef CLOUDMODEL_HPP
#define CLOUDMODEL_HPP

#include <QObject>

#include "2d.hpp"
#include "pointcluster.hpp"
#include "../model/model.hpp"

// FWD declaration
class PointCloudView;

class CloudModel : public QObject
{
  Q_OBJECT
public:
  CloudModel(QObject* parent=0);

  PointCluster* createCluster(
    size_t population=800,
    const PointCluster::normal_dist_data* normalData=nullptr
  );

  void removeCluster(PointCluster* cluster);

  PointCluster* getSelection() const { return this->selection_; }

  const QVector<const PointCluster*>& clusters() const {
    return this->clusters_;
  }

  void getClusterPoints(const PointCluster* cluster, QVector<p2d>& dest) {
    this->cloud_.supplier_points(cluster, dest);
  }

  const distspctr::bbox_npoint_cloud<PointCluster, coord_type, 2>& cloud_source() const {
    return this->cloud_;
  }

public slots:
  void setSelection(PointCluster* cluster);

  void clusterPointsUpdated(PointCluster* cluster);

signals:
  void selectionChanged(const PointCluster* newSelection);

  void clusterAdded(PointCluster*);

  void clusterRemoved(const PointCluster*);

  void pointsPrechange(CloudModel* thizz_);

  void pointsChanged(CloudModel* thizz_);

private:
  QVector<const PointCluster*> clusters_;
  distspctr::bbox_npoint_cloud<PointCluster, coord_type, 2> cloud_;

  PointCluster* selection_;
};

#endif // CLOUDMODEL_HPP
