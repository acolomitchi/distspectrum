#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include "view/pointcloudview.hpp"

QT_CHARTS_USE_NAMESPACE

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();



private:
  Ui::MainWindow *ui;

  QLineSeries *experimental_;
  QLineSeries *baseline_;
  QLineSeries *diff_;
  QChart* chart_;
  QValueAxis* y_axis_;

};

#endif // MAINWINDOW_HPP
