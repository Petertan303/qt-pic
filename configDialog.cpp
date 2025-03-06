// configdialog.cpp
#include "configDialog.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QScrollArea>

ConfigDialog::ConfigDialog(const QString& configPath, QWidget *parent)
    : QDialog(parent), m_configPath(configPath)
{
    setupUI();
    loadConfig();
}

void ConfigDialog::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    m_tabWidget = new QTabWidget;

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QPushButton *saveButton = new QPushButton("保存");
    QPushButton *cancelButton = new QPushButton("恢复");

    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addWidget(m_tabWidget);
    mainLayout->addLayout(buttonLayout);

    connect(saveButton, &QPushButton::clicked, this, &ConfigDialog::saveChanges);
    connect(cancelButton, &QPushButton::clicked, this, &ConfigDialog::cancelChanges);
}

void ConfigDialog::loadConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("无法打开配置文件");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    m_originalData = doc.object();
    m_currentData = m_originalData;
    populateTabs();
}

void ConfigDialog::saveChanges() {
    QJsonArray dataArray = m_currentData["data"].toArray();

    for (int tabIndex = 0; tabIndex < m_tabWidget->count(); ++tabIndex) {
        QWidget *tab = m_tabWidget->widget(tabIndex);
        QScrollArea *scrollArea = qobject_cast<QScrollArea*>(tab->layout()->itemAt(0)->widget());
        QWidget *contentWidget = scrollArea->widget();
        QFormLayout *layout = qobject_cast<QFormLayout*>(contentWidget->layout());

        // 遍历所有输入框
        for (int row = 0; row < layout->rowCount(); ++row) {
            QLayoutItem *fieldItem = layout->itemAt(row, QFormLayout::FieldRole);
            if (QLineEdit *edit = qobject_cast<QLineEdit*>(fieldItem->widget())) {
                QString fullKey = edit->property("fullKey").toString();
                QString value = edit->text();

                // 更新对应的JSON数据
                QJsonArray updatedArray;
                for (auto itemValue : dataArray) {
                    QJsonObject item = itemValue.toObject();
                    if (item["name"] == m_tabWidget->tabText(tabIndex)) {
                        QJsonObject body = item["body"].toObject();
                        updateJsonObject(body, fullKey.split('.'), value);
                        item["body"] = body;
                    }
                    updatedArray.append(item);
                }
                dataArray = updatedArray;
            }
        }
    }

    m_currentData["data"] = dataArray;

    // 保存到文件（同之前）
    QFile file(m_configPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(m_currentData).toJson());
    }
    accept();
}

void ConfigDialog::cancelChanges() {
    m_currentData = m_originalData;
    populateTabs();
}

QJsonObject ConfigDialog::getConfigData() const {
    return m_currentData;
}


void ConfigDialog::populateTabs() {
    m_tabWidget->clear();

    QJsonArray dataArray = m_currentData["data"].toArray();
    for (const QJsonValue &itemValue : dataArray) {
        QJsonObject item = itemValue.toObject();
        QString tabName = item["name"].toString();

        QWidget *tab = new QWidget;
        QScrollArea *scrollArea = new QScrollArea;
        QWidget *contentWidget = new QWidget;
        QFormLayout *layout = new QFormLayout(contentWidget);

        QJsonObject body = item["body"].toObject();
        populateForm(layout, body);

        scrollArea->setWidget(contentWidget);
        scrollArea->setWidgetResizable(true);

        QVBoxLayout *tabLayout = new QVBoxLayout(tab);
        tabLayout->addWidget(scrollArea);

        m_tabWidget->addTab(tab, tabName);
    }
}

// 递归创建表单的辅助函数
void ConfigDialog::populateForm(QFormLayout* layout, const QJsonObject& obj, const QString& prefix) {
    for (const QString& key : obj.keys()) {
        QString fullKey = prefix.isEmpty() ? key : prefix + "." + key;

        if (obj[key].isObject()) {
            // 递归处理嵌套对象
            populateForm(layout, obj[key].toObject(), fullKey);
        } else {
            QLineEdit *edit = new QLineEdit(obj[key].toString());
            edit->setProperty("fullKey", fullKey);
            layout->addRow(fullKey, edit);
        }
    }
}


void ConfigDialog::updateJsonObject(QJsonObject& target, const QStringList& keys, const QString& value) {
    if (keys.isEmpty()) return;

    QString currentKey = keys.first();
    if (keys.size() == 1) {
        target[currentKey] = value;
    } else {
        if (!target.contains(currentKey) || !target[currentKey].isObject()) {
            target[currentKey] = QJsonObject();
        }
        QJsonObject child = target[currentKey].toObject();
        updateJsonObject(child, keys.mid(1), value);
        target[currentKey] = child;
    }
}
