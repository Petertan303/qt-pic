#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "starPromptWindow.h"
// #include "imageWindow.h"
#include <QRandomGenerator>
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
#include <QSet>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
// class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    enum ConnectionMode { Default, WebUI, ComfyUI, Unknown};
    ~MainWindow();

private slots:

private:
    // ui
    Ui::MainWindow *ui;
    // 模式 如 sd comfyui
    ConnectionMode m_currentMode;
    // 模式对应的需要发送的数据 和 request 实例
    QHash<MainWindow::ConnectionMode, QNetworkRequest> requestMap;
    QHash<MainWindow::ConnectionMode, QByteArray> postDataMap;
    // 计数
    qint16 count;
    // 模式 和 枚举 之间的对应
    QHash<QString, MainWindow::ConnectionMode> stringToMode;
    QHash<MainWindow::ConnectionMode, QString> modeToString;

    // 初始化
    void initAll();
    void initConnection(); // 建立websocket链接
    void addMenuBar(); // 菜单

    // 保存prompt历史，恢复prompt历史
    void saveHistory();
    void recoverHistory();

    // error窗口
    void showError(const QString &errorString);
    // 将图片和prompt信息保存到本地
    void saveImageAndJson(const QByteArray &imageData, const QJsonObject &jsonResponse);
    // std::tuple<QHash<MainWindow::ConnectionMode, QNetworkRequest>, QHash<MainWindow::ConnectionMode, QByteArray>> readApiData();
    // 读取本地的文件 中的api数据
    void readApiData();

    // 星标窗口 实例
    starPromptWindow *m_starPromptWindow;
    void showStarPrompts();

    // 图片窗口
    void sendImage(QByteArray &);

    // http 的 QNetworkAccessManager, 通用的
    QNetworkAccessManager *m_httpNetworkManager;
    // comfyui 独立用一个S
    QNetworkAccessManager *m_httpNetworkManagerComfyUI;
    // websocket，comfyUI 用的
    QWebSocket *m_webSocket; // 用于获取生成完成消息

    // prompt id 队列，使用集合 set
    QSet<QString> promptIdSet;

    // 本机的id（comfyui特色）
    QString clientId;
    //
    QString m_currentPromptId;

    // 预览生成的图片
    void viewImage(const QString& filename, const QString& subfolder);

    // websocket建立连接
    void connecWebSocket();
    // 断连 试图重连
    void onComfyUIDisconnected();
    // 连接 显示“已连接”字样
    void onComfyUIConnected();
    // websocket 响应 会返回图片生成成功
    void onWebSocketMessageReceived(const QString &message);
    // 构建 comfyUI 使用的 body
    void generateRequestComfyUI(const QString& prompt, const QString& negativePrompt, const QString& key);
    // 不过目前打算还是直接用固定的 body
    // 后续可能会增加自己连接
    // 感觉真要这么搞的话工作量会很大啊

    // 初始化时 加载prompt
    void loadPrompt(QString prompt, QString negativePrompt, QString title);
    // 启动绘画进程 和按钮绑定
    void beginDrawProcess(QString prompt, QString negativePrompt, QString title);
    // 收到响应
    void onNetworkReply(QString title, QString prompt, QString negativePrompt, QNetworkReply* reply);
    // 收到响应
    void onNetworkReplyComfyUI(QString title, QString prompt, QString negativePrompt, QNetworkReply* reply);
    // 清理窗口中的日志
    void onClearButtonClicked();
    // 设置窗口
    void showConfigDialog();
    // 当前的配置
    QJsonObject m_currentConfig;
    // 获取当前设置 json
    QJsonObject getCurrentConfig() const;


    void saveJsonData(QString jsonString){
        QString filePath = "./postData.json";
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qWarning() << "无法打开文件进行写入:" << filePath;
            return;
        }

        QTextStream out(&file);
        out << jsonString;
        file.close();
    }

    QByteArray setPromptComfyUI(QByteArray postData, QString prompt, QString negative_prompt){
        QJsonDocument jsonDoc = QJsonDocument::fromJson(postData);
        QJsonObject jsonObj = jsonDoc.object();
        QJsonObject promptObj = jsonObj["prompt"].toObject();

        QJsonObject threeObj = promptObj["3"].toObject();
        QJsonObject sixObj = promptObj["6"].toObject();
        QJsonObject sevenObj = promptObj["7"].toObject();

        QJsonObject seedObj = threeObj["inputs"].toObject();
        QJsonObject inputsObj = sixObj["inputs"].toObject();
        QJsonObject negativeObj = sevenObj["inputs"].toObject();

        if(seedObj["seed"]==""){
            seedObj["seed"] = QRandomGenerator::global()->bounded(999999999);
        }
        inputsObj["text"] = prompt;
        negativeObj["text"] = negative_prompt;

        threeObj["inputs"] = seedObj;
        sixObj["inputs"] = inputsObj;
        sevenObj["inputs"] = negativeObj;

        promptObj["3"] = threeObj;
        promptObj["6"] = sixObj;
        promptObj["7"] = sevenObj;

        jsonObj["prompt"] = promptObj;
        jsonDoc = QJsonDocument(jsonObj);
        return jsonDoc.toJson();
    }

    QByteArray setPromptWebUI(QByteArray postData, QString prompt, QString negative_prompt){
        QJsonDocument jsonDoc = QJsonDocument::fromJson(postData);
        QJsonObject jsonObj = jsonDoc.object();
        jsonObj["prompt"] = prompt;
        jsonObj["negative_prompt"] = negative_prompt;
        if(jsonObj["seed"] == ""){
            jsonObj["seed"] = QRandomGenerator::global()->bounded(999999999);
        }
        jsonDoc = QJsonDocument(jsonObj);
        return jsonDoc.toJson();
    }

    void handleResponse(QNetworkReply *reply) {
        qDebug() << "\n=== Response Received ===";

        // 打印响应头
        qDebug() << "Headers:";
        QList<QByteArray> headers = reply->rawHeaderList();
        for (const QByteArray &header : headers) {
            qDebug() << header << ":" << reply->rawHeader(header);
        }

        // 打印响应内容
        qDebug() << "\nBody:";
        QByteArray response = reply->readAll();
        qDebug() << response;

        // 打印状态码
        qDebug() << "\nStatus Code:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    }
};

#endif // MAINWINDOW_H
