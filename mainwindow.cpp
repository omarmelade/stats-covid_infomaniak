#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    // API Request
    manager = new QNetworkAccessManager();
    QObject::connect(manager, SIGNAL(finished(QNetworkReply*)),
        this, SLOT(managerFinished(QNetworkReply*)));
}

MainWindow::~MainWindow()
{
    delete ui;
    delete manager;
}


void MainWindow::managerFinished(QNetworkReply *reply) {
    if (reply->error()) {
        qDebug() << reply->errorString();
        return;
    }

    QString answer = reply->readAll();

    qDebug() << answer;
}


void MainWindow::on_loadBtn_clicked()
{
    const QString dateFormat = "yyyy-MM-dd";
    QString url  = "https://data.drees.solidarites-sante.gouv.fr";
    QString path = "/api/records/1.0/search/?dataset=covid-19-resultats-issus-des-appariements-entre-si-vic-si-dep-et-vac-si";

    QString sDate  = ui->start_date->date().toString(dateFormat);
    QString eDate  = ui->end_date->date().toString(dateFormat);
    QString nbRows = ui->nb_rows->text() == "" || "0" ? "10" : ui->nb_rows->text();

    QString args = "&q=date%3A%5B" + sDate +"+TO+" + eDate + "%5D&facet=date&facet=vac_statut&rows=+"+ nbRows;

    request.setUrl(QUrl(url + path + args));
    manager->get(request);
}

