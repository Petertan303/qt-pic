#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "starPromptWindow.h"
#include "imageWindow.h"
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
#include <QWebSocket>
#include <QUrl>

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
    enum ConnectionMode {
        HTTP,
        WebSocket
    };

signals:
    void sendImageToImageWindow(const QByteArray &image);  // 定义信号，用于发送 QImage
    // void addPromptToStarSignal(QString prompt, QString negativePrompt, QString title);

private slots:
    void loadPrompt(QString prompt, QString negativePrompt, QString title);
    void showImage(const QByteArray &imageData);
    void showImage(const QImage &imageData);
    void draw(QString prompt, QString negativePrompt, QString key);
    void onNetworkReply(QNetworkReply* reply);
    void onClearButtonClicked();
    // void sendAddPromptToStarSignal(QString prompt, QString negativePrompt, QString title);

private:
    Ui::MainWindow *ui;
    // imageWindow *m_imageWindow;  // 添加 imageWindow 作为成员变量
    imageWindow *m_imageWindow;  // 添加 imageWindow 作为成员变量
    starPromptWindow *m_starPromptWindow;  // StarPromptWindow 的实例
    QNetworkAccessManager *networkManager;
    ConnectionMode m_currentMode = HTTP;
    // QLabel *statusLabel;  // 显示请求状态的标签
    void showStarPrompts();
    void saveImageAndJson(const QByteArray &imageData, const QJsonObject &jsonResponse);
    void showError(const QString &errorString);
    void recoverHistory();
    void saveHistory();
    std::tuple<QNetworkRequest, QByteArray> readApiData(QString prompt, QString negativePrompt);
    void addMenuBar();
    void connectWebSocket();
    QUrl ws_url;
    QWebSocket *m_webSocket;
    void onViewResponse(QNetworkReply* reply);
    void viewImage(const QString& filename, const QString& subfolder);
    void handleExecutionComplete(const QJsonObject& outputs);
    void subscribeTask(const QString & taskId);
    void onWorkflowResponse(QNetworkReply* reply);
    void sendWorkflow(const QJsonObject& workflowData);
    void onWebSocketTextMessageReceived(QString message);
    void onWebSocketError(QAbstractSocket::SocketError error);
    void onWebSocketDisconnected();
    void onWebSocketConnected();
    QNetworkAccessManager *view_manager;
    void sendRequestComfyUI(const QString& prompt, const QString& negativePrompt);
    void wheelEvent(QLabel *imageLabel, QWheelEvent *event);
    void mouseMoveEvent(QLabel *imageLabel, QPoint *mousePos, QMouseEvent *event);
    void contextMenuEvent(QContextMenuEvent *event, QImage image);

};

#endif // MAINWINDOW_H
