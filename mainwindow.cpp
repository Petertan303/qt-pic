#include "globals.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
// #include "ui_error.h"
#include "errordialog.h"
// #include <QErrorMessage>
#include "imageWindow.h"
#include "starPromptWindow.h"
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>
#include <QFile>
#include <QImage>
#include <QImageWriter>
#include <QDebug>
#include <QTextStream>
#include <QDialog>
#include <QLabel>
#include <QJsonArray>
#include <QShortcut>
#include <QVBoxLayout>
#include <QKeySequence>
#include <QDir>
#include <QMessageBox>
#include <QWidget>
#include <QLineEdit>
#include <QWebSocket>
#include <QWebsocketServer>
#include <QFileDialog>
#include <QActionGroup>
#include <QUrlQuery>
#include "configDialog.h"

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent)
{
    initAll();
    initConnection();
}

MainWindow::~MainWindow()
{
    if(m_webSocket) {
        m_webSocket->close();
        delete m_webSocket;
    }
    delete ui;
    delete m_starPromptWindow;
}

// 初始化
void MainWindow::initAll(){
    count = 0;
    ui = new Ui::MainWindow;
    stringToMode = {
        {"Default", ConnectionMode::Default},
        {"WebUI", ConnectionMode::WebUI},
        {"ComfyUI", ConnectionMode::ComfyUI},
        {"Unknown", ConnectionMode::Unknown}
    };
    for (auto it = stringToMode.begin(); it != stringToMode.end(); ++it) {
        modeToString.insert(it.value(), it.key());
    }
    m_starPromptWindow = new starPromptWindow(this);
    m_httpNetworkManager = new QNetworkAccessManager(this);
    m_httpNetworkManagerComfyUI = new QNetworkAccessManager(this);
    ui->setupUi(this);
    m_webSocket = nullptr; // 初始化为空
    // m_currentTaskId = "";
    // auto [requestMap, postDataMap] = readApiData();
    readApiData();
    recoverHistory();
    addMenuBar();
    ui->outputTextEdit->append(QStringLiteral("启动！"));
}


// 读取本地api数据
// std::tuple<QHash<MainWindow::ConnectionMode, QNetworkRequest>, QHash<MainWindow::ConnectionMode, QByteArray>> MainWindow::readApiData(){
void MainWindow::readApiData(){
    QFile ApiJson("./dataFile/ApiData.json");
    ApiJson.open(QIODevice::ReadOnly);
    QByteArray jsonData = ApiJson.readAll();
    ApiJson.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    QJsonObject jsonObject = jsonDoc.object();
    QJsonArray apiArray = jsonObject["data"].toArray();

    foreach (const QJsonValue &value, apiArray) {
        if (value.isObject()) {
            QNetworkRequest request;
            QUrl url;
            url.setUrl(value["url"].toString());
            request.setUrl(url);
            qDebug() << "url: " << url.url();
            QJsonObject jsonHeader = value["header"].toObject();
            QJsonObject jsonBody = value["body"].toObject();
            // request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");
            for (auto it = jsonHeader.constBegin(); it != jsonHeader.constEnd(); ++it){
                if(it.key() != "url")
                    request.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());
            }

            QByteArray postData;
            if (request.header(QNetworkRequest::ContentTypeHeader).toByteArray() == "application/json"){
                qDebug() << "application/json";
                QJsonDocument jsonDoc(jsonBody);
                postData = jsonDoc.toJson(QJsonDocument::Compact);
                // saveJsonData(postData);
                if (value["name"] == "ComfyUI"){
                    clientId = jsonBody["client_id"].toString();
                    qDebug() << clientId;
                }
            }
            else {
                for (auto it = jsonBody.constBegin(); it != jsonBody.constEnd(); ++it){
                    QString stringPost = it.key().toUtf8() + "=" + it.value().toString().toUtf8() + "&";
                    postData.append(stringPost.toUtf8());
                }
            }
            requestMap.insert(stringToMode[value["name"].toString()], request);
            postDataMap.insert(stringToMode[value["name"].toString()], postData);
            // if(value["name"]=="WebUI"){
            //     qDebug() << value["body"];
            // }
        }
    }

    // QJsonDocument doc;
    // doc.setArray(apiArray);
    // saveJsonData(doc.toJson());


    // qDebug() << postDataMap;

    return;
}

