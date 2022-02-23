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

////////////////////////////////////
//// ------------------ Http Request
////////////////////////////////////

// Do updates and store data when reply has arrived
void MainWindow::handleReply(QNetworkReply *reply) {
    if (reply->error()) {
        qDebug() << reply->errorString();
        return;
    }


    QString answer = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(answer.toUtf8());

    this->data = doc;


    fillTable();
    updateLoadBtnText("Données prêtes");
}

// make the request with the given parameters
void MainWindow::makeRequest(QString sDate, QString eDate, QString nbRows)
{
    QString url  = "https://data.drees.solidarites-sante.gouv.fr";
    QString path = "/api/records/1.0/search/?dataset=covid-19-resultats-issus-des-appariements-entre-si-vic-si-dep-et-vac-si";

    QString args = "&q=date%3A%5B" + sDate +"+TO+" + eDate + "%5D&facet=date&facet=vac_statut&rows=+"+ nbRows;

    request.setUrl(QUrl(url + path + args));
    manager->get(request);

}

////////////////////////////////////
//// ------------------ Utils
////////////////////////////////////


void MainWindow::updateLoadBtnText(QString text)
{
    ui->loadBtn->setText(text);
}

// store the biggest effectif from all fields
void calcEffectif(std::vector<double>& effectif, QJsonValue fields, int index)
{
    double oldEff = effectif.at(index);
    double newEff = fields["effectif"].toDouble();
    if(oldEff == 0 || oldEff < newEff)
    {
        effectif.at(index) = newEff;
    }
}

int getIndex(std::vector<QString> v, QString k)
{
    auto it = find(v.begin(), v.end(), k);

    if (it == v.end())
    {
        return -1;
    }
    return it - v.begin();
}

// Return the index of the field to update the column
int MainWindow::indexOfStatut(QJsonValue fields)
{

    QString statut = fields["vac_statut"].toString();
    if(statut == "Non-vaccinés")
    {
        return 0;
    }

    return 1;
}

////////////////////////////////////
//// ------------------ TABLE VIEW
////////////////////////////////////

// Create Headers for the TableView, with the given array
void MainWindow::createTableHeaders(QJsonArray status, std::vector<QString> rowsHeaderVal)
{

    // I store the index of status in a tab
    // to be able to retreive the status index for inserting data
    horizontalHeader.append("Non-vaccinés");
    statusIndex.push_back("Non-vaccinés");

    horizontalHeader.append("Autres statuts");
    statusIndex.push_back("Autres statuts");

    for(size_t i = 0; i < rowsHeaderVal.size(); i++)
    {
        verticalHeader.append(rowsHeaderVal[i]);
    }

    // set the model to the table view
    model.index(1,1,model.index(0,0));
    model.setHorizontalHeaderLabels(horizontalHeader);
    model.setVerticalHeaderLabels(verticalHeader);

    ui->tableResults->setModel(&model);

    // we want to do this only one time
    filled = true;
}


// Display percentage for each cell
void MainWindow::showPercentage(std::vector<double> effectif, std::vector<QString> rowsHeaderVal)
{

    for(int i = 0; i < ui->tableResults->model()->columnCount(); i++)
    {
        for(size_t j = 0; j < rowsHeaderVal.size(); j++ )
        {
            QModelIndex qIndex = ui->tableResults->model()->index(j, i);
            QVariant oldV = ui->tableResults->model()->data(qIndex);
            qDebug() << oldV;

            // we made percentage only if the value is valid
            if(oldV.isValid() && oldV.toString() != "NC")
            {
                QString percent = QString::number( (oldV.toDouble() * 100 ) / effectif.at(i), 'f', 3);
                QString valAndPercent = oldV.toString() + " / " + percent + " %";
                ui->tableResults->model()->setData(qIndex, valAndPercent);
            }
            else
            {
                ui->tableResults->model()->setData(qIndex, "NC");
            }
        }
    }
}

// clear all cells of table
void MainWindow::clearCells(std::vector<QString> rowsHeaderVal)
{
    qDebug() << ui->tableResults->model()->columnCount();
    for(int i = 0; i < ui->tableResults->model()->columnCount(); i++)
    {
        for(size_t j = 0; j < rowsHeaderVal.size(); j++ )
        {
            QModelIndex qIndex = ui->tableResults->model()->index(j, i);
            qDebug() << i << j << ui->tableResults->model()->clearItemData(qIndex);
        }
    }
}


