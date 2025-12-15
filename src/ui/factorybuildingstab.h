/**
 * @file factorybuildingstab.h
 * @brief Factory-Buildings tab for Data Hub - conveyors, production, power equipment
 */

#ifndef FACTORYBUILDINGSTAB_H
#define FACTORYBUILDINGSTAB_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

#include "core/types.h"

namespace Frontier {
class Database;
}

class FactoryBuildingsTab : public QWidget
{
    Q_OBJECT

public:
    explicit FactoryBuildingsTab(Frontier::Database *database, QWidget *parent = nullptr);


public slots:
    void refreshData();

private slots:
    void onFilterChanged();
    void onSelectionChanged();
    void onImportClicked();

private:
    void setupUi();
    void loadFactoryBuildings();
    void populateTable(const QVector<Frontier::FactoryBuilding> &buildings);

    Frontier::Database *m_database;

    // Filters
    QLineEdit *m_searchEdit;
    QComboBox *m_categoryCombo;
    QPushButton *m_importBtn;

    // Summary
    QLabel *m_countLabel;
    QLabel *m_totalPowerLabel;
    QLabel *m_totalGeneratedLabel;

    // Table
    QTableWidget *m_table;

    // Data
    QVector<Frontier::FactoryBuilding> m_buildings;
};

#endif // FACTORYBUILDINGSTAB_H
