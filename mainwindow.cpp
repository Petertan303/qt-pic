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
#include "imageWindow.h"

qint16 count;

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_starPromptWindow(new starPromptWindow(this)),
    // m_imageWindow(new imageWindow(this))
    m_imageWindow(new imageWindow(this))
    // : ui(new Ui::MainWindow)
{
    // m_imageWindow = new imageWindow(this);
    ui->setupUi(this);
    addMenuBar();
    networkManager = new QNetworkAccessManager(this);

    // statusoutputTextEdit = new QoutputTextEdit("准备发送请求...", this);
    ui->outputTextEdit->append("启动！");
    // ui->statusBar->addWidget(statusoutputTextEdit);  // 将 statusoutputTextEdit 添加到 statusBar

    QShortcut *shortcutDraw = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_S), this);
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
        if (m_currentMode == WebSocket) {
            sendRequestComfyUI(ui->promptTextEdit->toPlainText(),
                               ui->negativePromptTextEdit->toPlainText());
        } else {
            MainWindow::draw(ui->promptTextEdit->toPlainText(),
                             ui->negativePromptTextEdit->toPlainText(),
                             ui->titleLineEdit->text());
        }
    });
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onNetworkReply);

    // connect(starWindow, &QApplication::aboutToQuit, this, &MainWindow::saveStarPrompt);

    connect(m_starPromptWindow,
            &starPromptWindow::drawSignal,
            this,
            &MainWindow::draw);
    connect(m_starPromptWindow,
            &starPromptWindow::loadPromptSignal,
            this,
            &MainWindow::loadPrompt);

    // connect(this,
    //         &MainWindow::sendImageToImageWindow,
    //         m_imageWindow,
    //         &imageWindow::setImage);
    connect(this,
            &MainWindow::sendImageToImageWindow,
            m_imageWindow,
            &imageWindow::setImage);

    // connectWebSocket();
    recoverHistory();
}

MainWindow::~MainWindow()
{
    if(m_webSocket) {
        m_webSocket->close();
        delete m_webSocket;
    }
    delete ui;
    delete m_starPromptWindow;
    delete m_imageWindow;
}

void MainWindow::connectWebSocket() {
    if(m_webSocket) {
        m_webSocket->close();
        delete m_webSocket;
    }

    ws_url = QUrl("ws://localhost:3457/");
    m_webSocket = new QWebSocket();
    connect(m_webSocket, &QWebSocket::connected, this, [this]() {
        ui->outputTextEdit->append("WebSocket已连接");
    });
    connect(m_webSocket, &QWebSocket::disconnected, this, [this]() {
        ui->outputTextEdit->append("WebSocket已断开");
    });
    connect(m_webSocket, &QWebSocket::textMessageReceived,
            this, &MainWindow::onWebSocketTextMessageReceived);
    // connect(m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::errorOccurred()),
    //         this, [this](QAbstractSocket::SocketError error) {
    //             ui->outputTextEdit->append("WebSocket错误: " + m_webSocket->errorString());
    //         });
    m_webSocket->open(ws_url);
}


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
    httpAction->setChecked(true); // 默认选择HTTP

    connect(httpAction, &QAction::triggered, this, [this]() {
        m_currentMode = HTTP;
        ui->outputTextEdit->append("切换到HTTP模式");
    });
    connect(wsAction, &QAction::triggered, this, [this]() {
        m_currentMode = WebSocket;
        connectWebSocket();
        ui->outputTextEdit->append("切换到WebSocket模式");
    });
}

// void MainWindow::sendAddPromptToStarSignal(QString prompt, QString negativePrompt, QString title){
//     emit addPromptToStarSignal(prompt, negativePrompt, title);
// }