QJsonObject MainWindow::agregateData()
{
    QJsonObject agregatedData = QJsonObject();

    std::vector<QString> rowsHeaderVal = {"hc_pcr", "sc_pcr", "dc_pcr"};
    QJsonArray records = this->data["records"].toArray();
    std::map<QString, double> eff;

    // Init the data storage
    QJsonObject dataValues = QJsonObject();
    for(size_t i = 0; i < rowsHeaderVal.size(); i++ )
    {
        dataValues.insert(rowsHeaderVal[i], 0.0);
    }

    dataValues.insert("effectif", 0.0);

    agregatedData.insert("Non-vaccinés", dataValues);
    agregatedData.insert("Autres statuts", dataValues);

    for(int i = 0; i < records.size(); i++)
    {
        QJsonValue r = records[i];
        QJsonValue fields = r["fields"];
        QString statut = fields["vac_statut"].toString();

        // Store effectifs for percentage
        if(eff[statut] < fields["effectif"].toDouble())
            eff[statut] = fields["effectif"].toDouble();

        QString key = statut;
        if(statut != "Non-vaccinés")
        {
            key = "Autres statuts";
        }

        for(size_t j = 0; j < rowsHeaderVal.size(); j++ )
        {
            auto statutObject = agregatedData.find(key).value().toObject();

            auto value = statutObject.find(rowsHeaderVal[j]).value();
            // making sum if the old value is valid
            value = fields[rowsHeaderVal[j]].toDouble() + value.toDouble();

            agregatedData.find(key).value() = statutObject;
        }
    }

    for (auto &e : eff) {
        if(e.first != "Non-vaccinés")
        {
            eff["Autres statuts"] += e.second;
        }
    }

    auto nonVac = agregatedData.find("Non-vaccinés").value().toObject();
    nonVac.find("effectif").value() = eff["Non-vaccinés"];
    agregatedData.find("Non-vaccinés").value() = nonVac;

    auto others = agregatedData.find("Autres statuts").value().toObject();
    others.find("effectif").value() = eff["Autres statuts"];
    agregatedData.find("Autres statuts").value() = others;

    qDebug() << agregatedData;

    return agregatedData;
}

void MainWindow::fillTable()
{
    QJsonObject data = agregateData();

    // Get all status and records and store it
    QJsonArray status = this->data["facet_groups"][1]["facets"].toArray();
    std::vector<QString> rowsHeaderVal = {"hc_pcr", "sc_pcr", "dc_pcr"};
    QJsonArray records = this->data["records"].toArray();

    if(!filled)
        createTableHeaders(status, rowsHeaderVal);

    std::vector<double> effectif = std::vector<double>(statusIndex.size());
    std::fill(effectif.begin(), effectif.end(), 0);

    for(int i = 0; i < records.size(); i++)
    {
        QJsonValue r = records[i];
        QJsonValue fields = r["fields"];

        // we get the corresponding index of the status.
        int index = indexOfStatut(fields);

        calcEffectif(effectif, fields, index);

        for(size_t j = 0; j < rowsHeaderVal.size(); j++ )
        {
            // we getting the index and the old value
            QModelIndex qIndex = ui->tableResults->model()->index(j, index);
            QVariant oldV = ui->tableResults->model()->data(qIndex);

            if(oldV != "")
            {
                // making sum if the old value is valid
                double count = (fields[rowsHeaderVal[j]].toDouble() + oldV.toDouble());
                QString newV = QString::number(count);
                ui->tableResults->model()->setData(qIndex, newV);
            }
        }
    }

    // Printing percentage after the all values are set
    //showPercentage(effectif, rowsHeaderVal);

    ui->tableResults->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

}


////////////////////////////////////
//// ------------------ UI Events
////////////////////////////////////

// We made a request with the user data onClick.

void MainWindow::on_loadBtn_clicked()
{
    const QString dateFormat = "yyyy-MM-dd";

    QString sDate  = ui->start_date->date().toString(dateFormat);
    QString eDate  = ui->end_date->date().toString(dateFormat);
    QString nbRows = ui->nb_rows->text() == "" ? "10" : ui->nb_rows->text();

    makeRequest(sDate, eDate, nbRows);
    updateLoadBtnText("Chargement en cours...");
}


// When any field is modified, we update the text button
// For the user to know that i can have new set of values

void MainWindow::on_nb_rows_textChanged(const QString &arg1)
{
    (void) arg1;
    updateLoadBtnText("Charger");
}


void MainWindow::on_end_date_dateChanged(const QDate &date)
{
    (void) date;
    updateLoadBtnText("Charger");
}


void MainWindow::on_start_date_dateChanged(const QDate &date)
{
    (void) date;
    updateLoadBtnText("Charger");
}


