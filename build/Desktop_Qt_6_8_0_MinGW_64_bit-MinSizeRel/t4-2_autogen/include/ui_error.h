/********************************************************************************
** Form generated from reading UI file 'error.ui'
**
** Created by: Qt User Interface Compiler version 6.8.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ERROR_H
#define UI_ERROR_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>

QT_BEGIN_NAMESPACE

class Ui_ErrorDialog
{
public:
    QLabel *labelError;
    QPushButton *copyButton;
    QPushButton *closeButton;

    void setupUi(QDialog *ErrorDialog)
    {
        if (ErrorDialog->objectName().isEmpty())
            ErrorDialog->setObjectName("ErrorDialog");
        ErrorDialog->resize(245, 178);
        labelError = new QLabel(ErrorDialog);
        labelError->setObjectName("labelError");
        labelError->setGeometry(QRect(10, 20, 221, 81));
        QFont font;
        font.setPointSize(10);
        labelError->setFont(font);
        labelError->setAlignment(Qt::AlignmentFlag::AlignCenter);
        labelError->setWordWrap(true);
        copyButton = new QPushButton(ErrorDialog);
        copyButton->setObjectName("copyButton");
        copyButton->setGeometry(QRect(10, 110, 101, 51));
        closeButton = new QPushButton(ErrorDialog);
        closeButton->setObjectName("closeButton");
        closeButton->setGeometry(QRect(120, 110, 111, 51));

        retranslateUi(ErrorDialog);

        QMetaObject::connectSlotsByName(ErrorDialog);
    } // setupUi

    void retranslateUi(QDialog *ErrorDialog)
    {
        ErrorDialog->setWindowTitle(QCoreApplication::translate("ErrorDialog", "Dialog", nullptr));
        labelError->setText(QCoreApplication::translate("ErrorDialog", "TextLabel", nullptr));
        copyButton->setText(QCoreApplication::translate("ErrorDialog", "copy", nullptr));
        closeButton->setText(QCoreApplication::translate("ErrorDialog", "close", nullptr));
    } // retranslateUi

};

namespace Ui {
    class ErrorDialog: public Ui_ErrorDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ERROR_H
