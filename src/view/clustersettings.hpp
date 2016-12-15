#ifndef ABSTRACTCLUSTERSETTINGS_HPP
#define ABSTRACTCLUSTERSETTINGS_HPP

#include <QWidget>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>

#include "pointcluster.hpp"

namespace Ui {
class ClusterSettings;
}

class ClusterSettings : public QWidget
{
  Q_OBJECT

public:
  explicit ClusterSettings(QWidget *parent = 0, PointCluster* getEdited=0);
  ~ClusterSettings();

  PointCluster* getEdited() const {
    return this->edited_;
  }

  void setSelectionStatus(bool newStatus);

signals:
  void refreshRequest(ClusterSettings* changed);

  void distorsionVisibilityChange(ClusterSettings* changed, bool visible);

  void deletionRequest(ClusterSettings* settings);

private:
  PointCluster* edited_;

  Ui::ClusterSettings *ui;

  typedef struct inner_data_* inner_data;

  inner_data data_;

  void showInnerData();

  // in response to the signal of clusterType
  void clusterTypeChange(int selectionIndex);
  // in response to signal from refillBtn
  void doRefill();
  // in reponse to the colorSelect
  void colorChange(int selectionIndex);
  // in reponse to the pointCount
  void pointCountChange(int newCount);
  // in reponse to the showSelection
  void showSelectionChange(int newState);
  // in reponse to the deleteBtn
  void deleteMe();

  // in response to the stdDev
  void stdDevChanged(double val);
  // in response to the clipRadius
  void clipRadiusChanged(double val);
};

#endif // ABSTRACTCLUSTERSETTINGS_HPP