// 初始化，连接按钮、方法，或者说信号、槽
void MainWindow::initConnection(){
    QShortcut *shortcutDraw = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_D), this);
    connect(shortcutDraw, &QShortcut::activated, ui->sendRequestButton, &QPushButton::click);
    QShortcut *shortcutSaveHistory = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), this);
    connect(ui->ClearButton, &QPushButton::clicked, this, &MainWindow::onClearButtonClicked);

    connect(shortcutSaveHistory, &QShortcut::activated, this, &MainWindow::saveHistory);
    // connect(ui->ClearButton, &QPushButton::clicked, this, &ui->outputTextEdit->clear);

    // connect(ui->sendRequestButton, &QPushButton::clicked, this, [this]() {
    //     MainWindow::draw(ui->promptTextEdit->toPlainText(),
    //                      ui->negativePromptTextEdit->toPlainText(),
    //                      ui->titleLineEdit->text());
    // });
    connect(ui->sendRequestButton, &QPushButton::clicked, this, [this]() {
        switch(m_currentMode){
            case ConnectionMode::Default:
            case ConnectionMode::WebUI:
            case ConnectionMode::ComfyUI:
                MainWindow::beginDrawProcess(ui->promptTextEdit->toPlainText(),
                                 ui->negativePromptTextEdit->toPlainText(),
                                 ui->titleLineEdit->text());
                break;
            case ConnectionMode::Unknown:
                ui->outputTextEdit->append("未能成功连接。");
                break;
            // case:break;
        }
    });
    // connect(m_httpNetworkManager,
    //         &QNetworkAccessManager::finished,
    //         this,
    //         [=]() {
    //         MainWindow::onNetworkReply(ui->titleLineEdit->text(),
    //                                     ui->promptTextEdit->toPlainText(),
    //                                     ui->negativePromptTextEdit->toPlainText(),
    //                                     reply);
    //         reply->deleteLater();
    // });

    // connect(m_httpNetworkManagerComfyUI,
    //         &QNetworkAccessManager::finished,
    //         this,
    //         [=]() {
    //             MainWindow::onNetworkReplyComfyUI(ui->titleLineEdit->text(),
    //                                        ui->promptTextEdit->toPlainText(),
    //                                        ui->negativePromptTextEdit->toPlainText(),
    //                                        reply);
    //         });

    connect(m_starPromptWindow,
            &starPromptWindow::drawSignal,
            this, // 方括号：捕获的变量；圆括号：传递的变量或者说参数
            [this](QString prompt, QString negativePrompt, QString key) {
                switch(m_currentMode){
                case ConnectionMode::Default:
                case ConnectionMode::WebUI:
                    MainWindow::beginDrawProcess(prompt,
                                     negativePrompt,
                                     key);
                    break;
                case ConnectionMode::ComfyUI:
                    MainWindow::beginDrawProcess(prompt,
                                     negativePrompt,
                                     key);
                    MainWindow::generateRequestComfyUI(prompt,
                                                   negativePrompt,
                                                   key);
                    break;
                case ConnectionMode::Unknown:
                    ui->outputTextEdit->append("未能成功连接。");
                    break;
                    // case:break;
                }
            });
    connect(m_starPromptWindow,
            &starPromptWindow::loadPromptSignal,
            this,
            &MainWindow::loadPrompt);
}