void MainWindow::saveHistory(){
    QJsonObject promptJson;
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

void MainWindow::recoverHistory(){
    QFile historyJson("./dataFile/history");
    historyJson.open(QIODevice::ReadOnly);
    QByteArray jsonData = historyJson.readAll();
    historyJson.close();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    QJsonObject jsonObject = jsonDoc.object();

    ui->titleLineEdit->setText(jsonObject["title"].toString());
    ui->promptTextEdit->setPlainText(jsonObject["prompt"].toString());
    ui->negativePromptTextEdit->setPlainText(jsonObject["negativePrompt"].toString());
}


void MainWindow::onClearButtonClicked(){
    ui->outputTextEdit->clear();
}


void MainWindow::loadPrompt(QString prompt, QString negativePrompt, QString title){
    ui->promptTextEdit->setPlainText(prompt);
    ui->negativePromptTextEdit->setPlainText(negativePrompt);
    ui->titleLineEdit->setText(title);

    qDebug() << "Updated key:" << title;
    qDebug() << "Negative Prompt:" << negativePrompt;
    qDebug() << "Prompt:" << prompt;
}


void MainWindow::draw(QString prompt, QString negativePrompt, QString key)
{
    // QString prompt = ui->promptTextEdit->toPlainText();
    // QString negativePrompt = ui->negativePromptTextEdit->toPlainText();
    if(count==0){
        ui->outputTextEdit->append("");
        if (key.isEmpty())
            ui->outputTextEdit->insertHtml(QStringLiteral("<strong>开始画图...</strong>"));
        else
            ui->outputTextEdit->insertHtml(QStringLiteral("<strong>开始绘制")\
                                                          + key\
                                                          + "...</strong>");
    }
    else{
        // ui->outputTextEdit->append("有 " + QString::number(count) + " 张在等待...");
        ui->outputTextEdit->append("");
        ui->outputTextEdit->insertHtml(QStringLiteral("<p>有<strong> ")\
                                       + QString::number(count)\
                                       + " </strong>张在等待...</p>");
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

void MainWindow::onNetworkReply(QNetworkReply* reply)
{
    qDebug() << "network replied.";

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
            ui->outputTextEdit->append("");
            ui->outputTextEdit->insertHtml(QStringLiteral("<p>接下来还有<strong> ")\
                                           + QString::number(count)\
                                           + QStringLiteral(" </front>张...</p>"));
        }
        else{
            ui->outputTextEdit->append("");
            ui->outputTextEdit->insertHtml("<strong>画图完成！</strong>");
            ui->outputTextEdit->append(" ");
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

        emit sendImageToImageWindow(imageData);
        saveHistory();
    }
    reply->deleteLater();
}

// void MainWindow::onSendImage(){
//     return;
// }

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

void MainWindow::showStarPrompts(){
    m_starPromptWindow->showStarPrompts();
}

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


void MainWindow::wheelEvent(QLabel *imageLabel, QWheelEvent *event)
{
    // 获取 QLabel 的当前大小
    QSize labelSize = imageLabel->size();

    // 根据鼠标滚轮的滚动量调整 QLabel 的大小
    labelSize.setWidth(labelSize.width() + event->pixelDelta().y() / 120.0 * 0.1);
    labelSize.setHeight(labelSize.height() + event->pixelDelta().y() / 120.0 * 0.1);

    // 限制 QLabel 的最小和最大大小
    labelSize.setWidth(qMax(labelSize.width(), 100));
    labelSize.setHeight(qMax(labelSize.height(), 100));

    // 设置 QLabel 的新大小
    imageLabel->resize(labelSize);
}

void MainWindow::mouseMoveEvent(QLabel *imageLabel, QPoint *mousePos, QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        // 获取鼠标移动的距离
        QPoint delta = event->pos() - *mousePos;

        // 移动 QLabel 的位置
        imageLabel->move(imageLabel->pos() + delta);

        // 限制 QLabel 的移动范围
        QRect rect = this->rect();
        imageLabel->move(
            qBound(rect.x(), imageLabel->x(), rect.x() + rect.width() - imageLabel->width()),
            qBound(rect.y(), imageLabel->y(), rect.y() + rect.height() - imageLabel->height())
            );
    }
    *mousePos = event->pos();
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


// void MainWindow::showError(const QString &errorString)
// {
//     // ErrorDialog errorDialog(this);
//     QErrorMessage errorDialog(this);
//     errorDialog.showMessage(errorString);  // 设置错误信息
//     // errorDialog.setAttribute(Qt::WA_DeleteOnClose);
//     errorDialog.exec();  // 显示窗口
//     // errorDialog.show(); //将会闪一下就消失
// }


void MainWindow::onWebSocketConnected() {
    qDebug() << "WebSocket connected.";
    // You can send subscribe message here if you already have task_id
}

void MainWindow::onWebSocketDisconnected() {
    qDebug() << "WebSocket disconnected.";
}


void MainWindow::onWebSocketError(QAbstractSocket::SocketError error)
{
    qDebug() << "websocket error: " << error;
}

void MainWindow::onWebSocketTextMessageReceived(QString message) {
    qDebug() << "WebSocket message received:" << message;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(message.toUtf8());
    if(jsonDoc.isObject())
    {
        QJsonObject jsonObj = jsonDoc.object();
        if(jsonObj.contains("type"))
        {
            QString msgType = jsonObj["type"].toString();
            if(msgType == "status")
            {
                QJsonObject statusData = jsonObj["data"].toObject();
                qDebug() << "Task Status:" << statusData["status"].toString();
            }
            else if(msgType == "execution_complete")
            {
                QJsonObject outputs = jsonObj["data"].toObject()["outputs"].toObject();
                qDebug() << "execution_complete, output is " << outputs;

                // Process image data here and call view image.
                handleExecutionComplete(outputs);
            }
            else if(msgType == "progress")
            {
                QJsonObject progressData = jsonObj["data"].toObject();
                qDebug() << "Task Progress:" << progressData["value"].toInt() << "/" << progressData["max"].toInt();
            }

        }
    }
}


void MainWindow::sendWorkflow(const QJsonObject& workflowData){
    networkManager = new QNetworkAccessManager();
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onWorkflowResponse);

    QNetworkRequest request(QUrl("http://127.0.0.1:8188/prompt")); // 替换为你的ComfyUI地址
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QByteArray jsonData = QJsonDocument(workflowData).toJson();
    networkManager->post(request, jsonData);
}

void MainWindow::onWorkflowResponse(QNetworkReply* reply){
    if (reply->error() == QNetworkReply::NoError)
    {
        QByteArray responseData = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
        if(jsonDoc.isObject())
        {
            QJsonObject jsonObj = jsonDoc.object();
            if(jsonObj.contains("prompt_id"))
            {
                QString taskId = jsonObj["prompt_id"].toString();
                qDebug() << "Task ID: " << taskId;
                subscribeTask(taskId);
            }
        }

    }
    else
    {
        qDebug() << "Request failed with error:" << reply->errorString();
    }

    reply->deleteLater();
}


void MainWindow::subscribeTask(const QString & taskId){
    QJsonObject subscribeMessage;
    subscribeMessage["type"] = "subscribe";
    subscribeMessage["task_id"] = taskId;

    if (m_webSocket && m_webSocket->isValid()) {
        m_webSocket->sendTextMessage(QJsonDocument(subscribeMessage).toJson());
    } else {
        qDebug() << "Websocket is not valid, can't subscribe task!";
    }
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


void MainWindow::viewImage(const QString& filename, const QString& subfolder){
    QNetworkAccessManager *view_manager = new QNetworkAccessManager();
    connect(view_manager, &QNetworkAccessManager::finished, this, &MainWindow::onViewResponse);
    QString viewUrl = QString("http://127.0.0.1:8188/view?filename=%1&subfolder=%2&type=output").arg(filename).arg(subfolder); // 替换为你的 ComfyUI 地址
    QNetworkRequest *request;
    request = new QNetworkRequest(QUrl(viewUrl));
    view_manager->get(*request);
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
            emit sendImageToImageWindow(imageData);
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

void MainWindow::sendRequestComfyUI(const QString& prompt, const QString& negativePrompt) {
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

    QJsonObject *node3 = new QJsonObject();
    node3->insert("inputs", QJsonObject({{"samples","5"}, {"vae","vae"}}));
    node3->insert("class_type", "VAEDecode");
    promptObject["7"] = *node3;

    QJsonObject *node4 = new QJsonObject();
    node4->insert("inputs", QJsonObject({{"model","model"},{"positive","6"},{"negative","6"}, {"latent_image", "latent_image"}}));
    node4->insert("class_type", "ApplyLora");
    promptObject["9"] = *node4;

    QJsonObject *node5 = new QJsonObject();
    node5->insert("inputs", QJsonObject({{"images","7"}}));
    node5->insert("class_type", "SaveImage");
    promptObject["11"] = *node5;

    workflowData["prompt"] = promptObject;

    QJsonObject client_id_data;
    client_id_data["client_id"] = "client_id";
    workflowData["extra_options"] = client_id_data;
    sendWorkflow(workflowData);
}
