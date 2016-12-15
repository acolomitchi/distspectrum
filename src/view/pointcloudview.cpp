#include <numeric>

#include <QResizeEvent>
#include <QPaintEvent>
#include <QPainter>

#include "pointcloudview.hpp"

PointCloudView::PointCloudView(QWidget *parent, CloudModel* owner) :
  QWidget(parent), model_(nullptr), selected_knob_(-1), global_trn_()
{
  this->setStyleSheet("background-color:black;color:white");
  this->setAutoFillBackground(true);
  this->setModel(owner);
}

void PointCloudView::setModel(CloudModel* newOwner)
{
  if(this->model_!=newOwner) {
    if(this->model_) {
      // disconnect from the old signals
      while(this->model_conn_.size()>0) {
        QObject::disconnect(this->model_conn_.last());
        this->model_conn_.pop_back();
      }
    }
    this->model_=newOwner;
    if(newOwner) {
      auto changedSignal=&CloudModel::pointsChanged;
      this->model_conn_.push_back(
        QObject::connect(
          newOwner, changedSignal,
          [this](CloudModel*) { this->update(); }
        )
      );
      auto selChSignal=&CloudModel::selectionChanged;
      this->model_conn_.push_back(
        QObject::connect(
          newOwner, selChSignal,
          [this](const PointCluster*) { this->update(); }
        )
      );
    }
  }

}

static bool neighhit(const QPointF& p0, const QPointF& p1, qreal threshold) {
  qreal dx=p0.x()-p1.x(), dy=p0.y()-p1.y();
  return (dx*dx+dy*dy<=threshold*threshold); // maybe manhatan dist?
}

void PointCloudView::mousePressEvent(QMouseEvent *e) {
  if(e->button()==Qt::MouseButton::LeftButton) {
    if(this->model_ && this->model_->getSelection()) {
      const PointCluster* selection=this->model_->getSelection();
      this->selected_knob_=-1;
      QPolygonF distortionHull;
      selection->getDistorsionHull(distortionHull);
      distortionHull=this->global_trn_.map(distortionHull);
      QPointF pos=e->localPos();
      unsigned len=distortionHull.size();
      QPointF p0=distortionHull[len-1], baricentre(0.0f, 0.0f);
      for(unsigned i=0; i<len && this->selected_knob_<0; i++) {
        QPointF p1=distortionHull[i];
        if(this->selected_knob_<0 && neighhit(p1, pos, 3)) {
          this->selected_knob_=int(i);
        }
        if(this->selected_knob_<0) {
          // No? What a pity!
          p0+=p1;
          p0/=2; // mid with the prev point
          if(neighhit(p0, pos, 3)) {
            this->selected_knob_=int(i+4);
            break;
          }
        }
        baricentre+=p1;
        p0=p1;
      }
      if(this->selected_knob_<0) {
        baricentre /=len;
        if(neighhit(baricentre, pos, 5)) {
          this->selected_knob_=2*len;
        }
      }
      if(this->selected_knob_>=0) {
        this->update();
      }
    }
  }
}

static void normalize(QPointF& p) {
  qreal norm=std::hypot(p.x(), p.y());
  if(norm>1e-7) {
    p/=norm;
  }
}

void PointCloudView::mouseMoveEvent(QMouseEvent *event) {
  if(this->model_ && this->model_->getSelection() && this->selected_knob_>=0) {
    bool invertible=true;
    QTransform inv=this->global_trn_.inverted(&invertible);
    assert(invertible); // based on the possible manupulation, the matrix shoul never be singular

    PointCluster* selection=this->model_->getSelection();

    QPolygonF distortionHull;
    selection->getDistorsionHull(distortionHull);
    distortionHull=this->global_trn_.map(distortionHull);
    int len=distortionHull.size();
    if(this->selected_knob_<len) {
      // distortion
      QPointF pos=event->localPos();
      pos=inv.map(pos);
      switch(this->selected_knob_) {
        case 0:
          selection->setHullLowerLeft(pos);
          break;
        case 1:
          selection->setHullLowerRight(pos);
          break;
        case 2:
          selection->setHullUpperRight(pos);
          break;
        case 3:
          selection->setHullUpperLeft(pos);
        default:
          break;
      }
    }
    else if(this->selected_knob_<2*len) {
      // The baticentre stays put
      QPointF baricentre(0.0, 0.0);
      for(int i=0; i<len; i++) {
        baricentre+=distortionHull[i];
      }
      baricentre/=len;
      int currIx=this->selected_knob_-len;
      int prevIx=(this->selected_knob_-1)%len;
      QPointF prevPos=(distortionHull[currIx]+distortionHull[prevIx])/2.0;
      QPointF newPos=event->localPos();
      // everything happens around baricentre
      prevPos-=baricentre; newPos-=baricentre;
      normalize(prevPos); normalize(newPos);
      qreal cos=QPointF::dotProduct(prevPos, newPos);
      qreal sin=prevPos.y()*newPos.x()-prevPos.x()*newPos.y();

      // now compute a transform that does:
      // move the baricenter in the origon
      // apply a rotation with the angle
      // move the baricenter back
      QTransform local;
      local.translate(baricentre.x(), baricentre.y());
      QTransform rotation(cos, -sin, sin, cos, 0, 0);
      local=rotation*local;
      local.translate(-baricentre.x(), -baricentre.y());
      distortionHull=local.map(distortionHull);
      // then translate into the model coordinates
      distortionHull=inv.map(distortionHull);
      this->applyDistortion(distortionHull);
    }
    else if(this->selected_knob_==2*len) {
      // translation
      QPointF baricentre(0.0, 0.0);
      for(int i=0; i<len; i++) {
        baricentre+=distortionHull[i];
      }
      baricentre/=len;
      qreal dx=event->x()-baricentre.x(), dy=event->y()-baricentre.y();
      distortionHull.translate(dx, dy);
      distortionHull=inv.map(distortionHull);
      this->applyDistortion(distortionHull);
    }
  }
}

