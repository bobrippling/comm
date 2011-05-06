#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "dialogconnect.h"
#include "../../config.h"

#include <cstdio>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
  _comm(NULL),
  _name(NULL)
{
    ui->setupUi(this);
    setupMenu();

    _comm = new comm_t;
    comm_init(_comm);
    _name = new char[32];
    strcpy(_name, "Tim");
}

void MainWindow::setupMenu()
{
    connect(
          actionConnect,
          SIGNAL(triggered()),
          this,
          SLOT(on_actionConnect_triggered());
}


MainWindow::~MainWindow()
{
    delete ui;
    comm_close(_comm);
    delete _comm;
}

void MainWindow::btnSendHandler()
{
    printf("fucking finally\n");
}

void MainWindow::addtext(AddTextType t, const char *s)
{
  (void)t;
  printf("addtext(): %s\n", s);
}

void MainWindow::txtNameChanged()
{
  QString name = txtName->toPlainText();

  if(name.length() > 31)
    addtext(INFO, "Name must be less than 32 chars");
  else{
    strcpy(_name, name.toStdString().c_str());
    printf("name changed: \"%s\"\n", _name);
  }
}

void MainWindow::dlgConnectionSet(QString &addr)
{
  std::string str = addr.toStdString();
  char *host;
  const char *port;

  if(!_name)
    return;

  host = new char[str.length() + 1];

  strcpy(host, str.c_str());

  port = strchr(host, ':');
  if(!port++)
    port = DEFAULT_PORT;

  comm_connect(_comm, host, port, _name);

  delete[] host;
}

void MainWindow::on_actionDisconnect_triggered()
{
    printf("disco trigger\n");
}

void MainWindow::on_actionConnect_triggered()
{
  printf("conn trigger\n");

  DialogConnect *dc = new DialogConnect();

  connect(dc,
      SIGNAL(connectionSet(QString&)),
      this,
      SLOT(dlgConnectionSet(QString&)));

  dc->exec();
}
