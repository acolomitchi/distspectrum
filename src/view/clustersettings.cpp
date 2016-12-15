#include <iostream>

#include <QColor>
#include <QPixmap>
#include <QIcon>
#include <QVariant>

#include "clustersettings.hpp"
#include "ui_clustersettings.h"

inline void addColorItem(QComboBox* dest, const QColor& color) {
  QVariant colorVariant=color;
  QPixmap colorIcon(QSize(16,16));
  colorIcon.fill(color);
  dest->addItem(QIcon(colorIcon), "", colorVariant);
}

struct inner_data_ {
  bool normal_;
  QColor color_;
  size_t point_count_;
  qreal std_dev_;
  qreal clip_radius_;
  bool selection_shown_;

  inner_data_(PointCluster* edited=nullptr) :
    normal_(true), color_(Qt::red), point_count_(1000),
    std_dev_(0.3), clip_radius_(0.5),
    selection_shown_(false)
  {
    this->read_from(edited);
  }

  void read_from(PointCluster* edited) {
    if(edited) {
      this->normal_=edited->isNormal();
      this->point_count_=edited->size();
      this->color_= edited->getColor();
      this->std_dev_=edited->normalDistribData().deviation;
      this->clip_radius_=edited->normalDistribData().clipRadius;
    }
  }
};

ClusterSettings::ClusterSettings(QWidget *parent, PointCluster* edited) :
  QWidget(parent),
  edited_(edited),
  ui(new Ui::ClusterSettings),
  data_(new inner_data_(edited))

{  

  static QColor colors[8]={
    Qt::red , Qt::green, QColor("#33F"), QColor("#800"),
    QColor("#C0C"), QColor("#0CC"), QColor("#CC0"), QColor("#366")
  };
  static size_t last_color_ix=0;

  ui->setupUi(this);

  for(size_t i=0; i<8; i++) {
    addColorItem(this->ui->colorSelect, colors[i]);
  }
  this->data_->color_=colors[last_color_ix];
  edited->setColor(this->data_->color_);
  last_color_ix = (last_color_ix+1) % 8;


  this->ui->pointCount->setAccelerated(true);
  this->ui->pointCount->setRange(0, 10000);
  this->ui->pointCount->setSingleStep(50);

  this->ui->deleteBtn->setIcon(this->style()->standardIcon(QStyle::SP_TrashIcon));

  this->ui->stdDev->setRange(0.01, 2.00);
  this->ui->stdDev->setSingleStep(0.05);

  this->ui->clipRadius->setRange(0.05, 4.00);
  this->ui->clipRadius->setSingleStep(0.05);

  this->ui->pointCount->setStyleSheet("padding:0");
  this->ui->stdDev->setStyleSheet("padding:0");
  this->ui->clipRadius->setStyleSheet("padding:0");
  this->ui->colorSelect->setStyleSheet("padding:1");
  this->ui->clusterType->setStyleSheet("padding:1");
  this->ui->showSelection->setStyleSheet("padding:0");
  this->ui->refillBtn->setStyleSheet("padding:1");
  this->ui->deleteBtn->setStyleSheet("padding:1");

  auto chbSignal = &QCheckBox::clicked;
  QObject::connect(
    this->ui->showSelection, chbSignal,
    [this]() {
      int state=this->ui->showSelection->isChecked();
      this->showSelectionChange(state);
    }
  );
  void (QComboBox::* cbSignal)(int) = &QComboBox::currentIndexChanged;
  QObject::connect(this->ui->clusterType, cbSignal,
                [this](int ix) { this->clusterTypeChange(ix);}
  );
  QObject::connect(this->ui->colorSelect, cbSignal,
                [this](int ix) { this->colorChange(ix);}
  );

  void (QSpinBox::* spSignal)(int) = &QSpinBox::valueChanged;
  QObject::connect(this->ui->pointCount, spSignal, [this](int val) {this->pointCountChange(val);});

  void (QDoubleSpinBox::* dsbSignal)(double) = &QDoubleSpinBox::valueChanged;
  QObject::connect(this->ui->stdDev, dsbSignal, [this](double val){ this->stdDevChanged(val);});
  QObject::connect(this->ui->clipRadius, dsbSignal, [this](double val) { this->clipRadiusChanged(val);});

  auto btnClickedSig=&QPushButton::clicked;
  QObject::connect(this->ui->refillBtn, btnClickedSig, [this](bool) { this->doRefill();});
  QObject::connect(this->ui->deleteBtn, btnClickedSig, [this](bool) { this->deleteMe();});
  this->showInnerData();
}

ClusterSettings::~ClusterSettings()
{
  delete ui;
}

void ClusterSettings::setSelectionStatus(bool newStatus) {
  this->ui->showSelection->setChecked(newStatus);
  this->data_->selection_shown_=newStatus;
}

void ClusterSettings::showInnerData() {
  this->ui->pointCount->setValue(this->data_->point_count_);
  int ix=this->ui->colorSelect->findData(this->data_->color_);
  this->ui->colorSelect->setCurrentIndex(ix);
  ix=this->data_->normal_ ? 1 : 0;
  this->ui->clusterType->setCurrentIndex(ix);
  this->ui->stdDev->setValue(this->data_->std_dev_);
  this->ui->clipRadius->setValue(this->data_->clip_radius_);
  this->ui->showSelection->setChecked(this->data_->selection_shown_);
  if(this->data_->normal_) {
    this->ui->normalDataArea->show();
  }
  else {
    this->ui->normalDataArea->hide();
  }
}

// in response to the signal of clusterType
void ClusterSettings::clusterTypeChange(int selIndex) {
  bool normal=(selIndex>0);
  if(normal) {
    this->ui->normalDataArea->show();
  }
  else {
    this->ui->normalDataArea->hide();
  }
  this->data_->normal_=normal;
}

// in response to signal from refillBtn
void ClusterSettings::doRefill() {
  if(this->edited_) {
    this->edited_->setNormal(this->data_->normal_);
    PointCluster::normal_dist_data normalData(
      this->data_->std_dev_, this->data_->clip_radius_
    );
    this->edited_->setNormalDistribData(normalData);
    this->edited_->clear();
    this->edited_->fill(this->data_->point_count_);
    emit refreshRequest(this);
  }
}

// in reponse to the colorSelect
void ClusterSettings::colorChange(int ) {
 QColor val=this->ui->colorSelect->currentData().value<QColor>();
 if(this->edited_) {
   this->edited_->setColor(val);
 }
 this->data_->color_=val;
 emit refreshRequest(this);
}

// in reponse to the pointCount
void ClusterSettings::pointCountChange(int newCount) {
  this->data_->point_count_=newCount;
}

// in reponse to the showSelection
void ClusterSettings::showSelectionChange(int newState) {
  this->data_->selection_shown_=newState;
  emit distorsionVisibilityChange(this, this->data_->selection_shown_);

}

// in reponse to the deleteBtn
void ClusterSettings::deleteMe() {
  emit deletionRequest(this);
}

// in response to the stdDev
void ClusterSettings::stdDevChanged(double val) {
  this->data_->std_dev_=val;
}

// in response to the clipRadius
void ClusterSettings::clipRadiusChanged(double val) {
  this->data_->clip_radius_=val;
}


