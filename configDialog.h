#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

// configdialog.h
#include <QDialog>
#include <QTabWidget>
#include <QJsonObject>
#include <QFormLayout>

class ConfigDialog : public QDialog {
    Q_OBJECT
public:
    explicit ConfigDialog(const QString& configPath, QWidget *parent = nullptr);
    QJsonObject getConfigData() const;

private slots:
    void saveChanges();
    void cancelChanges();

private:
    void setupUI();
    void loadConfig();
    void populateTabs();

    void populateForm(QFormLayout* layout, const QJsonObject& obj, const QString& prefix = "");
    void updateJsonObject(QJsonObject& target, const QStringList& keys, const QString& value);

    QString m_configPath;
    QJsonObject m_originalData;
    QJsonObject m_currentData;
    QTabWidget *m_tabWidget;
};

#endif // CONFIGDIALOG_H
