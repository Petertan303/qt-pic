#include "historyManager.h"
#include <QJsonObject>

// historyManager::historyManager(){

//     stringToMode = {
//         {"Default", ConnectionMode::Default},
//         {"WebUI", ConnectionMode::WebUI},
//         {"ComfyUI", ConnectionMode::ComfyUI},
//         {"Unknown", ConnectionMode::Unknown}
//     };
//     for (auto it = stringToMode.begin(); it != stringToMode.end(); ++it) {
//         modeToString.insert(it.value(), it.key());
//     }
// };

// // 保存最后一次绘画的prompt
// void historyManager::saveHistory(){
//     QJsonObject promptJson;
//     promptJson["mode"] = modeToString[m_currentMode];
//     promptJson["title"] = ui->titleLineEdit->text();
//     promptJson["prompt"] = ui->promptTextEdit->toPlainText();
//     promptJson["negativePrompt"] = ui->negativePromptTextEdit->toPlainText();
//     QFile historyJson("./dataFile/history");
//     if (historyJson.open(QIODevice::WriteOnly)) {
//         QTextStream out(&historyJson);
//         out << QJsonDocument(promptJson).toJson();
//         historyJson.close();
//     }
// }

// // 启动时初始化
// void historyManager::recoverHistory(){
//     QFile historyJson("./dataFile/history");
//     historyJson.open(QIODevice::ReadOnly);
//     QByteArray jsonData = historyJson.readAll();
//     historyJson.close();

//     QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
//     QJsonObject jsonObject = jsonDoc.object();

//     m_currentMode = stringToMode[jsonObject["mode"].toString()];
//     ui->titleLineEdit->setText(jsonObject["title"].toString());
//     ui->promptTextEdit->setPlainText(jsonObject["prompt"].toString());
//     ui->negativePromptTextEdit->setPlainText(jsonObject["negativePrompt"].toString());
// }
