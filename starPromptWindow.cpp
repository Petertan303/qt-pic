#include "starPromptWindow.h"
#include "ui_starPromptWindow.h"
// #include "globals.h"
#include <QTabWidget>
#include <QVBoxLayout>
#include <QFile>
#include <QTextEdit>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPushButton>
#include <QCloseEvent>
#include <QLayout>
#include <QString>

starPromptWindow::starPromptWindow(QMainWindow *parent) :
    QMainWindow(parent), ui(new Ui::starPromptWindow)
{
    ui->setupUi(this);
    initStarPrompts();
}

starPromptWindow::~starPromptWindow()
{
    delete ui;  // 清理资源
}

// 隐藏窗口而不是关闭它
// 因为用的是show
// 我不想但是也可以重复创建窗口实例
void starPromptWindow::closeEvent(QCloseEvent *event) {
    this->hide();
    saveStarPrompt();
    event->ignore();  // 忽略关闭事件，防止窗口被销毁
}

void starPromptWindow::addPromptToStar(QString prompt, QString negativePrompt, QString title){
    qDebug() << "exists " << title << "already";
    if (!tabWidget){
        return;
    }
    if (!this->isActiveWindow())
        showStarPrompts();
    int tabCount = tabWidget->count();
    for (int i = 0; i < tabCount; ++i) {
        if (tabWidget->tabText(i) == title) {
            qDebug() << "exists " << title << "already";
            tabWidget->widget(i)->findChild<QTextEdit*>("negativePromptTextEdit")->setText(negativePrompt);
            tabWidget->widget(i)->findChild<QTextEdit*>("promptTextEdit")->setText(prompt);
            return;
        }
    }
    if (!title.isEmpty()){
        qDebug() << "adding tab" << title;
        addTabForStarPrompts(prompt, negativePrompt, title, tabWidget);
    }
    saveStarPrompt();
}

void starPromptWindow::addTabForStarPrompts(QString prompt, QString negativePrompt, QString key, QTabWidget *tabWidget){
    QFont ft;
    ft.setPointSize(12);
    // 创建每个 Tab 页面
    QWidget *tabPage = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(tabPage);
    QHBoxLayout *buttonLayout = new QHBoxLayout(tabPage);

    // 创建两个 QTextEdit 控件用于显示和编辑数据
    QTextEdit *negativePromptTextEdit = new QTextEdit(tabPage);
    QTextEdit *promptTextEdit = new QTextEdit(tabPage);

    negativePromptTextEdit->setObjectName("negativePromptTextEdit");
    promptTextEdit->setObjectName("promptTextEdit");

    negativePromptTextEdit->setText(negativePrompt);
    promptTextEdit->setText(prompt);
    promptTextEdit->setFont(ft);

    layout->addWidget(promptTextEdit);
    layout->addWidget(negativePromptTextEdit);

    // 创建保存按钮
    QPushButton *loadButton = new QPushButton("load", tabPage);
    QPushButton *drawButton = new QPushButton("quick draw", tabPage);
    QPushButton *delButton = new QPushButton("delete", tabPage);
    // QPushButton *saveButton = new QPushButton("save", tabPage);
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    loadButton->setSizePolicy(sizePolicy);
    drawButton->setSizePolicy(sizePolicy);
    delButton->setSizePolicy(sizePolicy);
    // saveButton->setSizePolicy(sizePolicy);

    loadButton->setFont(ft);
    drawButton->setFont(ft);
    delButton->setFont(ft);
    // saveButton->setFont(ft);

    buttonLayout->addWidget(loadButton);
    buttonLayout->addWidget(drawButton);
    // buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(delButton);

    layout->addLayout(buttonLayout);

    layout->setStretchFactor(promptTextEdit, 5);
    layout->setStretchFactor(negativePromptTextEdit, 2);
    layout->setStretchFactor(buttonLayout, 1);

    tabWidget->addTab(tabPage, key);

    connect(loadButton,
            &QPushButton::clicked,
            this,
            [this, key, negativePromptTextEdit, promptTextEdit]() {
                starPromptWindow::sendLoadPromptSignal(promptTextEdit->toPlainText(),
                                                       negativePromptTextEdit->toPlainText(),
                                                       key);
            });
    connect(drawButton,
            &QPushButton::clicked,
            this,
            [this,key, negativePrompt, prompt](){starPromptWindow::sendDrawSignal(prompt, negativePrompt, key);});
    // 不使用固定索引值，而是使用tabWidget->currentIndex()获取
    connect(delButton,
            &QPushButton::clicked,
            this,
            &starPromptWindow::deleteCurrentStarPrompt);
    // connect(saveButton,
    //         &QPushButton::clicked,
    //         this,
    //         &starPromptWindow::saveCurrentStarPrompt);
}

void starPromptWindow::sendDrawSignal(QString key, QString negativePrompt, QString prompt){
    emit drawSignal(key, negativePrompt, prompt);
}

void starPromptWindow::sendLoadPromptSignal(QString prompt, QString negativePrompt, QString title){
    emit loadPromptSignal(prompt, negativePrompt, title);
}


QJsonObject starPromptWindow::readStarPrompts(){
    QFile historyJson("./dataFile/StarPrompts.json");
    historyJson.open(QIODevice::ReadOnly);
    QByteArray jsonData = historyJson.readAll();
    historyJson.close();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    QJsonObject jsonObject = jsonDoc.object();
    return jsonObject;
}

void starPromptWindow::initStarPrompts(){
    this->setWindowTitle("Star Prompts");
    this->resize(600, 600);
    // QVBoxLayout *layout = new QVBoxLayout(imageDialog);

    QJsonObject json = readStarPrompts();
    tabWidget = new QTabWidget(this);
    // tabWidget->setTabPosition(QTabWidget::West);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    // QLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabWidget);

    // 遍历 JSON 对象，为每个键创建 Tab 页面
    for (QString key : json.keys()) {
        QJsonObject item = json[key].toObject();
        addTabForStarPrompts(item["prompt"].toString(), item["negativePrompt"].toString(), key, tabWidget);
    }

    // setLayout(mainLayout);
    // 会挤成一团
    // this->setLayout(mainLayout);
    ui->layoutWidget->setLayout(mainLayout);
    this->setAttribute(Qt::WA_DeleteOnClose);
}

void starPromptWindow::showStarPrompts(){

    this->show();
}

void starPromptWindow::saveStarPrompt(){
    int tabNum = tabWidget->count();
    QJsonObject starPromptJson;
    QJsonObject promptJson;
    QFile historyJson("./dataFile/StarPrompts.json");
    QString title;
    if (historyJson.open(QIODevice::WriteOnly)) {
        for (int currentIndex = 0; currentIndex < tabNum; currentIndex++){
            // 不使用widget，而是tabWidget统一获取
            title = tabWidget->tabText(currentIndex);
            promptJson["prompt"] = tabWidget->widget(currentIndex)->findChild<QTextEdit*>("promptTextEdit")->toPlainText();
            promptJson["negativePrompt"] = tabWidget->widget(currentIndex)->findChild<QTextEdit*>("negativePromptTextEdit")->toPlainText();
            starPromptJson.insert(title, promptJson);
        }
        QTextStream out(&historyJson);
        out << QJsonDocument(starPromptJson).toJson();
        historyJson.close();
    }
}


void starPromptWindow::deleteCurrentStarPrompt(){
    tabWidget->removeTab(tabWidget->currentIndex());
    saveStarPrompt();
}


void starPromptWindow::saveCurrentStarPrompt(){
    return;
}