void PointCloudView::applyDistortion(const QPolygonF& distortionHull) {
  if(this->model_ && this->model_->getSelection()) {
     PointCluster* selection=this->model_->getSelection();
     selection->setDistorsionHull(distortionHull);
  }
}

void PointCloudView::mouseReleaseEvent(QMouseEvent * /*event*/) {
  this->selected_knob_=-1;
  this->update();
}

void PointCloudView::resizeEvent(QResizeEvent* event) {
  QWidget::resizeEvent(event);
  const QSize& sz=event->size();
  qreal origX, origY, scale;
  if(sz.width()<sz.height()) {
    origX=0.0;
    origY=(sz.height()-sz.width())/2.0;
    scale=sz.width();
  }
  else {
    origY=0.0;
    origX=(sz.width()-sz.height())/2.0;
    scale=sz.height();
  }
  // so the transform:
  // flip the yAxis and scale
  // move to origX, height-origY
  this->global_trn_.setMatrix(
      1.0, 0.0, 0.0,
      0.0, 1.0, 0.0,
      0.0, 0.0, 1.0
  );
  this->global_trn_.scale(scale, -scale);
  this->global_trn_.translate(origX/scale, (origY-sz.height())/scale);
  // this->global_trn_.scale(scale, scale);
}

void PointCloudView::paintEvent(QPaintEvent *e) {
  QWidget::paintEvent(e);

  QPainter painter(this);
  painter.setPen(Qt::PenStyle::NoPen);

  const QColor& bkgColor=this->palette().color(QPalette::Background);
  painter.setBrush(QBrush(bkgColor));
  painter.drawRects(e->region().rects());

  painter.setBrush(QBrush(Qt::GlobalColor::white));

  QRectF bkgRect(QPointF(0,0), QSize(1,1));
  bkgRect=this->global_trn_.mapRect(bkgRect);
  painter.drawRect(bkgRect);

  if(this->model_) {
    QPointF probe;
    QVector<p2d> clusterPoints;

    for(const PointCluster* cluster : this->model_->clusters()) {
      clusterPoints.clear();
      this->model_->getClusterPoints(cluster, clusterPoints);
      painter.setPen(QPen(cluster->getColor()));
      painter.setBrush(QBrush(cluster->getColor()));
      for(auto p : clusterPoints) {
        assign(probe, p);
        probe=this->global_trn_.map(probe);
        painter.drawPoint(probe);
      }
    }

    if(this->model_->getSelection()) {
      PointCluster* selection_=this->model_->getSelection();
      QPolygonF distortionHull;
      selection_->getDistorsionHull(distortionHull);
      distortionHull=this->global_trn_.map(distortionHull);
      QPen pen(Qt::GlobalColor::black);
      pen.setCapStyle(Qt::PenCapStyle::SquareCap);
      pen.setJoinStyle(Qt::PenJoinStyle::MiterJoin);
      pen.setStyle(Qt::PenStyle::DashLine);
      pen.setWidthF(1.05);
      painter.setPen(pen);
      painter.setBrush(Qt::BrushStyle::NoBrush);
      painter.drawPolygon(distortionHull);

      // knobs
      pen.setStyle(Qt::PenStyle::SolidLine);
      painter.setPen(pen);
      QBrush knobFill(Qt::GlobalColor::black);
      QRectF knobRect({-2.0f, -2.0f}, QSize(5.0f, 5.0f));
      int len=distortionHull.size();
      QPointF p0 = distortionHull[len-1], baricentre(0.0f, 0.0f);
      for(int i=0; i<distortionHull.size(); i++) {
        const QPointF& p1=distortionHull[i];
        if(this->selected_knob_==i) {
          painter.setBrush(knobFill);
        }
        else {
          painter.setBrush(Qt::BrushStyle::NoBrush);
        }
        knobRect.moveCenter(p1);
        painter.drawRect(knobRect);
        p0+=p1;
        p0/=2;
        if(i+len==this->selected_knob_) {
          painter.setBrush(knobFill);
        }
        else {
          painter.setBrush(Qt::BrushStyle::NoBrush);
        }
        knobRect.moveCenter(p0);
        painter.drawRect(knobRect);
        baricentre+=p1;
        p0=p1;
      }
      baricentre /= len;
      knobRect.adjust(-2, -2, 2, 2);
      knobRect.moveCenter(baricentre);
      QPainterPath centralKnob;
      centralKnob.addEllipse(knobRect);
      pen.setWidthF(1.75);
      painter.setPen(pen);
      if(2*len==this->selected_knob_) {
        painter.setBrush(knobFill);
      }
      else {
        painter.setBrush(Qt::BrushStyle::NoBrush);
      }
      painter.drawPath(centralKnob);
    }
  }
}
