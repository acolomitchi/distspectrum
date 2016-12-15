#include "controllerform.hpp"
#include "ui_controllerform.h"

ControllerForm::ControllerForm(QWidget *parent) :
  QWidget(parent),
  ui(new Ui::ControllerForm),
  edited_model_(nullptr), baseline_model_(nullptr),
  histogram_collector_(nullptr), cluster_editors_()
{
  ui->setupUi(this);

  this->ui->cbExhaustiveDists->setChecked(true);
  this->ui->maxSampleDists->setDisabled(true);
  this->ui->maxSampleDists->setValue(4000000);

  QVBoxLayout* supportLayout=new QVBoxLayout();
  supportLayout->setSizeConstraint(QLayout::SetFixedSize);
  supportLayout->setContentsMargins(0,2,0,2);
  this->ui->clusterSupport->setLayout(supportLayout);
  // this->ui->scrollArea->setWidgetResizable(false);
  QPushButton* newClusterBtn=this->ui->newClusterBtn;
  auto btnClickedSignal=&QPushButton::clicked;
  QObject::connect(
     newClusterBtn, btnClickedSignal,
     [this](bool) {
       this->initiateClusterCreation();
     }
  );
  auto cbValChSignal=&QCheckBox::stateChanged;
  QObject::connect(
    this->ui->cbExhaustiveDists, cbValChSignal,
    [this](int) { this->updateSampledDistUi(); }
  );
  using sb_signal_type=void (QSpinBox::*)(int);
  sb_signal_type sbValChSignal=&QSpinBox::valueChanged;
  QObject::connect(
    this->ui->maxSampleDists, sbValChSignal,
    [this](int) { this->updateSampledDistUi(); }
  );
  this->initClouds();
}

ControllerForm::~ControllerForm()
{
  delete ui; // all the other created are register children
}

size_t ControllerForm::fillExperimentalSeries(QXYSeries& dest, qreal& progPct, qreal* min, qreal* max) const {
  return this->histogram_collector_->experimentalSeries(dest, progPct, min, max);
}
size_t ControllerForm::fillBaselineSeries(QXYSeries& dest, qreal& progPct, qreal* min, qreal* max) const {
  return this->histogram_collector_->baselineSeries(dest, progPct, min, max);
}
size_t ControllerForm::fillDiffSeries(QXYSeries& dest, qreal* min, qreal* max) const {
  return this->histogram_collector_->diffSeries(dest, min, max);
}


void ControllerForm::initClouds() {
  this->edited_model_=new CloudModel(this);
  auto createdSig=&CloudModel::clusterAdded;
  QObject::connect(
    this->edited_model_, createdSig,
    [this](PointCluster* created) { this->editedClusterCreated(created); }
  );
  auto removedSig=&CloudModel::clusterRemoved;
  QObject::connect(
    this->edited_model_, removedSig,
    [this](const PointCluster* deleted) { this->editedClusterRemoved(deleted); }
  );
  auto selChanged=&CloudModel::selectionChanged;
  QObject::connect(
    this->edited_model_, selChanged,
    [this](const PointCluster* selected) {
      for(auto& editor : this->cluster_editors_) {
        editor->setSelectionStatus(editor->getEdited()==selected);
      }
    }
  );
  this->baseline_model_=new CloudModel(this);
  // TODO adjust the number of points in baseline or make it somehow configurable
  PointCluster* cluster=this->baseline_model_->createCluster(4000); // 16e6 distances

  this->histogram_collector_= new L2XYHistogramCollector(
    *this->edited_model_, *this->baseline_model_, this
  );
  auto collSignal=&L2XYHistogramCollector::updated;
  QObject::connect(
    this->histogram_collector_, collSignal,
    [&](const L2XYHistogramCollector* c) {
      if(this->histogram_collector_==c) {
        emit hasSeriesUpdates(this);
      }
    }
  );
  // not only this restores the hull to the unit square, but it should trigger
  // an update for the baseline
  QPolygonF d;
  d << QPointF(0,0) << QPointF(1,0) << QPointF(1,1) << QPointF(0,1);
  cluster->setDistorsionHull(d);

}

void ControllerForm::editedClusterCreated(PointCluster* cluster) {
  QWidget* supp=this->ui->clusterSupport;
  ClusterSettings* newItem=new ClusterSettings(supp, cluster);
  this->cluster_editors_.push_back(newItem);
  supp->layout()->addWidget(newItem);
  newItem->setSelectionStatus(this->edited_model_->getSelection()==cluster);
  auto selectSignal=&ClusterSettings::distorsionVisibilityChange;
  QObject::connect(
    newItem, selectSignal,
    [this](ClusterSettings* sender, bool state) {
      this->edited_model_->setSelection(state ? sender->getEdited() : nullptr);
    }
  );
  auto deletionRequestSignal=&ClusterSettings::deletionRequest;
  QObject::connect(
     newItem, deletionRequestSignal,
     [this](ClusterSettings* editor) {
       if(this->cluster_editors_.contains(editor)) {
         this->edited_model_->removeCluster(editor->getEdited());
       }
     }
  );
}

void ControllerForm::editedClusterRemoved(const PointCluster* cluster) {
  for(int ix=0; ix<this->cluster_editors_.size(); ix++) {
    ClusterSettings* editor=this->cluster_editors_[ix];
    if(editor->getEdited()==cluster) {
      this->cluster_editors_.remove(ix);
      QWidget* supp=this->ui->clusterSupport;
      supp->layout()->removeWidget(editor);
      editor->hide();
      editor->deleteLater();
      supp->updateGeometry();
      break;
    }
  }
}

void ControllerForm::updateSampledDistUi() {
  this->ui->maxSampleDists->setDisabled(this->ui->cbExhaustiveDists->isChecked());
  size_t maxDistSampleCount=
      this->ui->cbExhaustiveDists->isChecked()
    ? std::numeric_limits<size_t>::max()
    : size_t(this->ui->maxSampleDists->value())
  ;
  this->histogram_collector_->setMaxDistSamples(maxDistSampleCount);
}
