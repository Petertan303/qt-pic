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

qint16 count;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_starPromptWindow(new starPromptWindow(this))
    // : ui(new Ui::MainWindow)
{
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
    connect(ui->sendRequestButton, &QPushButton::clicked, this, [this]() {
        MainWindow::draw(ui->promptTextEdit->toPlainText(),
                         ui->negativePromptTextEdit->toPlainText(),
                         ui->titleLineEdit->text());
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

    connectWebSocket();
    recoverHistory();
}

MainWindow::~MainWindow()
{
    if(m_webSocket)
    {
        m_webSocket->close();
    }
    delete ui;
    delete m_starPromptWindow;
}


void MainWindow::connectWebSocket() {
    ws_url = QUrl("ws://127.0.0.1:8188/ws"); // 替换为你的 ComfyUI WebSocket 地址
    m_webSocket = new QWebSocket();
    connect(m_webSocket, &QWebSocket::connected, this, &MainWindow::onWebSocketConnected);
    connect(m_webSocket, &QWebSocket::disconnected, this, &MainWindow::onWebSocketDisconnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &MainWindow::onWebSocketTextMessageReceived);
    connect(m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, &MainWindow::onWebSocketError);

    m_webSocket->open(QUrl(ws_url));
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
    QMenu *fileMenu = menuBar->addMenu("&Star");
    fileMenu->setFont(ft);
    QAction *starPromptAction = new QAction("&Star Prompt", this);
    QAction *addToStarAction = new QAction("&Add to Star", this);
    QAction *actionButton3 = new QAction("&Button 3", this);
    starPromptAction->setFont(ft);
    addToStarAction->setFont(ft);
    actionButton3->setFont(ft);

    connect(starPromptAction, &QAction::triggered, this, &MainWindow::showStarPrompts);
    // connect(addToStarAction, &QAction::triggered, this, [this]() {
    //     MainWindow::addPromptToStar(ui->promptTextEdit->toPlainText(),
    //                                 ui->negativePromptTextEdit->toPlainText(),
    //                                 ui->titleLineEdit->text());});
    connect(addToStarAction,
            &QAction::triggered,
            this,
            [this]() {
            qDebug() << "adding to star......";
            m_starPromptWindow->addPromptToStar(ui->promptTextEdit->toPlainText(),
                                                ui->negativePromptTextEdit->toPlainText(),
                                                ui->titleLineEdit->text());});

    fileMenu->addAction(starPromptAction);
    fileMenu->addAction(addToStarAction);
    fileMenu->addAction(actionButton3);
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
    // QString strTmp = QString::fromUtf8(postData);
    // qDebug() << strTmp;
    networkManager->post(request, postData);
    // networkManager->deleteLater();
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


void MainWindow::onNetworkReply(QNetworkReply* reply)
{
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

    QJsonObject jsonObj = jsonResponse.object();
    QJsonArray images = jsonObj["images"].toArray();

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
        saveImageAndJson(imageData, jsonObj);
        showImage(imageData);

        saveHistory();
    }
    reply->deleteLater();
}

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
    // 创建一个新的对话框来显示图片
    QDialog *imageDialog = new QDialog(this);
    QVBoxLayout *layout = new QVBoxLayout(imageDialog);

    if (!image.isNull()) {
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
        QImage image;
        image.loadFromData(imageData);

        // show image in label or other widget
        if(!image.isNull())
        {
            qDebug() << "show image in label...";
            showImage(image);
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


void MainWindow::sendRequestComfyUI()
{
    QJsonObject workflowData;
    QJsonObject promptObject;

    QJsonObject *node1 = new QJsonObject();
    node1->insert("inputs", QJsonObject({{"seed",4179143560},{"steps",20},{"cfg",8},{"sampler_name","euler_ancestral"},{"scheduler","normal"},{"denoise",1}}));
    node1->insert("class_type", "KSampler");
    promptObject["5"]= *node1;

    QJsonObject *node2 = new QJsonObject();
    node2->insert("inputs", QJsonObject({{"text","Girl, pink hair, anime, kawaii"}, {"clip","clip"}}));
    node2->insert("class_type", "CLIPTextEncode");
    promptObject["6"] = *node2;

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
