#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QStandardItemModel>
#include <QHeaderView>





QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void makeRequest(QString sDate, QString eDate, QString nbRows);

    void createTableHeaders(QJsonArray status, std::vector<QString> rowsHeaderVal);
    void fillTable();

private:
    Ui::MainWindow *ui;
    // Network Objects
    QNetworkAccessManager *manager;
    QNetworkRequest request;

    // Table Objects
    QStandardItemModel model;
    QModelIndex modelIndex;
    QStringList horizontalHeader;
    QStringList verticalHeader;
    bool filled = false;
    std::vector<QString> statusIndex;

    // Stored data
    QJsonDocument data;



private slots:
    void handleReply(QNetworkReply *reply);
    void on_loadBtn_clicked();

};

#endif // MAINWINDOW_H
