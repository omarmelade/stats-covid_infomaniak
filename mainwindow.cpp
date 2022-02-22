#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    manager = new QNetworkAccessManager();
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(handleReply(QNetworkReply*)));
}

MainWindow::~MainWindow()
{
    delete ui;
    delete manager;
}


//// ------------------ REQUEST AND CRITERIA SELECTIONS

void MainWindow::handleReply(QNetworkReply *reply) {
    if (reply->error()) {
        qDebug() << reply->errorString();
        return;
    }


    QString answer = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(answer.toUtf8());

    this->data = doc;

    fillTable();
}


void MainWindow::makeRequest(QString sDate, QString eDate, QString nbRows)
{
    QString url  = "https://data.drees.solidarites-sante.gouv.fr";
    QString path = "/api/records/1.0/search/?dataset=covid-19-resultats-issus-des-appariements-entre-si-vic-si-dep-et-vac-si";

    QString args = "&q=date%3A%5B" + sDate +"+TO+" + eDate + "%5D&facet=date&facet=vac_statut&rows=+"+ nbRows;

    request.setUrl(QUrl(url + path + args));
    manager->get(request);

}

void MainWindow::on_loadBtn_clicked()
{
    const QString dateFormat = "yyyy-MM-dd";

    QString sDate  = ui->start_date->date().toString(dateFormat);
    QString eDate  = ui->end_date->date().toString(dateFormat);
    QString nbRows = ui->nb_rows->text() == "" ? "10" : ui->nb_rows->text();

    makeRequest(sDate, eDate, nbRows);

}


//// ------------------ TABLE VIEW

int getIndex(std::vector<QString> v, QString k)
{
    auto it = find(v.begin(), v.end(), k);

    if (it == v.end())
    {
        return -1;
    }
    return it - v.begin();
}


void MainWindow::createTableHeaders(QJsonArray status, std::vector<QString> rowsHeaderVal)
{

    for (int i = 0; i < status.size() ; i++ ) {
        QJsonValue s = status[i];
        QString sName = s["name"].toString();
        horizontalHeader.append(sName);
        statusIndex.push_back(sName);
    }

    for(size_t i = 0; i < rowsHeaderVal.size(); i++)
    {
        verticalHeader.append(rowsHeaderVal[i]);
    }
    filled = true;
}

void MainWindow::fillTable()
{
    QJsonArray status = this->data["facet_groups"][1]["facets"].toArray();
    std::vector<QString> rowsHeaderVal = {"hc_pcr", "sc_pcr", "dc_pcr"};

    if(!filled)
    {
        createTableHeaders(status, rowsHeaderVal);
    }else
    {
        /// RESET TABLE

        ui->tableResults->model()->removeColumns(0, status.size());
        ui->tableResults->model()->removeRows(0, 3);
    }

    model.index(1,1,model.index(0,0));
    model.setHorizontalHeaderLabels(horizontalHeader);
    model.setVerticalHeaderLabels(verticalHeader);

    ui->tableResults->setModel(&model);

    QJsonArray records = this->data["records"].toArray();

    std::vector<double> effectif = std::vector<double>(statusIndex.size());
    std::fill(effectif.begin(), effectif.end(), 0);

    for(int i = 0; i < records.size(); i++)
    {
        QJsonValue r = records[i];
        QJsonValue fields = r["fields"];
        QString statut = fields["vac_statut"].toString();
        int index = getIndex(statusIndex, statut);

        // calcul du pourcentage
        double oldEff = effectif.at(index);
        double newEff = fields["effectif"].toDouble();
        if(oldEff == 0 || oldEff < newEff)
        {
            effectif.at(index) = newEff;
        }

        if(index != -1)
        {
                for(size_t j = 0; j < rowsHeaderVal.size(); j++ )
                {

                    QModelIndex qIndex = ui->tableResults->model()->index(j, index);
                    QVariant oldV = ui->tableResults->model()->data(qIndex);

                    if(oldV != "")
                    {
                        double count = (fields[rowsHeaderVal[j]].toDouble() + oldV.toDouble());
                        QString newV = QString::number(count);
                        ui->tableResults->model()->setData(qIndex, newV);
                    }
                }
        }else
        {
            qDebug() << "error values are in the wrong format.";
        }
    }





    ui->tableResults->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

}

