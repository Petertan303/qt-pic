#include "globals.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
// #include "ui_error.h"
#include "errordialog.h"
// #include <QErrorMessage>
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
#include <tuple>
#include <QMessageBox>
#include <QWidget>
#include <QLineEdit>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QFileDialog>
#include <QActionGroup>

// static QMap<QString, MainWindow::ConnectionMode> modeToString;
// static QMap<MainWindow::ConnectionMode, QString> stringToMode;

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

void MainWindow::initAll(){
    ui = new Ui::MainWindow;
    // modeToString = mapModeToString();
    // stringToMode = mapStringToMode();
    // m_currentMode = ConnectionMode::Default;
    m_starPromptWindow = new starPromptWindow(this);
    networkManager = new QNetworkAccessManager(this);
    ui->setupUi(this);
    m_webSocket = nullptr; // 初始化为空
    taskId = "";
    m_currentTaskId = "empty";
    ws_url = QUrl("ws://localhost:3457/");
    recoverHistory();
    addMenuBar();
    ui->outputTextEdit->append(QStringLiteral("启动！"));
}

// 连接按钮、方法，或者说信号、槽
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
            case ConnectionMode::ComfyUI:
            case ConnectionMode::WebUI:
                MainWindow::draw(ui->promptTextEdit->toPlainText(),
                                 ui->negativePromptTextEdit->toPlainText(),
                                 ui->titleLineEdit->text());
                break;
            case ConnectionMode::WebSocket:
                MainWindow::sendRequestComfyUI(ui->promptTextEdit->toPlainText(),
                                   ui->negativePromptTextEdit->toPlainText(),
                                   ui->titleLineEdit->text());
                break;
            case ConnectionMode::UnknownMode:
                ui->outputTextEdit->append("未能成功连接。");
                break;
            // case:break;
        }
    });
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onNetworkReply);

    // connect(starWindow, &QApplication::aboutToQuit, this, &MainWindow::saveStarPrompt);

    connect(m_starPromptWindow,
            &starPromptWindow::drawSignal,
            this, // 方括号：捕获的变量；圆括号：传递的变量或者说参数
            [this](QString prompt, QString negativePrompt, QString key) {
                switch(m_currentMode){
                case ConnectionMode::Default:
                case ConnectionMode::ComfyUI:
                case ConnectionMode::WebUI:
                    MainWindow::draw(prompt,
                                     negativePrompt,
                                     key);
                    break;
                case ConnectionMode::WebSocket:
                    MainWindow::sendRequestComfyUI(prompt,
                                                   negativePrompt,
                                                   key);
                    break;
                case ConnectionMode::UnknownMode:
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

// 切换到websocket模式的时候启动
void MainWindow::connectWebSocket() {
    // 如果任务 ID 相同且已连接，则无需重新订阅
    if (m_webSocket && m_webSocket->state() == QAbstractSocket::ConnectedState) {
        if (m_currentTaskId == taskId) {
            return;
        }
    }

    // 清理旧的 WebSocket 连接
    if (m_webSocket) {
        m_webSocket->close(); // 关闭连接
        m_webSocket->deleteLater(); // 延迟删除对象
        m_webSocket = nullptr; // 将指针置为 nullptr
    }
    m_webSocket = new QWebSocket();
    connect(m_webSocket, &QWebSocket::connected, this, [this]() {
        ui->outputTextEdit->append(QStringLiteral("WebSocket已连接"));
    });
    connect(m_webSocket, &QWebSocket::disconnected, this, [this]() {
        ui->outputTextEdit->append(QStringLiteral("WebSocket已断开"));
    });
    connect(m_webSocket, &QWebSocket::textMessageReceived,
            this, &MainWindow::onWebSocketTextMessageReceived);
    connect(m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred),
            this, &MainWindow::handleWebSocketError);

    m_webSocket->open(ws_url);
}

// http 解析数据
std::tuple<QNetworkRequest, QByteArray> MainWindow::readApiData(QString prompt, QString negativePrompt){
    // QString prompt = ui->promptTextEdit->toPlainText();
    // QString negativePrompt = ui->negativePromptTextEdit->toPlainText();

    QFile ApiJson("./dataFile/ApiData.json");
    ApiJson.open(QIODevice::ReadOnly);
    QByteArray jsonData = ApiJson.readAll();
    ApiJson.close();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    QJsonObject jsonObject = jsonDoc.object();

    QNetworkRequest request(jsonObject["url"].toString());
    // request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, false); //qt6没有这个
    // request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy); //防止重定向
    qDebug() << "url: " << jsonObject["url"].toString();
    QJsonObject jsonHeader = jsonObject.value("header").toObject();
    QJsonObject jsonBody = jsonObject.value("body").toObject();
    for (auto it = jsonHeader.constBegin(); it != jsonHeader.constEnd(); ++it){
        if(it.key() != "url")
            request.setRawHeader(it.key().toUtf8(), it.value().toString().toUtf8());
    }
    request.setRawHeader("Content-Type", "application/x-www-form-urlencoded");

    QByteArray postData;
    for (auto it = jsonBody.constBegin(); it != jsonBody.constEnd(); ++it){
        QString stringPost = it.key().toUtf8() + "=" + it.value().toString().toUtf8() + "&";
        postData.append(stringPost.toUtf8());
    }
    postData.append("prompt=" + QUrl::toPercentEncoding(prompt) + "&");
    postData.append("negative_prompt=, " + QUrl::toPercentEncoding(negativePrompt));

    return std::make_tuple(request, postData);
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
    QAction *httpAction = new QAction("HTTP", this);
    QAction *wsAction = new QAction("WebSocket", this);
    httpAction->setCheckable(true);
    wsAction->setCheckable(true);
    modeGroup->addAction(httpAction);
    modeGroup->addAction(wsAction);
    modeMenu->addAction(httpAction);
    modeMenu->addAction(wsAction);
    // httpAction->setChecked(true); // 默认选择HTTP

    connect(httpAction, &QAction::triggered, this, [this]() {
        m_currentMode = ConnectionMode::Default;
        ui->outputTextEdit->append("");
        ui->outputTextEdit->append(QStringLiteral("<strong>切换到HTTP模式。</strong>"));
    });
    connect(wsAction, &QAction::triggered, this, [this]() {
        m_currentMode = ConnectionMode::WebSocket;
        connectWebSocket();
        ui->outputTextEdit->append("");
        ui->outputTextEdit->append(QStringLiteral("<strong>切换到WebSocket模式</strong>"));
    });

    switch(m_currentMode){
    case ConnectionMode::Default:
        httpAction->setChecked(true);
        break;
    case ConnectionMode::WebSocket:
        if(!m_webSocket) connectWebSocket();
        wsAction->setChecked(true);
        break;
    case ConnectionMode::UnknownMode:
        m_currentMode = ConnectionMode::Default;
        httpAction->setChecked(true);
        break;
        // case:break;
    }
}

// 保存最后一次绘画的prompt
void MainWindow::saveHistory(){
    QJsonObject promptJson;
    promptJson["mode"] = mapModeToString(m_currentMode);
    // promptJson["mode"] = #m_currentMode; // #只能在宏定义里使用
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

    m_currentMode = mapStringToMode(jsonObject["mode"].toString());
    ui->titleLineEdit->setText(jsonObject["title"].toString());
    ui->promptTextEdit->setPlainText(jsonObject["prompt"].toString());
    ui->negativePromptTextEdit->setPlainText(jsonObject["negativePrompt"].toString());
}

// 清理日志窗口
void MainWindow::onClearButtonClicked(){
    ui->outputTextEdit->clear();
}

// 通用加载prompt函数
void MainWindow::loadPrompt(QString prompt, QString negativePrompt, QString title){
    ui->promptTextEdit->setPlainText(prompt);
    ui->negativePromptTextEdit->setPlainText(negativePrompt);
    ui->titleLineEdit->setText(title);

    qDebug() << "Updated key:" << title;
    qDebug() << "Negative Prompt:" << negativePrompt;
    qDebug() << "Prompt:" << prompt;
}

// 发送http的请求
void MainWindow::draw(QString prompt, QString negativePrompt, QString key)
{
    // QString prompt = ui->promptTextEdit->toPlainText();
    // QString negativePrompt = ui->negativePromptTextEdit->toPlainText();
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
    auto [request, postData] = readApiData(prompt, negativePrompt);

    // 打印请求头
    QList<QByteArray> headers = request.rawHeaderList();
    for (const QByteArray &header : headers) {
        qDebug() << "Header:" << header << "=" << request.rawHeader(header);
    }

    //打印请求体
    QString strTmp = QString::fromUtf8(postData);
    qDebug() << strTmp;
    networkManager->post(request, postData);
    // networkManager->deleteLater();
}

// http的请求
void MainWindow::onNetworkReply(QNetworkReply* reply)
{
    qDebug() << "http network replied.";

    count--;
    if (reply->error() != QNetworkReply::NoError) {
        ui->outputTextEdit->append("请求失败: " + reply->errorString());
        showError(reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument jsonResponse = QJsonDocument::fromJson(responseData);

    if (jsonResponse.isNull()) {
        ui->outputTextEdit->append("无效的 JSON 响应");
        showError("无效的 JSON 响应");
        reply->deleteLater();
        return;
    }
    // qDebug() << "reply:" << jsonResponse;

    QJsonObject jsonObj = jsonResponse.object();
    QJsonArray images = jsonObj["images"].toArray();

    qDebug() << "img get.";

    if (!images.isEmpty()) {
        if(count!=0){
            // ui->outputTextEdit->append("");
            ui->outputTextEdit->append(QStringLiteral(""));
            ui->outputTextEdit->append(QStringLiteral("接下来还有<strong> ")\
                                           + QString::number(count)\
                                           + QStringLiteral(" </strong>张..."));
                                           // + QStringLiteral(" </front>张..."));
        }
        else{
            // ui->outputTextEdit->append("");
            ui->outputTextEdit->append(QStringLiteral("<strong>画图完成！</strong>"));
            // ui->outputTextEdit->append(" ");
        }
        QByteArray imageData = QByteArray::fromBase64(images[0].toString().toUtf8());

        // qDebug() << imageData;
        saveImageAndJson(imageData, jsonObj);
        qDebug() << "showing img..";

        // QImage* image = new QImage;
        // *image = QImage(imageData);
        // showImage(*image);
        // 显示不正常

        // showImage(imageData);

        // emit sendImageToImageWindow(imageData);
        sendImage(imageData);
        saveHistory();
    }
    reply->deleteLater();
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

// 本来的显示图片函数
void MainWindow::showImage(const QByteArray &imageData)
{
    // 创建一个新的对话框来显示图片
    qDebug() << "showing img...";
    QDialog *imageDialog = new QDialog(this);
    QVBoxLayout *layout = new QVBoxLayout(imageDialog);

    QImage image;
    if (image.loadFromData(imageData)) {
        // 获取对话框的大小
        // QSize dialogSize = imageDialog->size();

        // 设置图片的最大宽度和高度为对话框的大小
        // QSize scaledSize = image.size().scaled(dialogSize, Qt::KeepAspectRatio);

        // 缩放图片
        QImage scaledImage = image.scaled(600, 600, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QLabel *imageLabel = new QLabel(imageDialog);
        imageLabel->setPixmap(QPixmap::fromImage(scaledImage));
        layout->addWidget(imageLabel);
    }

    imageDialog->setLayout(layout);
    imageDialog->setWindowTitle("图片");
    imageDialog->resize(400, 600);  // 可以根据需要调整对话框的大小
    imageDialog->setModal(false);
    // 用exec默认是模态，只能聚焦最后一个窗口
    imageDialog->setAttribute(Qt::WA_DeleteOnClose);
    imageDialog->show();
}

// 同上
void MainWindow::showImage(const QImage &image)
{
    qDebug() << "showing img...";
    // 创建一个新的对话框来显示图片
    QDialog *imageDialog = new QDialog(this);
    QVBoxLayout *layout = new QVBoxLayout(imageDialog);

    if (!image.isNull()) {
        qDebug() << "image is not null";
        // 获取对话框的大小
        // QSize dialogSize = imageDialog->size();

        // 设置图片的最大宽度和高度为对话框的大小
        // QSize scaledSize = image.size().scaled(dialogSize, Qt::KeepAspectRatio);

        // 缩放图片
        QImage scaledImage = image.scaled(600, 600, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QLabel *imageLabel = new QLabel(imageDialog);
        imageLabel->setPixmap(QPixmap::fromImage(scaledImage));
        // imageLabel->setPixmap(QPixmap::fromImage(image));
        layout->addWidget(imageLabel);
    }

    imageDialog->setLayout(layout);
    imageDialog->setWindowTitle("图片");
    imageDialog->resize(400, 600);  // 可以根据需要调整对话框的大小
    imageDialog->setModal(false);
    // 用exec默认是模态，只能聚焦最后一个窗口
    imageDialog->setAttribute(Qt::WA_DeleteOnClose);
    imageDialog->show();
}


void MainWindow::contextMenuEvent(QContextMenuEvent *event, QImage image)
{
    QMenu menu;
    // menu.addAction("复制", [this, image]() { image.copy(); });
    menu.addAction("另存为", [this, image]() { image.save("path/to/saved_image.png"); });
    menu.addAction("打开所在文件夹", [this, image]() {
        // QDesktopServices::openUrl(QUrl::fromLocalFile(image.fileName()));
    });
    menu.exec(event->globalPos());
}


void MainWindow::showError(const QString &errorString)
{
    ErrorDialog errorDialog(this);
    // QErrorMessage errorDialog(this);
    errorDialog.setErrorMessage(errorString);  // 设置错误信息
    // errorDialog.setAttribute(Qt::WA_DeleteOnClose);
    errorDialog.exec();  // 显示窗口
    // errorDialog.show(); //将会闪一下就消失
}

void MainWindow::onWebSocketConnected() {
    qDebug() << "WebSocket connected.";
    // You can send subscribe message here if you already have task_id
}

void MainWindow::onWebSocketDisconnected() {
    qDebug() << "WebSocket disconnected.";
}


void MainWindow::handleWebSocketError(QAbstractSocket::SocketError error) {
    Q_UNUSED(error)
    ui->outputTextEdit->append("\nWebSocket错误: " + m_webSocket->errorString());
}

void MainWindow::onWebSocketError(QAbstractSocket::SocketError error)
{
    qDebug() << "websocket error: " << error;
}

// websocket的api收到json或者纯文本
void MainWindow::onWebSocketTextMessageReceived(const QString &message) {
    qDebug() << "websocket network replied.";
    count--;

    QJsonDocument jsonDoc = QJsonDocument::fromJson(message.toUtf8());
    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        ui->outputTextEdit->append("无效的 JSON 消息");
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();

    // 处理不同类型的消息
    if (jsonObj.contains("type")) {
        QString msgType = jsonObj["type"].toString();
        if (msgType == "status") {
            QJsonObject statusData = jsonObj["data"].toObject();
            QString status = statusData["status"].toString();
            ui->outputTextEdit->append("任务状态: " + status);
        } else if (msgType == "execution_complete") {
            QJsonObject outputs = jsonObj["data"].toObject()["outputs"].toObject();
            handleExecutionComplete(outputs);
        } else if (msgType == "progress") {
            QJsonObject progressData = jsonObj["data"].toObject();
            int value = progressData["value"].toInt();
            int max = progressData["max"].toInt();
            ui->outputTextEdit->append("任务进度: " + QString::number(value) + "/" + QString::number(max));
        }
    } else if (jsonObj.contains("prompt_id")) {
        // 处理任务 ID
        QString taskId = jsonObj["prompt_id"].toString();
        ui->outputTextEdit->append("收到任务 ID: " + taskId);
        subscribeTask(taskId); // 订阅任务更新
    } else if (jsonObj.contains("images")){
        QJsonArray images = jsonObj["images"].toArray();
        QByteArray imageData = QByteArray::fromBase64(images[0].toString().toUtf8());
        if (!imageData.isEmpty()){
            sendImage(imageData), qDebug() << "image get!";
            if(count!=0){
                // ui->outputTextEdit->append("");
                ui->outputTextEdit->append(QStringLiteral(""));
                ui->outputTextEdit->append(QStringLiteral("接下来还有<strong> ")\
                                               + QString::number(count)\
                                               + QStringLiteral(" </strong>张..."));
                // + QStringLiteral(" </front>张..."));
            }
            else{
                // ui->outputTextEdit->append("");
                ui->outputTextEdit->append(QStringLiteral("<strong>画图完成！</strong>"));
                // ui->outputTextEdit->append(" ");
            }
            saveHistory();
        }
    }
}

// comfyui用
void MainWindow::sendWorkflow(const QJsonObject& workflowData) {
    if (!m_webSocket){
        connectWebSocket();
    }
    // 检查 WebSocket 连接状态
    if (!m_webSocket || m_webSocket->state() != QAbstractSocket::ConnectedState) {
        ui->outputTextEdit->append(QStringLiteral("WebSocket 未连接，无法发送工作流"));
        return;
    }

    // 将工作流数据转换为 JSON 字符串
    QJsonDocument doc(workflowData);
    QString jsonString = doc.toJson(QJsonDocument::Compact);

    // 发送工作流数据
    m_webSocket->sendTextMessage(jsonString);

    // 如果需要，可以在这里处理任务 ID
    if (workflowData.contains("prompt_id")) {
        QString taskId = workflowData["prompt_id"].toString();
        subscribeTask(taskId); // 订阅任务更新
    }
}

void MainWindow::onWorkflowResponse(QNetworkReply* reply) {
    if (reply->error() != QNetworkReply::NoError) {
        ui->outputTextEdit->append(QStringLiteral("工作流请求错误: ") + reply->errorString() + QStringLiteral(""));
        reply->deleteLater();
        return;
    }

    QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
    if (jsonDoc.isObject()) {
        QJsonObject jsonObj = jsonDoc.object();
        if (jsonObj.contains("images")) {
            QJsonArray images = jsonObj["images"].toArray();
            QByteArray imageData = QByteArray::fromBase64(images[0].toString().toUtf8());
            if (!imageData.isEmpty())
                sendImage(imageData), qDebug() << "image get!";
            else qDebug() << "image is null";
        }
        if (jsonObj.contains("prompt_id")) {
            QString newTaskId = jsonObj["prompt_id"].toString();
            subscribeTask(newTaskId);
        } else if (jsonObj.contains("error")) {
            ui->outputTextEdit->append(QStringLiteral("API错误: ")
                                           + jsonObj["error"].toObject()["message"].toString()
                                           + QStringLiteral(""));
        }
    }
    reply->deleteLater();
}


void MainWindow::subscribeTask(const QString &taskId) {
    if (taskId.isEmpty()) {
        ui->outputTextEdit->append(QStringLiteral("任务 ID 为空，无法订阅"));
        return;
    } else
        ui->outputTextEdit->append(QStringLiteral("任务 ID 为")
                                       + taskId
                                       + QStringLiteral(""));

    if (!m_webSocket || m_webSocket->state() != QAbstractSocket::ConnectedState) {
        ui->outputTextEdit->append(QStringLiteral("WebSocket 未连接，无法订阅任务"));
        return;
    }

    // 构造订阅消息
    QJsonObject subMsg{
        {"action", "subscribe"},
        {"task_id", taskId}
    };

    // 发送订阅消息
    m_webSocket->sendTextMessage(QJsonDocument(subMsg).toJson());
    ui->outputTextEdit->append(QStringLiteral("已订阅任务: ") + taskId + QStringLiteral(""));
}

void MainWindow::sendSubscribeMessage()
{
    if (!m_webSocket || m_webSocket->state() != QAbstractSocket::ConnectedState) return;

    QJsonObject subMsg{
        {"action", "subscribe"},
        {"task_id", m_currentTaskId}
    };
    m_webSocket->sendTextMessage(QJsonDocument(subMsg).toJson());
    ui->outputTextEdit->append(QStringLiteral("已订阅任务: ") + m_currentTaskId + QStringLiteral(""));
}

void MainWindow::handleExecutionComplete(const QJsonObject& outputs)
{

    for (const QString& node_id : outputs.keys()) {
        QJsonValue nodeValue = outputs[node_id];
        if (nodeValue.isObject())
        {
            QJsonObject nodeObj = nodeValue.toObject();
            if(nodeObj.contains("images"))
            {
                QJsonArray imagesArray = nodeObj["images"].toArray();
                for(const QJsonValue& imageValue : imagesArray)
                {
                    if (imageValue.isObject())
                    {
                        QJsonObject imageData = imageValue.toObject();
                        if(imageData.contains("filename") && imageData.contains("subfolder"))
                        {
                            QString filename = imageData["filename"].toString();
                            QString subfolder = imageData["subfolder"].toString();

                            // call view api
                            viewImage(filename, subfolder);
                        }
                    }
                }
            }
        }
    }
}

// comfyui用
void MainWindow::viewImage(const QString& filename, const QString& subfolder){
    QNetworkAccessManager *view_manager = new QNetworkAccessManager();
    connect(view_manager, &QNetworkAccessManager::finished, this, &MainWindow::onViewResponse);
    QString viewUrl = QString("http://127.0.0.1:8188/view?filename=%1&subfolder=%2&type=output").arg(filename).arg(subfolder); // 替换为你的 ComfyUI 地址
    QNetworkRequest *request;
    request = new QNetworkRequest(QUrl(viewUrl));
    view_manager->get(*request);
}

// 显示图片
void MainWindow::sendImage(QByteArray &imageData){
    imageWindow *m_imageWindow = new imageWindow(this);
    // emit sendImageToImageWindow(imageData); // 不使用信号
    m_imageWindow->setImage(imageData);
}

void MainWindow::onViewResponse(QNetworkReply* reply){
    if (reply->error() == QNetworkReply::NoError)
    {
        QByteArray imageData = reply->readAll();

        // use image data to preview in Qt.
        // QImage image;
        // image.loadFromData(imageData);

        // show image in label or other widget
        if(!imageData.isNull())
        {
            qDebug() << "show image in label...";
            // showImage(image);
            sendImage(imageData);
        }
        else
        {
            qDebug() << "Failed to load image.";
        }
    }
    else
    {
        qDebug() << "Failed to view image:" << reply->errorString();
    }
    reply->deleteLater();
}

// 构建工作流请求
void MainWindow::sendRequestComfyUI(const QString& prompt, const QString& negativePrompt, const QString& key) {
    // 构造工作流数据
    QJsonObject workflowData;
    QJsonObject promptObject;

    // Positive Prompt
    QJsonObject posNode;
    posNode["class_type"] = "CLIPTextEncode";
    posNode["inputs"] = QJsonObject{{"text", prompt}, {"clip", "clip"}};
    promptObject["6"] = posNode;

    // Negative Prompt
    QJsonObject negNode;
    negNode["class_type"] = "CLIPTextEncode";
    negNode["inputs"] = QJsonObject{{"text", negativePrompt}, {"clip", "clip"}};
    promptObject["8"] = negNode;

    // KSampler
    QJsonObject samplerNode;
    samplerNode["class_type"] = "KSampler";
    samplerNode["inputs"] = QJsonObject{
        {"seed", QDateTime::currentSecsSinceEpoch()},
        {"steps", 20},
        {"cfg", 8},
        {"sampler_name", "euler_ancestral"},
        {"scheduler", "normal"},
        {"denoise", 1},
        {"positive", "6"},
        {"negative", "8"}
    };
    promptObject["5"] = samplerNode;

    // 其他节点保持不变...
    QJsonObject node3;
    node3["inputs"] = QJsonObject{{"samples", "5"}, {"vae", "vae"}};
    node3["class_type"] = "VAEDecode";
    promptObject["7"] = node3;

    QJsonObject node4;
    node4["inputs"] = QJsonObject{{"model", "model"}, {"positive", "6"}, {"negative", "6"}, {"latent_image", "latent_image"}};
    node4["class_type"] = "ApplyLora";
    promptObject["9"] = node4;

    QJsonObject node5;
    node5["inputs"] = QJsonObject{{"images", "7"}};
    node5["class_type"] = "SaveImage";
    promptObject["11"] = node5;

    workflowData["prompt"] = promptObject;

    if(count==0){
        ui->outputTextEdit->append("");
        if (key.isEmpty())
            ui->outputTextEdit->append(QStringLiteral("<strong>开始画图...</strong>"));
        else
            ui->outputTextEdit->append(QStringLiteral("<strong>开始绘制</strong>")\
                                           + key\
                                           + QStringLiteral("<strong>...</strong>"));
    }
    else{
        ui->outputTextEdit->append(QStringLiteral("有<strong> ")\
                                       + QString::number(count)\
                                       + QStringLiteral(" </strong>张在等待..."));
    }
    count ++;


    // 发送工作流数据
    sendWorkflow(workflowData);
}
