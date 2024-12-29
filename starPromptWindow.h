#ifndef STARPROMPTWINDOW_H
#define STARPROMPTWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QCloseEvent>

namespace Ui {
class starPromptWindow;  // 前向声明Ui类
}

class starPromptWindow : public QMainWindow
{
    Q_OBJECT

protected:
    void closeEvent(QCloseEvent *event) override;

public:
    explicit starPromptWindow(QMainWindow *parent = nullptr);
    ~starPromptWindow();  // 析构函数

public slots:
    void addPromptToStar(QString prompt, QString negativePrompt, QString title);
    void showStarPrompts();
    void initStarPrompts();
    void sendDrawSignal(QString key, QString negativePrompt, QString prompt);
    void sendLoadPromptSignal(QString prompt, QString negativePrompt, QString title);

// 使用信号而非直接控制控件
signals:
    void drawSignal(QString prompt, QString negativePrompt, QString key);
    void loadPromptSignal(QString prompt, QString negativePrompt, QString title);

private slots:

private:
    Ui::starPromptWindow *ui;
    // QWidget *starWindow;
    QTabWidget *tabWidget;
    void drawInfoPrint(QString key, QString *negativePromptTextEdit, QString *promptTextEdit);
    void addTabForStarPrompts(QString prompt, QString negativePrompt, QString key, QTabWidget *tabWidget);
    QJsonObject readStarPrompts();
    void saveStarPrompt();
    void deleteCurrentStarPrompt();

    // not used
    void saveCurrentStarPrompt();
};

#endif // STARPROMPTWINDOW_H
