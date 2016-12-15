#include "mainwindow.hpp"
#include <QApplication>

#include <ctime>
#include <random>

#include "model/model.hpp"
#include "model/dists.hpp"
#include "model/proc.hpp"
#include "view/pointcluster.hpp"
#include "view/chart_utils.hpp"

int main(int argc, char *argv[])
{

  QApplication a(argc, argv);
  MainWindow w;

  w.show();

  return a.exec();
}
