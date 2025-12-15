/**
 * @file factorybuildingstab.cpp
 * @brief Factory-Buildings tab implementation
 */

#include "factorybuildingstab.h"
#include "core/database.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QLocale>

namespace {
QString formatCurrency(double amount)
{
    QLocale locale(QLocale::English);
    return "$" + locale.toString(static_cast<qint64>(qRound(amount)));
}

QString formatPower(double kw)
{
    if (kw <= 0) return "-";
    QLocale locale(QLocale::English);
    return locale.toString(static_cast<qint64>(qRound(kw))) + " kW";
}
}

// =============================================================================
// Constructor & Setup
// =============================================================================

FactoryBuildingsTab::FactoryBuildingsTab(Frontier::Database *database, QWidget *parent)
    : QWidget(parent)
    , m_database(database)
{
    setupUi();
    refreshData();
}

void FactoryBuildingsTab::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(10);

    // Filter bar
    auto *filterLayout = new QHBoxLayout();

    filterLayout->addWidget(new QLabel(tr("Search:")));
    m_searchEdit = new QLineEdit();
    m_searchEdit->setPlaceholderText(tr("Search buildings..."));
    m_searchEdit->setMaximumWidth(200);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &FactoryBuildingsTab::onFilterChanged);
    filterLayout->addWidget(m_searchEdit);

    filterLayout->addWidget(new QLabel(tr("Category:")));
    m_categoryCombo = new QComboBox();
    m_categoryCombo->addItem(tr("All Categories"), "");
    m_categoryCombo->addItem(tr("Factory - Conveyors"), "Factory - Conveyors");
    m_categoryCombo->addItem(tr("Factory - Pipeline"), "Factory - Pipeline");
    m_categoryCombo->addItem(tr("Factory - Power"), "Factory - Power");
    m_categoryCombo->addItem(tr("Factory - Production"), "Factory - Production");
    connect(m_categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FactoryBuildingsTab::onFilterChanged);
    filterLayout->addWidget(m_categoryCombo);

    filterLayout->addStretch();

    // Summary labels
    m_countLabel = new QLabel("0 items");
    filterLayout->addWidget(m_countLabel);

    filterLayout->addWidget(new QLabel(" | "));

    m_totalPowerLabel = new QLabel("Power: 0 kW");
    m_totalPowerLabel->setStyleSheet("color: #c62828;");
    filterLayout->addWidget(m_totalPowerLabel);

    filterLayout->addWidget(new QLabel(" | "));

    m_totalGeneratedLabel = new QLabel("Generated: 0 kW");
    m_totalGeneratedLabel->setStyleSheet("color: #2e7d32;");
    filterLayout->addWidget(m_totalGeneratedLabel);

    filterLayout->addStretch();

    m_importBtn = new QPushButton(tr("Import JSON"));
    m_importBtn->setStyleSheet("background-color: #1976d2; color: white;");
    connect(m_importBtn, &QPushButton::clicked, this, &FactoryBuildingsTab::onImportClicked);
    filterLayout->addWidget(m_importBtn);

    mainLayout->addLayout(filterLayout);

    // Table
    m_table = new QTableWidget();
    m_table->setColumnCount(9);
    m_table->setHorizontalHeaderLabels({
        tr("Name"), tr("Category"), tr("Dimensions"), tr("Speed"),
        tr("Power (kW)"), tr("Generated (kW)"), tr("Capacity"), tr("Connections"), tr("Price")
    });
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSortingEnabled(true);

    auto *header = m_table->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::Stretch);      // Name
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents); // Category
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents); // Dimensions
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents); // Speed
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents); // Power
    header->setSectionResizeMode(5, QHeaderView::ResizeToContents); // Generated
    header->setSectionResizeMode(6, QHeaderView::ResizeToContents); // Capacity
    header->setSectionResizeMode(7, QHeaderView::ResizeToContents); // Connections
    header->setSectionResizeMode(8, QHeaderView::ResizeToContents); // Price

    connect(m_table, &QTableWidget::itemSelectionChanged, this, &FactoryBuildingsTab::onSelectionChanged);

    mainLayout->addWidget(m_table);
}

// =============================================================================
// Data Loading
// =============================================================================

void FactoryBuildingsTab::refreshData()
{
    loadFactoryBuildings();
}

void FactoryBuildingsTab::loadFactoryBuildings()
{
    m_buildings = m_database->getAllFactoryBuildings();

    // Apply filters
    QString search = m_searchEdit->text().toLower();
    QString category = m_categoryCombo->currentData().toString();

    QVector<Frontier::FactoryBuilding> filtered;
    for (const auto &b : m_buildings) {
        if (!search.isEmpty() && !b.name.toLower().contains(search)) {
            continue;
        }
        if (!category.isEmpty() && b.category != category) {
            continue;
        }
        filtered.append(b);
    }

    populateTable(filtered);
}