// 切换到ComfyUI模式的时候启动
void MainWindow::connecWebSocket() {
    // 清理旧的 ComfyUI 连接
    if (m_webSocket) {
        m_webSocket->close(); // 关闭连接
        m_webSocket->deleteLater(); // 延迟删除对象
        m_webSocket = nullptr; // 将指针置为 nullptr
    }
    m_webSocket = new QWebSocket();
    connect(m_webSocket, &QWebSocket::connected, this, [this]() {
        ui->outputTextEdit->append(QStringLiteral("ComfyUI已连接"));
    });
    connect(m_webSocket, &QWebSocket::disconnected, this, [this]() {
        ui->outputTextEdit->append(QStringLiteral("ComfyUI已断开"));
    });
    connect(m_webSocket,
            &QWebSocket::textMessageReceived,
            this,
            &MainWindow::onWebSocketMessageReceived);

    // connect(m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred),
    //         this, &MainWindow::handleComfyUIError);

    QUrl ws_url = requestMap[m_currentMode].url();
    ws_url.setScheme("ws");
    ws_url.setPath("/ws");
    ws_url.setQuery("clientId=" + clientId);
    qDebug() << ws_url.toString();
    m_webSocket->open(ws_url);
    // m_ComfyUI->open(QUrl(ws_url.path() + "/ws"));
}

// 设置菜单栏
void MainWindow::addMenuBar(){
    QFont ft;
    ft.setPointSize(12);
    QMenuBar *menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    // Star Menu
    QMenu *fileMenu = menuBar->addMenu("&Star");
    fileMenu->setFont(ft);
    QAction *starPromptAction = new QAction("&Star Prompt", this);
    QAction *addToStarAction = new QAction("&Add to Star", this);
    starPromptAction->setFont(ft);
    addToStarAction->setFont(ft);

    connect(starPromptAction, &QAction::triggered, this, &MainWindow::showStarPrompts);
    connect(addToStarAction, &QAction::triggered, this, [this]() {
        m_starPromptWindow->addPromptToStar(ui->promptTextEdit->toPlainText(),
                                            ui->negativePromptTextEdit->toPlainText(),
                                            ui->titleLineEdit->text());
    });
    fileMenu->addAction(starPromptAction);
    fileMenu->addAction(addToStarAction);

    // Mode Menu
    QMenu *modeMenu = menuBar->addMenu("&Mode");
    modeMenu->setFont(ft);
    QActionGroup *modeGroup = new QActionGroup(this);

    QAction *defaultAction = new QAction("Default", this);
    QAction *webUIAction = new QAction("WebUI", this);
    QAction *comfyUIAction = new QAction("ComfyUI", this);

    defaultAction->setCheckable(true);
    webUIAction->setCheckable(true);
    comfyUIAction->setCheckable(true);

    // 保证只能选择一个
    modeGroup->addAction(defaultAction);
    modeGroup->addAction(webUIAction);
    modeGroup->addAction(comfyUIAction);

    modeMenu->addAction(defaultAction);
    modeMenu->addAction(webUIAction);
    modeMenu->addAction(comfyUIAction);

    connect(defaultAction, &QAction::triggered, this, [this]() {
        m_currentMode = ConnectionMode::Default;
        ui->outputTextEdit->append("");
        ui->outputTextEdit->append(QStringLiteral("<strong>切换到Default模式。</strong>"));
    });
    connect(webUIAction, &QAction::triggered, this, [this]() {
        m_currentMode = ConnectionMode::WebUI;
        ui->outputTextEdit->append("");
        ui->outputTextEdit->append(QStringLiteral("<strong>切换到WebUI模式</strong>"));
    });
    connect(comfyUIAction, &QAction::triggered, this, [this]() {
        m_currentMode = ConnectionMode::ComfyUI;
        connecWebSocket();
        ui->outputTextEdit->append("");
        ui->outputTextEdit->append(QStringLiteral("<strong>切换到ComfyUI模式</strong>"));
    });

    switch(m_currentMode){
    case ConnectionMode::Default:
        defaultAction->setChecked(true);
        break;
    case ConnectionMode::WebUI:
        webUIAction->setChecked(true);
        break;
    case ConnectionMode::ComfyUI:
        if(!m_webSocket) connecWebSocket();
        comfyUIAction->setChecked(true);
        break;
    case ConnectionMode::Unknown:
        m_currentMode = ConnectionMode::Default;
        defaultAction->setChecked(true);
        break;
        // case:break;
    }

    QMenu *configMenu = menuBar->addMenu("&Config");
    configMenu->setFont(ft);
    QActionGroup *configGroup = new QActionGroup(this);
    QAction *configAction = new QAction("Config", this);
    connect(configAction, &QAction::triggered, this, &MainWindow::showConfigDialog);
    configGroup->addAction(configAction);
    configMenu->addAction(configAction);

}

