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

    // Request
    void makeRequest(QString sDate, QString eDate, QString nbRows);
    QJsonObject agregateData();

    // Table
    void createTableHeaders(QJsonArray status, std::vector<QString> rowsHeaderVal);
    void fillTable();
    void clearCells(std::vector<QString> rowsHeaderVal);
    void showPercentage(std::vector<double> effectif, std::vector<QString> rowsHeaderVal);

    // Utils
    int  indexOfStatut(QJsonValue fields);
    void updateLoadBtnText(QString text);

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

    bool filled = false;                // init bool for table view
    std::vector<QString> statusIndex;   // vector to store order of status in tab

    // Stored data
    QJsonDocument data;



private slots:
    void handleReply(QNetworkReply *reply);
    void on_loadBtn_clicked();

    void on_nb_rows_textChanged(const QString &arg1);
    void on_end_date_dateChanged(const QDate &date);
    void on_start_date_dateChanged(const QDate &date);
};

#endif // MAINWINDOW_H
