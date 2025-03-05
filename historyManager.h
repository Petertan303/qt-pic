#ifndef HISTORYMANAGER_H
#define HISTORYMANAGER_H

#include <QString>
#include <QHash>


namespace Ui {
class historyManager;
}

class historyManager
{
public:
    // 保存prompt历史，恢复prompt历史
    void saveHistory();
    void recoverHistory();

    enum ConnectionMode { Default, WebUI, ComfyUI, Unknown};
    ConnectionMode m_currentMode;
    // 2MainWindow *ui;

    // 模式 和 枚举 之间的对应
    QHash<QString, historyManager::ConnectionMode> stringToMode;
    QHash<historyManager::ConnectionMode, QString> modeToString;

};

#endif // HISTORYMANAGER_H