// 保存最后一次绘画的prompt
void MainWindow::saveHistory(){
    QJsonObject promptJson;
    promptJson["mode"] = modeToString[m_currentMode];
    promptJson["title"] = ui->titleLineEdit->text();
    promptJson["prompt"] = ui->promptTextEdit->toPlainText();
    promptJson["negativePrompt"] = ui->negativePromptTextEdit->toPlainText();
    QFile historyJson("./dataFile/history");
    if (historyJson.open(QIODevice::WriteOnly)) {
        QTextStream out(&historyJson);
        out << QJsonDocument(promptJson).toJson();
        historyJson.close();
    }
}

// 启动时初始化
void MainWindow::recoverHistory(){
    QFile historyJson("./dataFile/history");
    historyJson.open(QIODevice::ReadOnly);
    QByteArray jsonData = historyJson.readAll();
    historyJson.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    QJsonObject jsonObject = jsonDoc.object();

    m_currentMode = stringToMode[jsonObject["mode"].toString()];
    ui->titleLineEdit->setText(jsonObject["title"].toString());
    ui->promptTextEdit->setPlainText(jsonObject["prompt"].toString());
    ui->negativePromptTextEdit->setPlainText(jsonObject["negativePrompt"].toString());
}

// 清理日志窗口
void MainWindow::onClearButtonClicked(){
    ui->outputTextEdit->clear();
}

// 加载prompt到文本框
void MainWindow::loadPrompt(QString prompt, QString negativePrompt, QString title){
    ui->promptTextEdit->setPlainText(prompt);
    ui->negativePromptTextEdit->setPlainText(negativePrompt);
    ui->titleLineEdit->setText(title);

    qDebug() << "Updated key:" << title;
    qDebug() << "Negative Prompt:" << negativePrompt;
    qDebug() << "Prompt:" << prompt;
}

// 发送http的请求，无论是webui还是comfyui
// url不同，body不同，这两个在本地文件里表示了
// 注意 comfyUI 的body是 prompt 和 clientId，而不是只有 prompt部分
void MainWindow::beginDrawProcess(QString prompt, QString negativePrompt, QString key)
{
    if(count==0){
        ui->outputTextEdit->append("");
        if (key.isEmpty())
            ui->outputTextEdit->append(QStringLiteral("<strong>开始画图...</strong>"));
        else
            ui->outputTextEdit->append(QStringLiteral("<strong>开始绘制</strong>")\
                                           + key\
                                           + "<strong>...</strong>");
    }
    else{
        ui->outputTextEdit->append(QStringLiteral("有<strong> ")\
                                       + QString::number(count)\
                                       + QStringLiteral(" </strong>张在等待..."));
    }
    count ++;

    QByteArray postData = postDataMap[m_currentMode];
    QNetworkRequest request = requestMap[m_currentMode];

    // qDebug() << postData;

    switch (m_currentMode) {
    case Default:
        postData.append("prompt=" + QUrl::toPercentEncoding(prompt) + "&");
        postData.append("negative_prompt=, " + QUrl::toPercentEncoding(negativePrompt));
        break;
    case WebUI:
        postData = setPromptWebUI(postData, prompt, negativePrompt);
        break;
    case ComfyUI:
        postData = setPromptComfyUI(postData, prompt, negativePrompt);
        // saveJsonData(postData);
        break;
    default:
        break;
    }

    QUrl url = request.url();
    url.setPath("/prompt");
    request.setUrl(url);

    qDebug() << postData;

    QNetworkReply *reply = m_httpNetworkManager->post(request, postData);
    connect(reply, &QNetworkReply::finished, [=](){
        MainWindow::onNetworkReply(ui->titleLineEdit->text(),
                                   ui->promptTextEdit->toPlainText(),
                                   ui->negativePromptTextEdit->toPlainText(),
                                   reply);
        reply->deleteLater();
    });
}


