#ifndef GLOBALS_H
#define GLOBALS_H

#include <QtTypes>
#include <QString>
#include <QWidget>
#include <QTextEdit>
extern qint16 count;

// class PureTextEdit : public QTextEdit
// {
// public:
//     QWidget *createWidget(QWidget *parent) override
//     {
//         QTextEdit *textEdit = new QTextEdit(parent);

//         // 限制输入法
//         QInputMethod *inputMethod = QInputMethod::create("英文输入法");
//         textEdit->setInputMethod(inputMethod);

//         // 禁止粘贴格式
//         textEdit->setAcceptRichText(false);

//         return textEdit;
//     }
// };


#endif
