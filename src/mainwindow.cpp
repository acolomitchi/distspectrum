#include "mainwindow.hpp"
#include "view/pointcloudview.hpp"
#include "view/chart_utils.hpp"

#include "ui_mainwindow.h"

#include <QLineSeries>

#define BASE 10

inline void compute_yrange(qreal& minY, qreal& maxY) {
  qreal minUnit=
      (minY==0)
    ? 1.0
    : std::pow(BASE, std::floor(std::log(std::abs(minY))/std::log(BASE)))
  ;
  qreal maxUnit=
      (maxY==0)
    ? 1.0
    : std::pow(BASE, std::floor(std::log(std::abs(maxY))/std::log(BASE)))
  ;
  qreal unit=std::min(minUnit, maxUnit);
  qreal minR=minY=( minY<0 ? -std::ceil(-minY/minUnit) : std::floor(minY/minUnit) )*minUnit;
  qreal maxR=( maxY<0 ? -std::floor(-maxY/maxUnit) : std::ceil(maxY/maxUnit) )*maxUnit;
  int delta=std::round((maxR-minR)/unit);
  switch(delta % 4) {
    case 1: // add it to the min
      minY=minR-unit; maxY=maxR;
      break;
    case 2: // add to the both of them
      minY=minR-unit; maxY=maxR+unit;
      break;
    case 3: // 1 up 2 down
      minY=minR-2*unit; maxY=maxR+unit;
      break;
    default: // ok
      minY=minR; maxY=maxR;
      break;
  }
}

MainWindow::MainWindow(QWidget *parent) :
  QMainWindow(parent),
  ui(new Ui::MainWindow),
  experimental_(nullptr), baseline_(nullptr), diff_(nullptr)
{
  ui->setupUi(this);

  this->ui->points->setModel(this->ui->ctrl->getExperimentalCloud());

  this->experimental_=new QLineSeries(this);
  this->experimental_->setName("Custom");
  this->experimental_->setPointsVisible(false);
  this->experimental_->setColor(Qt::blue);

  this->baseline_=new QLineSeries(this);
  this->baseline_->setName("Baseline");
  this->baseline_->setPointsVisible(false);
  this->baseline_->setColor(Qt::green);

  this->diff_=new QLineSeries(this);
  this->diff_->setName("diff");
  this->diff_->setPointsVisible(false);
  this->diff_->setColor(Qt::red);

  auto ctrlDataSeriesSignal=&ControllerForm::hasSeriesUpdates;
  QObject::connect(
    this->ui->ctrl, ctrlDataSeriesSignal, this,
    [&](const ControllerForm* src) {
      qreal mins[3], maxes[3];
      qreal expProgress, baselineProgress;
      src->fillBaselineSeries(*this->baseline_, baselineProgress, &mins[0], &maxes[0]);
      src->fillExperimentalSeries(*this->experimental_, expProgress, &mins[1], &maxes[1]);
      src->fillDiffSeries(*this->diff_, &mins[2], &maxes[2]);
      this->ui->experProgress->setValue(int(expProgress*100));
      this->ui->baselineProgress->setValue(int(baselineProgress*100));
      // unwrapped bubble-sort down
      if(mins[1]>mins[2]) std::swap(mins[1], mins[2]);
      if(mins[0]>mins[1]) std::swap(mins[0], mins[1]);
      if(maxes[1]<maxes[2]) std::swap(maxes[1], maxes[2]);
      if(maxes[0]<maxes[1]) std::swap(maxes[0], maxes[1]);
      if(maxes[0]>mins[0]) { // otherwise all the series are empty
        compute_yrange(mins[0], maxes[0]);
        this->y_axis_->setRange(mins[0], maxes[0]);
      }
    },
    Qt::QueuedConnection
  );

  this->chart_=new QChart();
  this->chart_->setAnimationOptions(QChart::AllAnimations);
  this->chart_->addSeries(this->baseline_);
  this->chart_->addSeries(this->experimental_);
  this->chart_->addSeries(this->diff_);

  QValueAxis* xAxis=new QValueAxis(this->chart_);
  xAxis->setRange(0.0, 1.5);
  xAxis->setLabelFormat("%2g");
  this->y_axis_=new QValueAxis(this->chart_);
  this->y_axis_->setRange(-0.1, 0.1);
  this->y_axis_->setLabelFormat("%2g");

  this->chart_->addAxis(this->y_axis_, Qt::AlignLeft);
  this->chart_->setAxisX(xAxis);

  this->baseline_->attachAxis(xAxis);
  this->baseline_->attachAxis(this->y_axis_);
  this->chart_->setAxisY(this->y_axis_, this->experimental_);
  this->chart_->setAxisY(this->y_axis_, this->diff_);
  this->chart_->setAxisX(xAxis, this->experimental_);
  this->chart_->setAxisX(xAxis, this->diff_);

  this->chart_->setAnimationDuration(750);

  this->ui->chart->setChart(this->chart_);

}

MainWindow::~MainWindow()
{
  delete this->chart_;
  delete ui;
}
