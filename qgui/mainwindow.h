#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ui_mainwindow.h"

#include <arpa/inet.h>
#include "../libcomm/comm.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow, public Ui::MainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    void setupMenu();

    Ui::MainWindow *ui;
    comm_t *_comm;
    char *_name;
    enum AddTextType
    {
      INFO
    };

public slots:
    void btnSendHandler();
    void dlgConnectionSet(QString &host);
    void txtNameChanged();
    void addtext(AddTextType t, const char *s);

private slots:
    void on_actionDisconnect_triggered();
    void on_actionConnect_triggered();
};

#endif // MAINWINDOW_H
