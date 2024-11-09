#ifndef ERRORDIALOG_H
#define ERRORDIALOG_H

#include <QDialog>

namespace Ui {
class ErrorDialog;  // 前向声明Ui类
}

class ErrorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ErrorDialog(QWidget *parent = nullptr);  // 构造函数
    ~ErrorDialog();  // 析构函数

    // 设置错误信息的方法
    void setErrorMessage(const QString &message);

private slots:
    void onCopyButtonClicked();  // 复制按钮点击槽函数
    void onCloseButtonClicked(); // 关闭按钮点击槽函数

private:
    Ui::ErrorDialog *ui;  // UI 类指针，连接到 error.ui 文件
};

#endif // ERRORDIALOG_H
