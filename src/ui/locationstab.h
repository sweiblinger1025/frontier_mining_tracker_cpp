/**
 * @file locationstab.h
 * @brief Locations management tab for Data Hub
 */

#ifndef LOCATIONSTAB_H
#define LOCATIONSTAB_H

#include <QWidget>
#include <QTableWidget>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>

namespace Frontier {
class Database;
}

class LocationsTab : public QWidget
{
    Q_OBJECT

public:
    explicit LocationsTab(Frontier::Database *database, QWidget *parent = nullptr);

public slots:
    void refreshData();

private slots:
    // Maps
    void onAddMap();
    void onDeleteMap();
    void onMapSelectionChanged();

    // Location Types
    void onAddType();
    void onDeleteType();
    void onTypeSelectionChanged();

    // Locations
    void onAddLocation();
    void onEditLocation();
    void onDeleteLocation();
    void onLocationSelectionChanged();

    // Filters
    void onMapFilterChanged(int index);
    void onTypeFilterChanged(int index);

private:
    void setupUi();
    void loadMaps();
    void loadTypes();
    void loadLocations();
    void updateSummary();
    void populateFilterCombos();

    Frontier::Database *m_database;

    // Left panel - Maps
    QTableWidget *m_mapsTable;
    QPushButton *m_addMapBtn;
    QPushButton *m_deleteMapBtn;

    // Left panel - Location Types
    QTableWidget *m_typesTable;
    QPushButton *m_addTypeBtn;
    QPushButton *m_deleteTypeBtn;

    // Right panel - Locations
    QComboBox *m_mapFilterCombo;
    QComboBox *m_typeFilterCombo;
    QLabel *m_summaryLabel;
    QTableWidget *m_locationsTable;
    QPushButton *m_addLocationBtn;
    QPushButton *m_editLocationBtn;
    QPushButton *m_deleteLocationBtn;
};

#endif // LOCATIONSTAB_H
