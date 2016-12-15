#include "pointcluster.hpp"
#include "cloudmodel.hpp"

PointCluster::PointCluster(
  CloudModel *owner, size_t initialCount,
  const normal_dist_data *normalData
) :
  QObject(owner),
  color_(Qt::GlobalColor::red), transform_(),
  adapted_transform_(transform_), grp_(&adapted_transform_),
  p00(0.75f, 0.0f), p01(0.75f, 0.25f),
  p10(1.0f, 0.0f), p11(1.0f, 0.25f),
  normal_(false), normal_data_(0.3, 2.0)
{
  if(normalData) {
    this->normal_data_=*normalData;
    this->normal_=true;
  }
  this->updateDistorsion();
  if(initialCount) {
    this->fill(initialCount);
  }
  if(owner) {
    auto slot=&CloudModel::clusterPointsUpdated;
    auto signal=&PointCluster::pointsUpdated;
    QObject::connect(
      this, signal, owner, slot
    );
  }
}




PointCluster& PointCluster::updateDistorsion() {
  QPolygonF distortionHull;
  this->getDistorsionHull(distortionHull);
  QTransform::quadToQuad(PointCluster::unitBox(),distortionHull, this->transform_);
  emit this->pointsUpdated(this);
  return *this;
}

void PointCluster::fill(size_t numExtraPoints) {
  std::mt19937& random=PointCluster::rng();
  if(this->normal_) {
    double dev=this->normal_data_.deviation;
    double clipR2=this->normal_data_.clipRadius*this->normal_data_.clipRadius;
    std::normal_distribution<double> d(0.0, dev);
    while(numExtraPoints>0) {
      double x=d(random), y=d(random);
      if(x*x+y*y<=clipR2) {
        this->grp_.add({coord_type(x+0.5), coord_type(y+0.5)});
      }
      numExtraPoints--;
    }
  }
  else { // uniform distribution
    std::uniform_real_distribution<double> d(0.0, 1.0);
    while(numExtraPoints>0) {
      double x=d(random), y=d(random);
      this->grp_.add({coord_type(x), coord_type(y)});
      numExtraPoints--;
    }
  }
  emit this->pointsUpdated(this);
}

std::mt19937& PointCluster::rng() {
  static std::mt19937 ret;
  static bool inited=false;
  if( ! inited ) {
    // FIXME init with a random seed -see https://xkcd.com/221/
    ret.seed(4);
    inited=true;
  }
  return ret;
}

const QPolygonF& PointCluster::unitBox() {
  static QPolygonF ret;
  if(!ret.size()) { // not inited
    ret << QPointF(0.0f, 0.0f)
        << QPointF(1.0f, 0.0f)
        << QPointF(1.0f, 1.0f)
        << QPointF(0.0f, 1.0f)
    ;
  }
  return ret;
}
