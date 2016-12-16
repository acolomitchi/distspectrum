#-------------------------------------------------
#
# Project created by QtCreator 2016-11-29T21:37:19
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets charts

TARGET = distspectrum
TEMPLATE = app


SOURCES +=\
    src/main.cpp \
    src/mainwindow.cpp \
    src/view/pointcluster.cpp \
    src/view/pointcloudview.cpp \
    src/view/controllerform.cpp \
    src/view/cloudmodel.cpp \
    src/view/clustersettings.cpp \
    src/view/l2xyhistogramcollector.cpp

HEADERS  += \
    src/model/dists.hpp \
    src/model/model.hpp \
    src/model/proc.hpp \
    src/mainwindow.hpp \
    src/view/2d.hpp \
    src/view/pointcluster.hpp \
    src/view/pointcloudview.hpp \
    src/view/controllerform.hpp \
    src/view/cloudmodel.hpp \
    src/view/clustersettings.hpp \
    src/view/chart_utils.hpp \
    src/typeout.hpp \
    src/view/l2xyhistogramcollector.hpp

FORMS    += \
    src/mainwindow.ui \
    src/view/controllerform.ui \
    src/view/clustersettings.ui

EIGEN_DIR = $$PWD/../../../c++-extra-libs/eigen3.3

INCLUDEPATH += $$EIGEN_DIR

CONFIG(debug, debug|release) {
  QMAKE_CXXFLAGS+=-O0
  QMAKE_LFLAGS+=-O0
}

CONFIG(release, debug|release) {
#  LIBS+= -lefence
  CONFIG += optimize_full
}

