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
        //qDebug() << reply->errorString();
        updateLoadBtnText("Trop de données demandées.");
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

void MainWindow::resetLoadBtnText()
{
    ui->loadBtn->setText("Charger");
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


////////////////////////////////////
//// ------------------ DATA Agregation
////////////////////////////////////

QJsonObject MainWindow::agregateData(std::vector<QString> rowsHeaderVal)
{
    QJsonObject agregatedData = QJsonObject();
    QJsonArray records = this->data["records"].toArray();
    QJsonObject dataValues = QJsonObject();
    std::map<QString, double> eff;

    // Init the data storage
    for(size_t i = 0; i < rowsHeaderVal.size(); i++ )
    {
        dataValues.insert(rowsHeaderVal[i], 0.0);
    }
    dataValues.insert("effectif", 0.0);

    agregatedData.insert("Non-vaccinés", dataValues);
    agregatedData.insert("Autres statuts", dataValues);

    // Agregate data
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
            // updating value into the JsonObject
            agregatedData.find(key).value() = statutObject;
        }
    }

    // sum of the other effectifs
    for (auto &e : eff) {
        if(e.first != "Non-vaccinés")
        {
            eff["Autres statuts"] += e.second;
        }
    }

    // Adding the effectifs to the JsonObject
    auto nonVac = agregatedData.find("Non-vaccinés").value().toObject();
    nonVac.find("effectif").value() = eff["Non-vaccinés"];
    agregatedData.find("Non-vaccinés").value() = nonVac;

    auto others = agregatedData.find("Autres statuts").value().toObject();
    others.find("effectif").value() = eff["Autres statuts"];
    agregatedData.find("Autres statuts").value() = others;

    return agregatedData;
}


////////////////////////////////////
//// ------------------ TABLE VIEW
////////////////////////////////////

// Create Headers for the TableView, with the given array
void MainWindow::createTableHeaders()
{

    // I store the index of status in a tab
    // to be able to retreive the status index for inserting data

    horizontalHeader.append("Non-vaccinés");
    statusIndex.push_back("Non-vaccinés");

    horizontalHeader.append("Autres statuts");
    statusIndex.push_back("Autres statuts");



    verticalHeader.append("Nombre d'entrées de patients en hospitalisation complète avec Covid-19 "
                          "pour lesquels un test PCR positif a été identifié (hc_pcr)");
    verticalHeader.append("Nombre d'entrées de patients en soins critiques avec Covid-19 pour "
                          "lesquels un test PCR positif a été identifié (sc_pcr)");
    verticalHeader.append("Nombre de décès de patients hospitalisés avec Covid-19 pour lesquels "
                          "un test PCR positif a été identifié (dc_pcr)");


    // set the model to the table view
    model.index(1,1,model.index(0,0));
    model.setHorizontalHeaderLabels(horizontalHeader);
    model.setVerticalHeaderLabels(verticalHeader);

    ui->tableResults->setModel(&model);

    // we want to do this only one time
    filled = true;
}

// fill the tableview with agregated data;
void MainWindow::fillTable()
{

    // Get all status and records and store it
    std::vector<QString> rowsHeaderVal = {"hc_pcr", "sc_pcr", "dc_pcr"};
    QJsonObject data = agregateData(rowsHeaderVal);

    if(!filled)
        createTableHeaders();

    for(int i = 0; i < ui->tableResults->model()->columnCount(); i++)
    {
        auto hItem = model.horizontalHeaderItem(i);
        auto dataObj = data.find(hItem->text()).value().toObject();
        int indexH = getIndex(statusIndex, hItem->text());
        for( auto it = dataObj.begin(); it != dataObj.end(); it++)
        {
            int indexV = getIndex(rowsHeaderVal, it.key());
            QModelIndex qIndex = ui->tableResults->model()->index(indexV, indexH);
            double eff = dataObj.find("effectif").value().toDouble();
            QString strPercent = QString::number( (it.value().toDouble() * 100 ) / eff, 'f', 3);
            QString val = QString::number(it.value().toDouble(), 'f', 3);
            QString valAndPercent = val + " / " + strPercent + " %";

            ui->tableResults->model()->setData(qIndex, valAndPercent);
        }
    }

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
    resetLoadBtnText();
}


void MainWindow::on_end_date_dateChanged(const QDate &date)
{
    (void) date;
    resetLoadBtnText();
}


void MainWindow::on_start_date_dateChanged(const QDate &date)
{
    (void) date;
    resetLoadBtnText();
}


