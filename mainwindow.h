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

    #define CONNECTION_MODE_MAP(XX) \
        XX(HTTP) \
            XX(WebSocket) \
            XX(SomeOtherMode) \

            enum ConnectionMode {
    #define XX(name) name,
                CONNECTION_MODE_MAP(XX)
    #undef XX
                UnknownMode // 添加一个未知模式的默认值
    };
    // enum ConnectionMode {
    //     HTTP,
    //     WebSocket
    // };

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
    void handleWebSocketError(QAbstractSocket::SocketError error);

private:
    Ui::MainWindow *ui;
    // imageWindow *m_imageWindow;  // 添加 imageWindow 作为成员变量
    starPromptWindow *m_starPromptWindow;  // StarPromptWindow 的实例
    QNetworkAccessManager *networkManager;
    ConnectionMode m_currentMode;
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
    QString m_currentTaskId;
    QString taskId;
    void onViewResponse(QNetworkReply* reply);
    void viewImage(const QString& filename, const QString& subfolder);
    void handleExecutionComplete(const QJsonObject& outputs);
    void subscribeTask(const QString & taskId);
    void onWorkflowResponse(QNetworkReply* reply);
    void sendWorkflow(const QJsonObject& workflowData);
    void onWebSocketTextMessageReceived(const QString &message);
    void onWebSocketError(QAbstractSocket::SocketError error);
    void onWebSocketDisconnected();
    void onWebSocketConnected();
    QNetworkAccessManager *view_manager;
    void sendRequestComfyUI(const QString& prompt, const QString& negativePrompt, const QString& key);
    void contextMenuEvent(QContextMenuEvent *event, QImage image);
    void sendImage(QByteArray &);
    void sendSubscribeMessage();
    void initAll();
    void initConnection();

    // 从枚举类型到字符串
    // QMap<QString, ConnectionMode> mapModeToString() {
    //     QMap<QString, ConnectionMode> map;
    // #define XX(name) map[#name] = ConnectionMode::name;
    //         CONNECTION_MODE_MAP(XX)
    // #undef XX
    //         return map;
    // }

    QString mapModeToString(ConnectionMode mode) {
        static QMap<ConnectionMode, QString> modeToStringMap = [] {
            QMap<ConnectionMode, QString> map;
#define XX(name) map[ConnectionMode::name] = #name;
            CONNECTION_MODE_MAP(XX)
#undef XX
            return map;
        }();

        if (modeToStringMap.contains(mode)) {
            return modeToStringMap[mode];
        }
        return "UnknownMode";
    }

    // 从字符串到枚举类型
    ConnectionMode mapStringToMode(const QString& modeString) {
        static QMap<QString, ConnectionMode> stringToModeMap = [] {
            QMap<QString, ConnectionMode> map;
#define XX(name) map[#name] = ConnectionMode::name;
            CONNECTION_MODE_MAP(XX)
#undef XX
            return map;
        }();

        if (stringToModeMap.contains(modeString)) {
            return stringToModeMap[modeString];
        }
        return ConnectionMode::UnknownMode;
    }
};

#endif // MAINWINDOW_H