// 收到http的响应
// 有可能是comfyUI post请求不含图片的响应，有可能是单独一个响应图片, 也有可能是含有图片的json
void MainWindow::onNetworkReply(QString title, QString prompt, QString negativePrompt, QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        ui->outputTextEdit->append("请求失败: " + reply->errorString());
        showError(reply->errorString());
        return;
    }
    QByteArray responseData = reply->readAll();

    // qDebug() << responseData;

    QJsonDocument jsonResponse;
    QJsonObject jsonObj;
    QJsonArray jsonImages;
    QList<QByteArray> images;

    switch(m_currentMode) {
    // 默认模式 测试用
    // 直接提取图片
    case ConnectionMode::Default:
        // handleResponse(reply);
        jsonResponse = QJsonDocument::fromJson(responseData);
        if (jsonResponse.isNull()) {
            ui->outputTextEdit->append("无效的 JSON 响应");
            showError("无效的 JSON 响应");
            return;
        }
        jsonObj = jsonResponse.object();
        jsonImages = jsonObj["images"].toArray();
        for (const QJsonValue &value : jsonImages) {
            if (value.isString()) {
                images.append(QByteArray::fromBase64(value.toString().toUtf8()));
            }
        }
        break;

    // comfyUI 模式
    case ConnectionMode::ComfyUI:
        qDebug() << "comfyui 接收到 http 响应";
        qDebug() << reply->header(QNetworkRequest::ContentTypeHeader).toByteArray();
        qDebug() << "Bytes available:" << reply->bytesAvailable();

        // handleResponse(reply);

        if (reply->header(QNetworkRequest::ContentTypeHeader).toByteArray().startsWith("image/")) {
            // 图片响应
            // GET /view 之后的响应
            images.append(responseData);
            // sendImage(images);
            break;
        }else if (reply->header(QNetworkRequest::ContentTypeHeader).toByteArray().startsWith("application/json")){
            // 获取图片文件名的响应
            // 也就是 GET /history/{prompt_id} 之后的响应

            // QByteArray responseData = reply->readAll();
            // 注意readall只能读一次，第二次读就是空的
            QJsonDocument jsonResponse = QJsonDocument::fromJson(responseData);

            // saveJsonData(responseData);

            if (jsonResponse.isNull()) {
                // ui->outputTextEdit->append("无效的 JSON 响应");
                // showError("无效的 JSON 响应");
                return;
            }

            qDebug() << "收到json响应";

            QJsonObject filenameJsonObj = jsonResponse.object();
            if(filenameJsonObj.keys().contains("prompt_id")){
                // 发送信号得到的
                // promptIdSet.insert(filenameJsonObj["prompt_id"].toString());
            }else {
                // 之后的
                m_currentPromptId = filenameJsonObj.keys()[0];
                qDebug() << m_currentPromptId;
                QJsonArray filenameJsonArray = filenameJsonObj[m_currentPromptId].toObject()
                                                   ["outputs"].toObject().begin().value().toObject()
                                                           ["images"].toArray();

                // qDebug() << filenameJsonArray;

                // 处理每个图像对象
                for (const QJsonValue &imageValue : filenameJsonArray) {
                    if (imageValue.isObject()) {
                        QJsonObject imageObj = imageValue.toObject();
                        QString filename = imageObj["filename"].toString();
                        QString subfolder = imageObj["subfolder"].toString();
                        QString type = imageObj["type"].toString();

                        QNetworkRequest request = requestMap[m_currentMode];
                        QByteArray data = postDataMap[m_currentMode];

                        QUrl url = request.url();
                        url.setPath("/view");
                        QUrlQuery urlQuery(url);
                        urlQuery.addQueryItem(QString("filename"), imageObj["filename"].toString());
                        urlQuery.addQueryItem(QString("type"), imageObj["type"].toString());
                        urlQuery.addQueryItem(QString("subfolder"), imageObj["subfolder"].toString());
                        url.setQuery(urlQuery);
                        request.setUrl(url);

                        QNetworkReply *reply = m_httpNetworkManager->get(request);
                        connect(reply, &QNetworkReply::finished, [=](){
                            MainWindow::onNetworkReply(ui->titleLineEdit->text(),
                                                       ui->promptTextEdit->toPlainText(),
                                                       ui->negativePromptTextEdit->toPlainText(),
                                                       reply);
                            reply->deleteLater();
                        });
                    }
                }
            }
            return;
        }

    case ConnectionMode::WebUI:
    case ConnectionMode::Unknown:
        return;
        break;
    }

    qDebug() << "img get.";

    if (!images.isEmpty()) {
        qDebug() << "接收到图片";

        jsonObj.insert("title", title);
        jsonObj.insert("prompt", prompt);
        jsonObj.insert("negativePrompt", negativePrompt);
        for (int i = 0; i < images.size(); i++){
            count--;
            if(count!=0){
                // ui->outputTextEdit->append("");
                ui->outputTextEdit->append(QStringLiteral(""));
                ui->outputTextEdit->append(QStringLiteral("接下来还有<strong> ")\
                                           + QString::number(count)\
                                           + QStringLiteral(" </strong>张..."));
            }
            else{
                ui->outputTextEdit->append(QStringLiteral("<strong>画图完成！</strong>"));
            }
            QByteArray imageData = images[i];
            saveImageAndJson(imageData, jsonObj);
            qDebug() << "showing img..";

            sendImage(imageData);
            saveHistory();
        }
    }
    return;
}


