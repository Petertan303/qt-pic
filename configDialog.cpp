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

// 根据配置文件设置GUI
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

// 加载配置文件到GUI
void ConfigDialog::loadConfig() {
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("无法打开配置文件");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    m_originalData = doc.object();
    m_currentData = m_originalData;
    // 递归展开配置文件
    populateTabs();
}

// 保存到本地
void ConfigDialog::saveChanges() {
    QJsonArray dataArray = m_currentData["data"].toArray();

    for (int tabIndex = 0; tabIndex < m_tabWidget->count(); ++tabIndex) {
        QWidget* tab = m_tabWidget->widget(tabIndex);
        QScrollArea* scrollArea = qobject_cast<QScrollArea*>(tab->layout()->itemAt(0)->widget());
        QWidget* contentWidget = scrollArea->widget();
        QFormLayout* layout = qobject_cast<QFormLayout*>(contentWidget->layout());

        for (int row = 0; row < layout->rowCount(); ++row) {
            QLayoutItem* fieldItem = layout->itemAt(row, QFormLayout::FieldRole);
            if (QLineEdit* edit = qobject_cast<QLineEdit*>(fieldItem->widget())) {
                QString fullKey = edit->property("fullKey").toString();
                int origType = edit->property("origType").toInt();
                QVariant origValue = edit->property("origValue");
                QString inputText = edit->text();

                QVariant convertedValue;
                switch (origType) {
                case QJsonValue::Array: {
                    QJsonParseError parseError;
                    QJsonDocument doc = QJsonDocument::fromJson(inputText.toUtf8(), &parseError);
                    if (parseError.error == QJsonParseError::NoError && doc.isArray()) {
                        convertedValue = doc.array().toVariantList();
                    } else {
                        qWarning() << "数组解析失败，保留原值。错误：" << parseError.errorString();
                        convertedValue = origValue;
                    }
                    break;
                }
                case QJsonValue::Double:
                    convertedValue = inputText.toDouble();
                    break;
                case QJsonValue::Bool:
                    convertedValue = (inputText.compare("true", Qt::CaseInsensitive) == 0);
                    break;
                default:
                    convertedValue = inputText;
                }

                // 更新数据数组
                QJsonArray updatedArray;
                foreach (const QJsonValue& itemValue, dataArray) {
                    QJsonObject item = itemValue.toObject();
                    if (item["name"] == m_tabWidget->tabText(tabIndex)) {
                        QJsonObject body = item["body"].toObject();
                        updateJsonObject(body,
                                         fullKey.split('.'),
                                         convertedValue,
                                         origType);
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

// 重新读一遍本地的
// 不过因为是直接重读，所以不会保留当前浏览位置
// 之后改成只改文本
// void ConfigDialog::cancelChanges() {
//     m_currentData = m_originalData;
//     populateTabs();
// }
void ConfigDialog::cancelChanges() {
    // 遍历所有现有控件恢复值
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        QWidget* tab = m_tabWidget->widget(i);
        QScrollArea* scrollArea = qobject_cast<QScrollArea*>(tab->layout()->itemAt(0)->widget());
        QWidget* contentWidget = scrollArea->widget();
        QFormLayout* layout = qobject_cast<QFormLayout*>(contentWidget->layout());

        for (int row = 0; row < layout->rowCount(); ++row) {
            QLayoutItem* fieldItem = layout->itemAt(row, QFormLayout::FieldRole);
            if (QLineEdit* edit = qobject_cast<QLineEdit*>(fieldItem->widget())) {
                // 恢复原始值
                QVariant origValue = edit->property("origValue");
                int origType = edit->property("origType").toInt();

                QString displayText = origValue.toString();
                if (origType == QJsonValue::Bool) {
                    displayText = origValue.toBool() ? "true" : "false";
                }
                edit->setText(displayText);
            }
        }
    }
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
// void ConfigDialog::populateForm(QFormLayout* layout, const QJsonObject& obj, const QString& prefix) {
//     for (const QString& key : obj.keys()) {
//         QString fullKey = prefix.isEmpty() ? key : prefix + "." + key;
//         QJsonValue value = obj[key];

//         if (value.isObject()) {
//             populateForm(layout, value.toObject(), fullKey);
//         } else {
//             QLineEdit *edit = new QLineEdit(value.toVariant().toString());
//             // 存储原始值类型和原始值
//             edit->setProperty("origType", value.type());
//             edit->setProperty("origValue", value.toVariant());
//             edit->setProperty("fullKey", fullKey);
//             layout->addRow(fullKey, edit);
//         }
//     }
// }

void ConfigDialog::populateForm(QFormLayout* layout, const QJsonObject& obj, const QString& prefix) {
    for (const QString& key : obj.keys()) {
        QString fullKey = prefix.isEmpty() ? key : prefix + "." + key;
        QJsonValue value = obj[key];

        if (value.isObject()) {
            // 处理嵌套对象
            populateForm(layout, value.toObject(), fullKey);
        } else if (value.isArray()) {
            // 处理数组类型
            QLineEdit* edit = new QLineEdit();
            edit->setText(QJsonDocument(value.toArray()).toJson(QJsonDocument::Compact));
            edit->setProperty("origType", QJsonValue::Array);
            edit->setProperty("origValue", value.toVariant());
            edit->setProperty("fullKey", fullKey);
            layout->addRow(fullKey, edit);
        } else {
            // 处理基本类型
            QLineEdit* edit = new QLineEdit();
            if (value.isBool()) {
                edit->setText(value.toBool() ? "true" : "false");
            } else {
                edit->setText(value.toVariant().toString());
            }
            edit->setProperty("origType", value.type());
            edit->setProperty("origValue", value.toVariant());
            edit->setProperty("fullKey", fullKey);
            layout->addRow(fullKey, edit);
        }
    }
}


void ConfigDialog::updateJsonObject(QJsonObject& target,
                                    const QStringList& keys,
                                    const QVariant& value,
                                    int valueType)
{
    if (keys.isEmpty()) return;

    QString currentKey = keys.first();
    if (keys.size() == 1) {
        switch (valueType) {
        case QJsonValue::Double:
            target[currentKey] = value.toDouble();
            break;
        case QJsonValue::Bool:
            target[currentKey] = value.toBool();
            break;
        case QJsonValue::String:
            target[currentKey] = value.toString();
            break;
        case QJsonValue::Array:
            target[currentKey] = QJsonArray::fromVariantList(value.toList());
            break;
        default:
            target[currentKey] = QJsonValue::fromVariant(value);
        }
    } else {
        if (!target.contains(currentKey) || !target[currentKey].isObject()) {
            target[currentKey] = QJsonObject();
        }
        QJsonObject child = target[currentKey].toObject();
        updateJsonObject(child, keys.mid(1), value, valueType);
        target[currentKey] = child;
    }
}
