#ifndef H2D_HPP
#define H2D_HPP

#include <QTransform>
#include <QPointF>
#include <QPolygonF>

#include "../model/model.hpp"

inline std::ostream& operator << (std::ostream& o, const QRectF& r) {
  o << "{rect x:" << r.left() << " y:" << r.top() << " w:"<< r.width() << " h:" << r.height() << "}";
  return o;
}

inline std::ostream& operator << (std::ostream& o, const QPointF& r) {
  o << "{point x:" << r.x() << " y:" << r.y() << "}";
  return o;
}

template <typename C> QPointF& assign(QPointF& dest, const distspctr::npoint<C, 2>& src) {
  dest.setX(static_cast<float>(src[0]));
  dest.setY(static_cast<float>(src[1]));
  return dest;
}

template <typename C> distspctr::npoint<C,2>&
assign(distspctr::npoint<C,2>& dest, const QPointF& src) {
  dest(0)=static_cast<C>(src.x());
  dest(1)=static_cast<C>(src.y());
  return src;
}

template <typename C> struct qtransf_adapter {
  const QTransform * trn_;

  distspctr::npoint<C,2> map(const distspctr::npoint<C,2>& p) const {
    distspctr::npoint<C,2> ret=p;
    if( this->trn_) {
      QPointF val;
      assign(val, p);
      val=this->trn_->map(val);
      assign(ret, val);
    }
    return ret;
  }
};

template <typename C> QPolygonF& assign(QPolygonF& dest, const distspctr::npoint_grp<C,2>& src) {
  QPointF val;
  dest.clear();
  for(const distspctr::npoint_grp<C,2>& v : src) {
    assign(val, v);
    dest << val;
  }
  return dest;
}

using coord_type=float;

using p2d=distspctr::npoint<coord_type,2>;

class qtrn_adaptor {
  QTransform& wrapped_;
public:

  qtrn_adaptor(QTransform &other) : wrapped_(other) {

  }

  void wrapped(QTransform &other) {
    this->wrapped_=other;
  }

  p2d map(const p2d& point) const {
    p2d ret;
    qreal px=point[0], py=point[1], tx=px, ty=py;
    this->wrapped_.map(px, py, &tx, &ty);
    ret[0]=tx; ret[1]=ty;
    return ret;
  }
};

using p2d_grp=distspctr::npoint_grp<float,2,qtrn_adaptor>;

#include "../model/dists.hpp"

using l2dist=distspctr::l2<coord_type,2>;

#endif // H2D_HPP