// 存图到本地
void MainWindow::saveImageAndJson(const QByteArray &imageData, const QJsonObject &jsonResponse)
{
    QJsonObject jsonObj = QJsonDocument(jsonResponse).object();
    jsonObj.remove("images");

    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");

    // 保存图片
    QDir outputDir = QDir(QDir::currentPath() + "/output");
    if (!outputDir.exists())
        outputDir.mkpath(".");
    QImage image;
    if (image.loadFromData(imageData)) {
        QString imagePath = "./output/image_" + timestamp + ".png";
        image.save(imagePath);
    }
    else {
        ui->outputTextEdit->append("Failed to save image.");
        showError("Failed to save image.");
    }

    // 保存JSON
    QString jsonFilePath = "./output/response_" + timestamp + ".json";
    QFile jsonFile(jsonFilePath);
    if (jsonFile.open(QIODevice::WriteOnly)) {
        QTextStream out(&jsonFile);
        out << QJsonDocument(jsonObj).toJson();
        jsonFile.close();
    } else {
        ui->outputTextEdit->append("Failed to save JSON.");
        showError("Failed to save JSON.");
    }
    return;
}

// 显示星标prompt窗口
void MainWindow::showStarPrompts(){
    m_starPromptWindow->showStarPrompts();
}

// 错误
void MainWindow::showError(const QString &errorString)
{
    ErrorDialog errorDialog(this);
    // QErrorMessage errorDialog(this);
    errorDialog.setErrorMessage(errorString);  // 设置错误信息
    // errorDialog.setAttribute(Qt::WA_DeleteOnClose);
    errorDialog.exec();  // 显示窗口
    // errorDialog.show(); //将会闪一下就消失
}

// comfyui 的 websocket 连接成功
void MainWindow::onComfyUIConnected() {
    qDebug() << "ComfyUI connected.";
}

// comfyui的websocket断连
void MainWindow::onComfyUIDisconnected() {
    qDebug() << "ComfyUI disconnected.";
}

