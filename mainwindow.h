#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "starPromptWindow.h"
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
#include <QMenuBar>
#include <QMenu>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
// class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void addPromptToStarSignal(QString prompt, QString negativePrompt, QString title);

private slots:
    void loadPrompt(QString prompt, QString negativePrompt, QString title);
    void showImage(const QByteArray &imageData);
    void draw(QString prompt, QString negativePrompt, QString key);
    void onNetworkReply(QNetworkReply* reply);
    void onClearButtonClicked();
    void sendAddPromptToStarSignal(QString prompt, QString negativePrompt, QString title);

private:
    Ui::MainWindow *ui;
    starPromptWindow *m_starPromptWindow;  // StarPromptWindow 的实例
    QNetworkAccessManager *networkManager;
    // QLabel *statusLabel;  // 显示请求状态的标签
    void showStarPrompts();
    void saveImageAndJson(const QByteArray &imageData, const QJsonObject &jsonResponse);
    void showError(const QString &errorString);
    void recoverHistory();
    void saveHistory();
    std::tuple<QNetworkRequest, QByteArray> readApiData(QString prompt, QString negativePrompt);
    void addMenuBar();
};

#endif // MAINWINDOW_H
