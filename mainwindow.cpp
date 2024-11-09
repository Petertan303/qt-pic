#include "mainwindow.h"
#include "ui_mainwindow.h"
// #include "ui_error.h"
#include "errordialog.h"
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

qint16 count;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    networkManager = new QNetworkAccessManager(this);

    // statusLabel = new QLabel("准备发送请求...", this);
    ui->outputTextEdit->append("启动！");
    // ui->statusBar->addWidget(statusLabel);  // 将 statusLabel 添加到 statusBar

    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_A), this);
    connect(shortcut, &QShortcut::activated, ui->sendRequestButton, &QPushButton::click);
    connect(ui->ClearButton, &QPushButton::clicked, this, &MainWindow::onClearButtonClicked);
    // connect(ui->ClearButton, &QPushButton::clicked, this, &ui->outputTextEdit->clear);
    connect(ui->sendRequestButton, &QPushButton::clicked, this, &MainWindow::onSendRequestButtonClicked);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onNetworkReply);

    recoverHistory();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::recoverHistory(){
    QFile historyJson("./history");
    historyJson.open(QIODevice::ReadOnly);
    QByteArray jsonData = historyJson.readAll();
    historyJson.close();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    QJsonObject jsonObject = jsonDoc.object();

    ui->promptTextEdit->setPlainText(jsonObject["prompt"].toString());
    ui->negativePromptTextEdit->setPlainText(jsonObject["negativePrompt"].toString());
}

void MainWindow::onSendRequestButtonClicked()
{
    QString prompt = ui->promptTextEdit->toPlainText();
    QString negativePrompt = ui->negativePromptTextEdit->toPlainText();
    if(count==0){
        ui->outputTextEdit->append("");
        ui->outputTextEdit->insertHtml(QStringLiteral("<strong>开始画图...</strong>"));
    }
    else{
        // ui->outputTextEdit->append("有 " + QString::number(count) + " 张在等待...");
        ui->outputTextEdit->append("");
        ui->outputTextEdit->insertHtml(QStringLiteral("<p>有<strong> ")\
                                       + QString::number(count)\
                                       + " </strong>张在等待...</p>");
    }
    count ++;
    QUrl url("https://api.draw.t4wefan.pub/sdapi/v1/txt2img");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("api", "t4");
    request.setRawHeader("api_key", "Kd_oa9Oj7GJF7rGLYUH0xg");

    QByteArray postData;
    postData.append("timeout=600&");
    postData.append("prompt=" + QUrl::toPercentEncoding(prompt) + "&");
    postData.append("negative_prompt=, " + QUrl::toPercentEncoding(negativePrompt));

    networkManager->post(request, postData);
}

void MainWindow::onClearButtonClicked(){
    ui->outputTextEdit->clear();
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
            // ui->outputTextEdit->append("接下来还有 " + QString::number(count) + " 张...");
            ui->outputTextEdit->append("");
            ui->outputTextEdit->insertHtml(QStringLiteral("<p>接下来还有<strong> ")\
                                           + QString::number(count)\
                                           + QStringLiteral(" </front>张...</p>"));
        }
        else{
            ui->outputTextEdit->append("");
            // ui->outputTextEdit->append("画图完成！");
            ui->outputTextEdit->insertHtml("<strong>画图完成！</strong>");
            ui->outputTextEdit->append(" ");
        }
        QByteArray imageData = QByteArray::fromBase64(images[0].toString().toUtf8());
        saveImageAndJson(imageData, jsonObj);
        showImage(imageData);

        QJsonObject promptJson;
        promptJson["prompt"] = ui->promptTextEdit->toPlainText();
        promptJson["negativePrompt"] = ui->negativePromptTextEdit->toPlainText();
        QFile historyJson("./history");
        if (historyJson.open(QIODevice::WriteOnly)) {
            QTextStream out(&historyJson);
            out << QJsonDocument(promptJson).toJson();
        }
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
    } else {
        ui->outputTextEdit->append("Failed to save JSON.");
        showError("Failed to save JSON.");
    }
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
    imageDialog->exec();  // 显示对话框
}

void MainWindow::showError(const QString &errorString)
{
    ErrorDialog errorDialog(this);
    errorDialog.setErrorMessage(errorString);  // 设置错误信息
    errorDialog.exec();  // 显示窗口
}