// websocket收到信息
// websocket疑似只会对同clientId最后连接的一个发消息
// 不确定
void MainWindow::onWebSocketMessageReceived(const QString &message) {
    // qDebug() << "websocket收到响应";
    QJsonDocument jsonDoc = QJsonDocument::fromJson(message.toUtf8());
    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        ui->outputTextEdit->append("无效的 JSON 消息");
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();

    // 处理不同类型的消息
    if (jsonObj.contains("type")) {
        QString msgType = jsonObj["type"].toString();
        qDebug() << msgType;
        // 普通的状态信息 不需要管
        // 一般是预览图、队列剩余图片个数
        if (msgType == "status") {
            // QJsonObject statusData = jsonObj["data"].toObject();
            // clientId = statusData["data"].toObject()["sid"].toString();
            // saveJsonData(jsonObj["data"].toString());

            QJsonObject jsonObjTmp = QJsonDocument::fromJson(postDataMap[m_currentMode]).object();
            jsonObjTmp.insert("client_id", clientId);
            postDataMap[m_currentMode] = QJsonDocument(jsonObjTmp).toJson(QJsonDocument::Compact);

            QString filePath = "./" + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".json";
            QFile file(filePath);
            if (!file.open(QIODevice::WriteOnly)) {
                qDebug() << "无法打开文件:" << filePath;
                return;
            }
            qint64 bytesWritten = file.write(QJsonDocument(jsonObjTmp).toJson(QJsonDocument::Compact));
            file.close();

        }else if (msgType == "execution_start"){
            // 开始画图
            qDebug() << "开始成功";
            if (jsonObj.contains("data")){
                QJsonObject jsonDataObj = jsonObj["data"].toObject();
                if (jsonDataObj.contains("prompt_id")){
                    promptIdSet.insert(jsonDataObj["prompt_id"].toString());
                }
            }
        }else if (msgType == "execution_success") {
            // 画图成功
            // 返回的信息中包括prompt id
            qDebug() << "绘图成功";
            if (jsonObj.contains("data")){
                QJsonObject jsonDataObj = jsonObj["data"].toObject();
                if (jsonDataObj.contains("prompt_id") && promptIdSet.contains(jsonDataObj["prompt_id"].toString())){
                    promptIdSet.remove(jsonDataObj["prompt_id"].toString());

                    QNetworkRequest request = requestMap[m_currentMode];
                    QByteArray postData = postDataMap[m_currentMode];

                    QUrl url = request.url();
                    url.setPath("/history/" + jsonDataObj["prompt_id"].toString());

                    qDebug() << url.toString();

                    request.setUrl(url); // 之前忘记这句了，唐完了

                    // 直接在这里发送了请求
                    // 在想要不要专门写个方法处理
                    qDebug() << request.url();

                    QNetworkReply *reply = m_httpNetworkManager->get(request);
                    connect(reply, &QNetworkReply::finished, [=](){
                        MainWindow::onNetworkReply(ui->titleLineEdit->text(),
                                                   ui->promptTextEdit->toPlainText(),
                                                   ui->negativePromptTextEdit->toPlainText(),
                                                   reply);
                        reply->deleteLater();
                    });
                }
            }

        } else if (msgType == "progress") {
            // 生成过程中的消息
            // 还有附带预览图的二进制响应
            // QJsonObject progressData = jsonObj["data"].toObject();
            // int value = progressData["value"].toInt();
            // int max = progressData["max"].toInt();
            // ui->outputTextEdit->append("任务进度: " + QString::number(value) + "/" + QString::number(max));

        }
    } else if (jsonObj.contains("prompt_id")) {
        // 处理任务 ID
        QString taskId = jsonObj["prompt_id"].toString();
        ui->outputTextEdit->append("收到任务 ID: " + taskId);
        // subscribeTask(taskId); // 订阅任务更新
    }
}

// 显示图片
void MainWindow::sendImage(QByteArray &imageData){
    imageWindow *m_imageWindow = new imageWindow(this);
    m_imageWindow->setImage(imageData);
}

// 构建工作流请求
// 暂时不用
void MainWindow::generateRequestComfyUI(const QString& prompt, const QString& negativePrompt, const QString& key) {
    return;
}


void MainWindow::showConfigDialog() {
    ConfigDialog dlg("./dataFile/ApiData.json", this);
    if (dlg.exec() == QDialog::Accepted) {
        m_currentConfig = dlg.getConfigData();
        // 这里可以触发配置更新
    }
}
