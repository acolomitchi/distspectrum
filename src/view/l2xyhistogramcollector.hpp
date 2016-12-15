#ifndef L2LINEHISTOGRAMCOLLECTOR_HPP
#define L2LINEHISTOGRAMCOLLECTOR_HPP

#include <QObject>

#include <QtCharts/QXYSeries>

QT_CHARTS_USE_NAMESPACE

#include "cloudmodel.hpp"
#include "chart_utils.hpp"

class L2XYHistogramCollector :
    public QObject, public DiffHistogramCollector<QXYSeries, l2dist>
{
  Q_OBJECT

public:
  L2XYHistogramCollector(
    const CloudModel& experimental, const CloudModel& baseline,
    QObject* parent=nullptr, size_t histogramSlots=100,
    size_t maxDistCount=std::numeric_limits<size_t>::max()
  );

  virtual ~L2XYHistogramCollector() {}

  void setMaxDistSamples(size_t maxDistSamples);

signals:
  void updated(const L2XYHistogramCollector* thizz);

protected:
  virtual void processUpdate(std::shared_ptr<distspctr::histogram<coord_type> > hist,
    std::vector<QPointF>& dest
  );

private:
  l2dist dist_;
  size_t max_dists_samples_;
};
#endif // L2LINEHISTOGRAMCOLLECTOR_HPP
