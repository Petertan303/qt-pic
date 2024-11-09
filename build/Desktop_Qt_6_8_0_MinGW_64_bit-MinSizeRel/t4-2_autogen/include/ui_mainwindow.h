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
    QPushButton *ClearButton;

    void setupUi(QWidget *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->setWindowModality(Qt::WindowModality::WindowModal);
        MainWindow->resize(800, 600);
        sendRequestButton = new QPushButton(MainWindow);
        sendRequestButton->setObjectName("sendRequestButton");
        sendRequestButton->setGeometry(QRect(20, 460, 171, 121));
        QFont font;
        font.setPointSize(14);
        sendRequestButton->setFont(font);
        outputTextEdit = new QTextEdit(MainWindow);
        outputTextEdit->setObjectName("outputTextEdit");
        outputTextEdit->setGeometry(QRect(330, 20, 451, 561));
        QFont font1;
        font1.setFamilies({QString::fromUtf8("\345\276\256\350\275\257\351\233\205\351\273\221")});
        font1.setPointSize(14);
        outputTextEdit->setFont(font1);
        promptTextEdit = new QTextEdit(MainWindow);
        promptTextEdit->setObjectName("promptTextEdit");
        promptTextEdit->setGeometry(QRect(20, 20, 291, 291));
        QFont font2;
        font2.setFamilies({QString::fromUtf8("\345\276\256\350\275\257\351\233\205\351\273\221")});
        font2.setPointSize(12);
        promptTextEdit->setFont(font2);
        negativePromptTextEdit = new QTextEdit(MainWindow);
        negativePromptTextEdit->setObjectName("negativePromptTextEdit");
        negativePromptTextEdit->setGeometry(QRect(20, 330, 291, 111));
        negativePromptTextEdit->setFont(font2);
        ClearButton = new QPushButton(MainWindow);
        ClearButton->setObjectName("ClearButton");
        ClearButton->setGeometry(QRect(210, 460, 101, 121));
        ClearButton->setFont(font);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QWidget *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "Draw Picture", nullptr));
        sendRequestButton->setText(QCoreApplication::translate("MainWindow", "send request", nullptr));
        ClearButton->setText(QCoreApplication::translate("MainWindow", "clear", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