void FactoryBuildingsTab::populateTable(const QVector<Frontier::FactoryBuilding> &buildings)
{
    m_table->setSortingEnabled(false);
    m_table->setRowCount(buildings.size());

    double totalPower = 0;
    double totalGenerated = 0;

    for (int row = 0; row < buildings.size(); ++row) {
        const auto &b = buildings[row];

        // Name
        auto *nameItem = new QTableWidgetItem(b.name);
        nameItem->setData(Qt::UserRole, b.id.value_or(0));
        m_table->setItem(row, 0, nameItem);

        // Category
        QString shortCat = b.category;
        shortCat.replace("Factory - ", "");
        m_table->setItem(row, 1, new QTableWidgetItem(shortCat));

        // Dimensions
        m_table->setItem(row, 2, new QTableWidgetItem(b.dimensions.isEmpty() ? "-" : b.dimensions));

        // Speed
        m_table->setItem(row, 3, new QTableWidgetItem(b.speed.isEmpty() ? "-" : b.speed));

        // Power (kW) - consumed
        auto *powerItem = new QTableWidgetItem(formatPower(b.powerKw));
        powerItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (b.powerKw > 0) {
            powerItem->setForeground(QColor("#c62828"));
            totalPower += b.powerKw;
        }
        powerItem->setData(Qt::UserRole, b.powerKw);
        m_table->setItem(row, 4, powerItem);

        // Generated (kW) - produced
        auto *genItem = new QTableWidgetItem(formatPower(b.generatedKw));
        genItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (b.generatedKw > 0) {
            genItem->setForeground(QColor("#2e7d32"));
            totalGenerated += b.generatedKw;
        }
        genItem->setData(Qt::UserRole, b.generatedKw);
        m_table->setItem(row, 5, genItem);

        // Capacity
        QString capStr = b.capacity > 0 ? QString::number(b.capacity) : "-";
        auto *capItem = new QTableWidgetItem(capStr);
        capItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, 6, capItem);

        // Connections
        QString connStr = b.connections > 0 ? QString::number(b.connections) : "-";
        auto *connItem = new QTableWidgetItem(connStr);
        connItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_table->setItem(row, 7, connItem);

        // Price
        auto *priceItem = new QTableWidgetItem(formatCurrency(b.price));
        priceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        priceItem->setData(Qt::UserRole, b.price);
        m_table->setItem(row, 8, priceItem);
    }

    m_table->setSortingEnabled(true);

    // Update summary
    m_countLabel->setText(QString("%1 items").arg(buildings.size()));
    m_totalPowerLabel->setText(QString("Power: %1").arg(formatPower(totalPower)));
    m_totalGeneratedLabel->setText(QString("Generated: %1").arg(formatPower(totalGenerated)));
}

// =============================================================================
// Slots
// =============================================================================

void FactoryBuildingsTab::onFilterChanged()
{
    loadFactoryBuildings();
}

void FactoryBuildingsTab::onSelectionChanged()
{
    // Could show details panel if needed
}

void FactoryBuildingsTab::onImportClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this,
                                                    tr("Import Factory Buildings JSON"),
                                                    QString(),
                                                    tr("JSON Files (*.json)"));

    if (filePath.isEmpty()) {
        return;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not open file."));
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        QMessageBox::critical(this, tr("Error"),
                              tr("JSON parse error: %1").arg(error.errorString()));
        return;
    }

    if (!doc.isArray()) {
        QMessageBox::critical(this, tr("Error"), tr("JSON must be an array."));
        return;
    }

    QJsonArray arr = doc.array();
    int imported = 0;
    int skipped = 0;

    for (const auto &val : arr) {
        if (!val.isObject()) continue;

        QJsonObject obj = val.toObject();

        Frontier::FactoryBuilding building;
        building.name = obj["name"].toString();
        building.category = obj["category"].toString();
        building.dimensions = obj["dimensions"].toString();
        building.speed = obj["speed"].toString();

        // Handle null values
        building.powerKw = obj["power_kw"].isNull() ? 0 : obj["power_kw"].toDouble();
        building.generatedKw = obj["generated_kw"].isNull() ? 0 : obj["generated_kw"].toDouble();
        building.capacity = obj["capacity"].isNull() ? 0 : obj["capacity"].toDouble();
        building.connections = obj["connections"].isNull() ? 0 : obj["connections"].toInt();
        building.price = obj["price"].isNull() ? 0 : obj["price"].toDouble();

        if (building.name.isEmpty()) {
            skipped++;
            continue;
        }

        // Check if already exists
        auto existing = m_database->getFactoryBuildingByName(building.name);
        if (existing.has_value()) {
            // Update existing
            building.id = existing->id;
            m_database->updateFactoryBuilding(building);
        } else {
            // Add new
            m_database->addFactoryBuilding(building);
        }
        imported++;
    }

    refreshData();

    QMessageBox::information(this, tr("Import Complete"),
                             QString(tr("Imported: %1\nSkipped: %2")).arg(imported).arg(skipped));
}
