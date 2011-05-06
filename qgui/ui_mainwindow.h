/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created: Fri May 6 00:02:11 2011
**      by: Qt User Interface Compiler version 4.7.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGraphicsView>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QListView>
#include <QtGui/QMainWindow>
#include <QtGui/QPushButton>
#include <QtGui/QSplitter>
#include <QtGui/QTextEdit>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QSplitter *splitter_7;
    QSplitter *splitter_6;
    QSplitter *splitter_3;
    QSplitter *splitter_2;
    QLabel *label;
    QTextEdit *txtName;
    QSplitter *splitter;
    QListView *lstNames;
    QSplitter *splitter_5;
    QTextEdit *txtMain;
    QSplitter *splitter_4;
    QTextEdit *txtMsg;
    QPushButton *btnSend;
    QGraphicsView *gfxDrawan;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(672, 500);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        gridLayout = new QGridLayout(centralwidget);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        splitter_7 = new QSplitter(centralwidget);
        splitter_7->setObjectName(QString::fromUtf8("splitter_7"));
        splitter_7->setOrientation(Qt::Vertical);
        splitter_6 = new QSplitter(splitter_7);
        splitter_6->setObjectName(QString::fromUtf8("splitter_6"));
        splitter_6->setOrientation(Qt::Horizontal);
        splitter_3 = new QSplitter(splitter_6);
        splitter_3->setObjectName(QString::fromUtf8("splitter_3"));
        splitter_3->setOrientation(Qt::Vertical);
        splitter_2 = new QSplitter(splitter_3);
        splitter_2->setObjectName(QString::fromUtf8("splitter_2"));
        splitter_2->setOrientation(Qt::Horizontal);
        label = new QLabel(splitter_2);
        label->setObjectName(QString::fromUtf8("label"));
        splitter_2->addWidget(label);
        txtName = new QTextEdit(splitter_2);
        txtName->setObjectName(QString::fromUtf8("txtName"));
        txtName->setMaximumSize(QSize(16777215, 26));
        splitter_2->addWidget(txtName);
        splitter_3->addWidget(splitter_2);
        splitter = new QSplitter(splitter_3);
        splitter->setObjectName(QString::fromUtf8("splitter"));
        splitter->setOrientation(Qt::Vertical);
        lstNames = new QListView(splitter);
        lstNames->setObjectName(QString::fromUtf8("lstNames"));
        lstNames->setMaximumSize(QSize(250, 16777215));
        splitter->addWidget(lstNames);
        splitter_3->addWidget(splitter);
        splitter_6->addWidget(splitter_3);
        splitter_5 = new QSplitter(splitter_6);
        splitter_5->setObjectName(QString::fromUtf8("splitter_5"));
        splitter_5->setOrientation(Qt::Vertical);
        txtMain = new QTextEdit(splitter_5);
        txtMain->setObjectName(QString::fromUtf8("txtMain"));
        splitter_5->addWidget(txtMain);
        splitter_4 = new QSplitter(splitter_5);
        splitter_4->setObjectName(QString::fromUtf8("splitter_4"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(splitter_4->sizePolicy().hasHeightForWidth());
        splitter_4->setSizePolicy(sizePolicy);
        splitter_4->setOrientation(Qt::Horizontal);
        txtMsg = new QTextEdit(splitter_4);
        txtMsg->setObjectName(QString::fromUtf8("txtMsg"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(txtMsg->sizePolicy().hasHeightForWidth());
        txtMsg->setSizePolicy(sizePolicy1);
        txtMsg->setMaximumSize(QSize(16777215, 26));
        splitter_4->addWidget(txtMsg);
        btnSend = new QPushButton(splitter_4);
        btnSend->setObjectName(QString::fromUtf8("btnSend"));
        splitter_4->addWidget(btnSend);
        splitter_5->addWidget(splitter_4);
        splitter_6->addWidget(splitter_5);
        splitter_7->addWidget(splitter_6);
        gfxDrawan = new QGraphicsView(splitter_7);
        gfxDrawan->setObjectName(QString::fromUtf8("gfxDrawan"));
        sizePolicy1.setHeightForWidth(gfxDrawan->sizePolicy().hasHeightForWidth());
        gfxDrawan->setSizePolicy(sizePolicy1);
        splitter_7->addWidget(gfxDrawan);

        gridLayout->addWidget(splitter_7, 0, 0, 1, 1);

        MainWindow->setCentralWidget(centralwidget);
#ifndef QT_NO_SHORTCUT
        label->setBuddy(txtName);
#endif // QT_NO_SHORTCUT
        QWidget::setTabOrder(txtName, txtMain);
        QWidget::setTabOrder(txtMain, lstNames);
        QWidget::setTabOrder(lstNames, txtMsg);
        QWidget::setTabOrder(txtMsg, btnSend);
        QWidget::setTabOrder(btnSend, gfxDrawan);

        retranslateUi(MainWindow);
        QObject::connect(btnSend, SIGNAL(clicked()), MainWindow, SLOT(btnSendHandler()));

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("MainWindow", "Name:", 0, QApplication::UnicodeUTF8));
        btnSend->setText(QApplication::translate("MainWindow", "Send", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
