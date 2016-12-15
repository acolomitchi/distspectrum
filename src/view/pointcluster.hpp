#ifndef POINTCLUSTER_HPP
#define POINTCLUSTER_HPP

#include <random>

#include <QObject>
#include <QColor>
#include <QPointF>
#include <QTransform>

#include "2d.hpp"

class CloudModel;

class PointCluster : public QObject // maybe needed at destruction
{

  Q_OBJECT

public:
  struct normal_dist_data {
    double deviation;
    double clipRadius;

    normal_dist_data(double dev, double radius) : deviation(dev), clipRadius(radius)
    {
    }

    normal_dist_data(const normal_dist_data& o) = default;

    ~normal_dist_data() = default;

    normal_dist_data& operator=(const normal_dist_data& o)=default;


  };

  PointCluster(
    CloudModel* owner=nullptr,
    size_t initialCount=0, const normal_dist_data* normalData=nullptr
  );

  virtual ~PointCluster() = default;


  const QColor& getColor() const { return this->color_; }
  PointCluster& setColor(const QColor &color) {
    this->color_=color;
    emit this->pointsUpdated(this);
    return *this;
  }


  bool isNormal() const { return this->normal_; }
  PointCluster& setNormal(bool normal) { this->normal_=normal; return *this; }

  const normal_dist_data& normalDistribData() const { return this->normal_data_; }
  normal_dist_data& normalDistribData() { return this->normal_data_; }

  PointCluster& setNormalDistribData(const normal_dist_data &normal_data)
  { this->normal_data_=normal_data; return *this; }

  const QPointF& hullLowerLeft() const { return this->p00; }
  const PointCluster& setHullLowerLeft(float x, float y) {
    return this->setHullPoint(this->p00, x, y);
  }
  const PointCluster& setHullLowerLeft(const QPointF& o) {
    return this->setHullPoint(this->p00, o.x(), o.y());
  }

  const QPointF& hullLowerRight() const { return this->p10; }
  const PointCluster& setHullLowerRight(float x, float y) {
    return this->setHullPoint(this->p10,x,y);
  }
  const PointCluster& setHullLowerRight(const QPointF& o) {
    return this->setHullPoint(this->p10,o.x(), o.y());
  }

  const QPointF& hullUpperLeft() const { return this->p01; }
  const PointCluster& setHullUpperLeft(float x, float y) {
    return this->setHullPoint(this->p01, x, y);
  }
  const PointCluster& setHullUpperLeft(const QPointF& o) {
    return this->setHullPoint(this->p01, o.x(), o.y());
  }

  const QPointF& hullUpperRight() const { return this->p11; }
  const PointCluster& setHullUpperRight(float x, float y) {
    return this->setHullPoint(this->p11, x, y);
  }
  const PointCluster& setHullUpperRight(const QPointF& o) {
    return this->setHullPoint(this->p11, o.x(), o.y());
  }

  // places p00, p10, p11, p01 into dest after clearing it
  void getDistorsionHull(QPolygonF& dest) const {
    dest.clear();
    dest << this->p00 << this->p10 << this->p11 << this->p01;
  }

  void setDistorsionHull(const QPolygonF& src) {
    if(4==src.size()) {
      this->p00=src[0];
      this->p10=src[1];
      this->p11=src[2];
      this->p01=src[3];
      this->updateDistorsion();
    }
  }

  const QTransform& transform() const { return this->transform_; }

  const p2d_grp& cluster() const {
    return this->grp_;
  }

  // to allow this function as a supplier for a bound_npoint_cloud
  // (even if getCluster would serve the same purpose). We'll see
  p2d operator()(size_t i) const {
    return this->grp_(i);
  }

  size_t size() const {
    return this->grp_.size();
  }

  void clear() {
    this->grp_.clear();
    emit this->pointsUpdated(this);
  }

  void fill(size_t numExtraPoints);

signals:
  void pointsUpdated(PointCluster*);

private:
  static const QPolygonF& unitBox();
  static std::mt19937& rng();

  const PointCluster& setHullPoint(QPointF& dest, float x, float y) {
    dest.setX(x); dest.setY(y);
    this->updateDistorsion();
    return *this;
  }

  PointCluster& updateDistorsion();

  QColor color_;
  QTransform transform_;
  qtrn_adaptor adapted_transform_;

  p2d_grp grp_;
  // positions of the "bounding box" corners after distorting
  QPointF p00, p01, p10, p11; // the initial bounding box is assumed to be the unit square


  bool normal_;
  normal_dist_data normal_data_;
};

#endif // POINTCLUSTER_HPP
