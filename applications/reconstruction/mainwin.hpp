#pragma once

#include <QtGui>
#include <QtWidgets>

#include "ui_mainwin.h"
#include "ogrewidget.hpp"
#include "workthread.hpp"

class MainWind : public QMainWindow {

    Q_OBJECT

public:
    MainWind(QWidget *parent = 0);

    void initGui();

public slots:
    void on_actionInsert_Panorama_triggered();
    void on_actionInsert_View_triggered();

private:
    OgreWidget * _w;
    WorkThread * _thread;
    QProgressBar * _progressBar;
    Ui::MainWind _ui;
};