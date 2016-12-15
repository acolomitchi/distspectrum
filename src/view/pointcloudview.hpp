#ifndef POINTCLOUDSVIEW_HPP
#define POINTCLOUDSVIEW_HPP

#include <QWidget>

#include "cloudmodel.hpp"

class PointCloudView : public QWidget
{
  Q_OBJECT
public:
  explicit PointCloudView(QWidget *parent = 0, CloudModel* getModel=0);

  void setModel(CloudModel* newOwner);

  CloudModel* getModel() const {
    return this->model_;
  }

protected:
  virtual void resizeEvent(QResizeEvent* event) Q_DECL_OVERRIDE;

  virtual void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

  virtual void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

  virtual void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

  virtual void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;



private:

  void applyDistortion(const QPolygonF& distortionHull);

  CloudModel* model_;
  // 0-3 - corners, 5-7 side mids, 8 - centre, nothing otherwise
  int selected_knob_; // valid only during drag ops

  QTransform global_trn_;

  QVector<QMetaObject::Connection> model_conn_;
};

#endif // POINTCLOUDSVIEW_HPP
