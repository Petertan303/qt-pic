#include "errordialog.h"
#include "ui_error.h"
#include <QClipboard>  // 用于操作剪贴板
#include <QApplication>  // 用于获取应用程序的实例
#include <QMessageBox>

ErrorDialog::ErrorDialog(QWidget *parent) :
    QDialog(parent), ui(new Ui::ErrorDialog)
{
    ui->setupUi(this);
    connect(ui->copyButton, &QPushButton::clicked, this, &ErrorDialog::onCopyButtonClicked);
    connect(ui->closeButton, &QPushButton::clicked, this, &ErrorDialog::onCloseButtonClicked);
}

ErrorDialog::~ErrorDialog()
{
    delete ui;  // 清理资源
}

void ErrorDialog::setErrorMessage(const QString &message)
{
    ui->labelError->setText(message);  // 设置错误信息到labelError控件
}

void ErrorDialog::onCopyButtonClicked()
{
    // 获取错误信息
    QString errorMessage = ui->labelError->text();

    // 获取剪贴板并将错误信息复制到剪贴板
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(errorMessage);  // 将文本设置到剪贴板

    // 可选：弹出提示消息，告知用户已复制到剪贴板
    QMessageBox::information(this, "Copied!", "Error message Copied!");
}

void ErrorDialog::onCloseButtonClicked()
{
    this->close();  // 关闭对话框
}
