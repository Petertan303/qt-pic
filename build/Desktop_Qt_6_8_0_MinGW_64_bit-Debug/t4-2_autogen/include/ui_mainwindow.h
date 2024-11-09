/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.8.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QPushButton *sendRequestButton;
    QTextEdit *outputTextEdit;
    QTextEdit *promptTextEdit;
    QTextEdit *negativePromptTextEdit;

    void setupUi(QWidget *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->setWindowModality(Qt::WindowModality::WindowModal);
        MainWindow->resize(800, 600);
        sendRequestButton = new QPushButton(MainWindow);
        sendRequestButton->setObjectName("sendRequestButton");
        sendRequestButton->setGeometry(QRect(20, 460, 291, 121));
        QFont font;
        font.setPointSize(14);
        sendRequestButton->setFont(font);
        outputTextEdit = new QTextEdit(MainWindow);
        outputTextEdit->setObjectName("outputTextEdit");
        outputTextEdit->setGeometry(QRect(330, 20, 441, 561));
        outputTextEdit->setFont(font);
        promptTextEdit = new QTextEdit(MainWindow);
        promptTextEdit->setObjectName("promptTextEdit");
        promptTextEdit->setGeometry(QRect(20, 20, 291, 291));
        QFont font1;
        font1.setPointSize(10);
        promptTextEdit->setFont(font1);
        negativePromptTextEdit = new QTextEdit(MainWindow);
        negativePromptTextEdit->setObjectName("negativePromptTextEdit");
        negativePromptTextEdit->setGeometry(QRect(20, 330, 291, 111));
        negativePromptTextEdit->setFont(font1);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QWidget *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "Widget", nullptr));
        sendRequestButton->setText(QCoreApplication::translate("MainWindow", "send request", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
