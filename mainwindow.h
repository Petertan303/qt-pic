#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QTextStream>
#include <QLabel>
#include <QDialog>
#include <QImage>
#include <QPixmap>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSendRequestButtonClicked();
    void onNetworkReply(QNetworkReply* reply);
    void onClearButtonClicked();

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManager;
    // QLabel *statusLabel;  // 显示请求状态的标签

    void saveImageAndJson(const QByteArray &imageData, const QJsonObject &jsonResponse);
    void showImage(const QByteArray &imageData);
    void showError(const QString &errorString);
    void recoverHistory();
    QNetworkRequest readApiData();
};

#endif // MAINWINDOW_H
