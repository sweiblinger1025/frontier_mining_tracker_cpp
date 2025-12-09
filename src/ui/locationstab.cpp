/**
 * @file locationstab.cpp
 * @brief Locations management tab implementation
 */

#include "locationstab.h"
#include "core/database.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QSplitter>

LocationsTab::LocationsTab(Frontier::Database *database, QWidget *parent)
    : QWidget(parent)
    , m_database(database)
{
    setupUi();
    refreshData();
}

void LocationsTab::setupUi()
{
    auto *mainLayout = new QHBoxLayout(this);

    // Create splitter for resizable panels
    auto *splitter = new QSplitter(Qt::Horizontal, this);

    // ==========================================================================
    // LEFT PANEL - Maps and Location Types
    // ==========================================================================
    auto *leftPanel = new QWidget();
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    // --- Maps Group ---
    auto *mapsGroup = new QGroupBox(tr("Maps"));
    auto *mapsLayout = new QVBoxLayout(mapsGroup);

    m_mapsTable = new QTableWidget();
    m_mapsTable->setColumnCount(2);
    m_mapsTable->setHorizontalHeaderLabels({tr("Abbrev"), tr("Map Name")});
    m_mapsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_mapsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_mapsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_mapsTable->horizontalHeader()->setStretchLastSection(true);
    m_mapsTable->verticalHeader()->setVisible(false);
    m_mapsTable->setAlternatingRowColors(true);
    mapsLayout->addWidget(m_mapsTable);

    auto *mapsButtonLayout = new QHBoxLayout();
    m_addMapBtn = new QPushButton(tr("+"));
    m_addMapBtn->setFixedWidth(30);
    m_addMapBtn->setToolTip(tr("Add Map"));
    m_deleteMapBtn = new QPushButton(tr("-"));
    m_deleteMapBtn->setFixedWidth(30);
    m_deleteMapBtn->setToolTip(tr("Delete Map"));
    m_deleteMapBtn->setEnabled(false);
    mapsButtonLayout->addWidget(m_addMapBtn);
    mapsButtonLayout->addWidget(m_deleteMapBtn);
    mapsButtonLayout->addStretch();
    mapsLayout->addLayout(mapsButtonLayout);

    leftLayout->addWidget(mapsGroup);

    // --- Location Types Group ---
    auto *typesGroup = new QGroupBox(tr("Location Types"));
    auto *typesLayout = new QVBoxLayout(typesGroup);

    m_typesTable = new QTableWidget();
    m_typesTable->setColumnCount(1);
    m_typesTable->setHorizontalHeaderLabels({tr("Type Name")});
    m_typesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_typesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_typesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_typesTable->horizontalHeader()->setStretchLastSection(true);
    m_typesTable->verticalHeader()->setVisible(false);
    m_typesTable->setAlternatingRowColors(true);
    typesLayout->addWidget(m_typesTable);

    auto *typesButtonLayout = new QHBoxLayout();
    m_addTypeBtn = new QPushButton(tr("+"));
    m_addTypeBtn->setFixedWidth(30);
    m_addTypeBtn->setToolTip(tr("Add Location Type"));
    m_deleteTypeBtn = new QPushButton(tr("-"));
    m_deleteTypeBtn->setFixedWidth(30);
    m_deleteTypeBtn->setToolTip(tr("Delete Location Type"));
    m_deleteTypeBtn->setEnabled(false);
    typesButtonLayout->addWidget(m_addTypeBtn);
    typesButtonLayout->addWidget(m_deleteTypeBtn);
    typesButtonLayout->addStretch();
    typesLayout->addLayout(typesButtonLayout);

    leftLayout->addWidget(typesGroup);

    // ==========================================================================
    // RIGHT PANEL - Locations
    // ==========================================================================
    auto *rightPanel = new QWidget();
    auto *rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    auto *locationsGroup = new QGroupBox(tr("Locations"));
    auto *locationsLayout = new QVBoxLayout(locationsGroup);

    // Filter bar
    auto *filterLayout = new QHBoxLayout();
    filterLayout->addWidget(new QLabel(tr("Filter by Map:")));
    m_mapFilterCombo = new QComboBox();
    m_mapFilterCombo->setMinimumWidth(150);
    filterLayout->addWidget(m_mapFilterCombo);

    filterLayout->addWidget(new QLabel(tr("Type:")));
    m_typeFilterCombo = new QComboBox();
    m_typeFilterCombo->setMinimumWidth(120);
    filterLayout->addWidget(m_typeFilterCombo);

    filterLayout->addStretch();
    m_summaryLabel = new QLabel();
    filterLayout->addWidget(m_summaryLabel);
    locationsLayout->addLayout(filterLayout);

    // Locations table
    m_locationsTable = new QTableWidget();
    m_locationsTable->setColumnCount(4);
    m_locationsTable->setHorizontalHeaderLabels({tr(""), tr("Location Name"), tr("Map"), tr("Type")});
    m_locationsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_locationsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_locationsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_locationsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    m_locationsTable->setColumnWidth(0, 40);  // Row number column
    m_locationsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_locationsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_locationsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_locationsTable->verticalHeader()->setVisible(false);
    m_locationsTable->setAlternatingRowColors(true);
    locationsLayout->addWidget(m_locationsTable);

    // Buttons
    auto *locButtonLayout = new QHBoxLayout();
    m_addLocationBtn = new QPushButton(tr("+ Add Location"));
    m_editLocationBtn = new QPushButton(tr("Edit"));
    m_editLocationBtn->setEnabled(false);
    m_deleteLocationBtn = new QPushButton(tr("Delete"));
    m_deleteLocationBtn->setEnabled(false);
    locButtonLayout->addWidget(m_addLocationBtn);
    locButtonLayout->addWidget(m_editLocationBtn);
    locButtonLayout->addWidget(m_deleteLocationBtn);
    locButtonLayout->addStretch();
    locationsLayout->addLayout(locButtonLayout);

    rightLayout->addWidget(locationsGroup);

    // ==========================================================================
    // Add panels to splitter
    // ==========================================================================
    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);
    splitter->setSizes({300, 700});  // Initial sizes

    mainLayout->addWidget(splitter);

    // ==========================================================================
    // Connect signals
    // ==========================================================================
    connect(m_mapsTable, &QTableWidget::itemSelectionChanged,
            this, &LocationsTab::onMapSelectionChanged);
    connect(m_typesTable, &QTableWidget::itemSelectionChanged,
            this, &LocationsTab::onTypeSelectionChanged);
    connect(m_locationsTable, &QTableWidget::itemSelectionChanged,
            this, &LocationsTab::onLocationSelectionChanged);

    connect(m_addMapBtn, &QPushButton::clicked, this, &LocationsTab::onAddMap);
    connect(m_deleteMapBtn, &QPushButton::clicked, this, &LocationsTab::onDeleteMap);

    connect(m_addTypeBtn, &QPushButton::clicked, this, &LocationsTab::onAddType);
    connect(m_deleteTypeBtn, &QPushButton::clicked, this, &LocationsTab::onDeleteType);

    connect(m_addLocationBtn, &QPushButton::clicked, this, &LocationsTab::onAddLocation);
    connect(m_editLocationBtn, &QPushButton::clicked, this, &LocationsTab::onEditLocation);
    connect(m_deleteLocationBtn, &QPushButton::clicked, this, &LocationsTab::onDeleteLocation);

    connect(m_mapFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LocationsTab::onMapFilterChanged);
    connect(m_typeFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LocationsTab::onTypeFilterChanged);
}

void LocationsTab::refreshData()
{
    loadMaps();
    loadTypes();
    populateFilterCombos();
    loadLocations();
    updateSummary();
}

void LocationsTab::loadMaps()
{
    m_mapsTable->setRowCount(0);

    auto maps = m_database->getAllMaps();
    m_mapsTable->setRowCount(maps.size());

    for (int i = 0; i < maps.size(); ++i) {
        const auto &map = maps[i];

        auto *abbrevItem = new QTableWidgetItem(map.abbrev);
        abbrevItem->setData(Qt::UserRole, map.id.value_or(0));
        m_mapsTable->setItem(i, 0, abbrevItem);

        m_mapsTable->setItem(i, 1, new QTableWidgetItem(map.name));
    }
}

void LocationsTab::loadTypes()
{
    m_typesTable->setRowCount(0);

    auto types = m_database->getAllLocationTypes();
    m_typesTable->setRowCount(types.size());

    for (int i = 0; i < types.size(); ++i) {
        const auto &type = types[i];

        auto *nameItem = new QTableWidgetItem(type.name);
        nameItem->setData(Qt::UserRole, type.id.value_or(0));
        m_typesTable->setItem(i, 0, nameItem);
    }
}

void LocationsTab::populateFilterCombos()
{
    // Block signals while populating
    m_mapFilterCombo->blockSignals(true);
    m_typeFilterCombo->blockSignals(true);

    m_mapFilterCombo->clear();
    m_typeFilterCombo->clear();

    // Add "All" options
    m_mapFilterCombo->addItem(tr("All Maps"), 0);
    m_typeFilterCombo->addItem(tr("All Types"), 0);

    // Add maps
    auto maps = m_database->getAllMaps();
    for (const auto &map : maps) {
        m_mapFilterCombo->addItem(map.name, map.id.value_or(0));
    }

    // Add types
    auto types = m_database->getAllLocationTypes();
    for (const auto &type : types) {
        m_typeFilterCombo->addItem(type.name, type.id.value_or(0));
    }

    m_mapFilterCombo->blockSignals(false);
    m_typeFilterCombo->blockSignals(false);
}

void LocationsTab::loadLocations()
{
    m_locationsTable->setRowCount(0);

    int mapId = m_mapFilterCombo->currentData().toInt();
    int typeId = m_typeFilterCombo->currentData().toInt();

    QVector<Frontier::Location> locations;

    if (mapId > 0 && typeId > 0) {
        locations = m_database->getLocationsByMapAndType(mapId, typeId);
    } else if (mapId > 0) {
        locations = m_database->getLocationsByMap(mapId);
    } else if (typeId > 0) {
        locations = m_database->getLocationsByType(typeId);
    } else {
        locations = m_database->getAllLocations();
    }

    m_locationsTable->setRowCount(locations.size());

    for (int i = 0; i < locations.size(); ++i) {
        const auto &loc = locations[i];

        // Row number
        auto *numItem = new QTableWidgetItem(QString::number(i + 1));
        numItem->setData(Qt::UserRole, loc.id.value_or(0));
        numItem->setTextAlignment(Qt::AlignCenter);
        m_locationsTable->setItem(i, 0, numItem);

        // Location name
        m_locationsTable->setItem(i, 1, new QTableWidgetItem(loc.name));

        // Map name
        m_locationsTable->setItem(i, 2, new QTableWidgetItem(loc.mapName));

        // Type name
        m_locationsTable->setItem(i, 3, new QTableWidgetItem(loc.typeName));

        // Color code rows by type (matching Python version style)
        QColor rowColor;
        if (loc.typeName == "Base") {
            rowColor = QColor(255, 255, 200);  // Light yellow
        } else if (loc.typeName == "Drill Site") {
            rowColor = QColor(255, 200, 200);  // Light red/pink
        } else if (loc.typeName == "Feature") {
            rowColor = QColor(200, 255, 255);  // Light cyan
        } else if (loc.typeName == "Infrastructure") {
            rowColor = QColor(255, 230, 200);  // Light orange
        } else if (loc.typeName == "Mine Site") {
            rowColor = QColor(200, 200, 255);  // Light blue
        } else if (loc.typeName == "Pad") {
            rowColor = QColor(255, 200, 255);  // Light magenta
        } else if (loc.typeName == "Processing") {
            rowColor = QColor(200, 255, 200);  // Light green
        } else if (loc.typeName == "Stockpile") {
            rowColor = QColor(230, 230, 230);  // Light gray
        } else {
            rowColor = Qt::white;
        }

        for (int col = 0; col < m_locationsTable->columnCount(); ++col) {
            if (auto *item = m_locationsTable->item(i, col)) {
                item->setBackground(rowColor);
            }
        }
    }

    updateSummary();
}

void LocationsTab::updateSummary()
{
    int mapCount = m_database->getAllMaps().size();
    int typeCount = m_database->getAllLocationTypes().size();
    int locCount = m_database->getAllLocations().size();
    int showingCount = m_locationsTable->rowCount();

    QString summary = tr("Maps: %1 | Types: %2 | Locations: %3")
                          .arg(mapCount)
                          .arg(typeCount)
                          .arg(locCount);

    if (showingCount != locCount) {
        summary += tr(" (Showing: %1)").arg(showingCount);
    }

    m_summaryLabel->setText(summary);
}

// =============================================================================
// Map Slots
// =============================================================================

void LocationsTab::onMapSelectionChanged()
{
    bool hasSelection = !m_mapsTable->selectedItems().isEmpty();
    m_deleteMapBtn->setEnabled(hasSelection);
}

void LocationsTab::onAddMap()
{
    bool ok;
    QString abbrev = QInputDialog::getText(this, tr("Add Map"),
                                           tr("Map Abbreviation (e.g., ARC):"),
                                           QLineEdit::Normal, "", &ok);
    if (!ok || abbrev.trimmed().isEmpty()) {
        return;
    }

    QString name = QInputDialog::getText(this, tr("Add Map"),
                                         tr("Map Name:"),
                                         QLineEdit::Normal, "", &ok);
    if (!ok || name.trimmed().isEmpty()) {
        return;
    }

    Frontier::Map map;
    map.abbrev = abbrev.trimmed().toUpper();
    map.name = name.trimmed();

    if (m_database->addMap(map) > 0) {
        loadMaps();
        populateFilterCombos();
        updateSummary();
    } else {
        QMessageBox::warning(this, tr("Error"),
                             tr("Failed to add map. It may already exist."));
    }
}

void LocationsTab::onDeleteMap()
{
    auto selected = m_mapsTable->selectedItems();
    if (selected.isEmpty()) {
        return;
    }

    int mapId = selected.first()->data(Qt::UserRole).toInt();
    QString mapName = m_mapsTable->item(selected.first()->row(), 1)->text();

    // Check if map has locations
    auto locations = m_database->getLocationsByMap(mapId);
    if (!locations.isEmpty()) {
        QMessageBox::warning(this, tr("Cannot Delete"),
                             tr("Cannot delete map '%1' because it has %2 locations.\n"
                                "Delete the locations first.")
                                 .arg(mapName)
                                 .arg(locations.size()));
        return;
    }

    auto result = QMessageBox::question(this, tr("Delete Map"),
                                        tr("Delete map '%1'?").arg(mapName));
    if (result == QMessageBox::Yes) {
        if (m_database->deleteMap(mapId)) {
            loadMaps();
            populateFilterCombos();
            updateSummary();
        }
    }
}

// =============================================================================
// Location Type Slots
// =============================================================================

void LocationsTab::onTypeSelectionChanged()
{
    bool hasSelection = !m_typesTable->selectedItems().isEmpty();
    m_deleteTypeBtn->setEnabled(hasSelection);
}

void LocationsTab::onAddType()
{
    bool ok;
    QString name = QInputDialog::getText(this, tr("Add Location Type"),
                                         tr("Type Name:"),
                                         QLineEdit::Normal, "", &ok);
    if (!ok || name.trimmed().isEmpty()) {
        return;
    }

    Frontier::LocationType type;
    type.name = name.trimmed();

    if (m_database->addLocationType(type) > 0) {
        loadTypes();
        populateFilterCombos();
        updateSummary();
    } else {
        QMessageBox::warning(this, tr("Error"),
                             tr("Failed to add location type. It may already exist."));
    }
}

void LocationsTab::onDeleteType()
{
    auto selected = m_typesTable->selectedItems();
    if (selected.isEmpty()) {
        return;
    }

    int typeId = selected.first()->data(Qt::UserRole).toInt();
    QString typeName = selected.first()->text();

    // Check if type has locations
    auto locations = m_database->getLocationsByType(typeId);
    if (!locations.isEmpty()) {
        QMessageBox::warning(this, tr("Cannot Delete"),
                             tr("Cannot delete type '%1' because it has %2 locations.\n"
                                "Delete the locations first.")
                                 .arg(typeName)
                                 .arg(locations.size()));
        return;
    }

    auto result = QMessageBox::question(this, tr("Delete Type"),
                                        tr("Delete location type '%1'?").arg(typeName));
    if (result == QMessageBox::Yes) {
        if (m_database->deleteLocationType(typeId)) {
            loadTypes();
            populateFilterCombos();
            updateSummary();
        }
    }
}

// =============================================================================
// Location Slots
// =============================================================================

void LocationsTab::onLocationSelectionChanged()
{
    bool hasSelection = !m_locationsTable->selectedItems().isEmpty();
    m_editLocationBtn->setEnabled(hasSelection);
    m_deleteLocationBtn->setEnabled(hasSelection);
}

void LocationsTab::onAddLocation()
{
    // Check we have maps and types
    auto maps = m_database->getAllMaps();
    auto types = m_database->getAllLocationTypes();

    if (maps.isEmpty()) {
        QMessageBox::warning(this, tr("Cannot Add"),
                             tr("Please add at least one map first."));
        return;
    }
    if (types.isEmpty()) {
        QMessageBox::warning(this, tr("Cannot Add"),
                             tr("Please add at least one location type first."));
        return;
    }

    bool ok;
    QString name = QInputDialog::getText(this, tr("Add Location"),
                                         tr("Location Name:"),
                                         QLineEdit::Normal, "", &ok);
    if (!ok || name.trimmed().isEmpty()) {
        return;
    }

    // Select map
    QStringList mapNames;
    for (const auto &m : maps) {
        mapNames << QString("[%1] %2").arg(m.abbrev, m.name);
    }
    QString selectedMap = QInputDialog::getItem(this, tr("Add Location"),
                                                tr("Select Map:"),
                                                mapNames, 0, false, &ok);
    if (!ok) {
        return;
    }
    int mapIndex = mapNames.indexOf(selectedMap);

    // Select type
    QStringList typeNames;
    for (const auto &t : types) {
        typeNames << t.name;
    }
    QString selectedType = QInputDialog::getItem(this, tr("Add Location"),
                                                 tr("Select Type:"),
                                                 typeNames, 0, false, &ok);
    if (!ok) {
        return;
    }
    int typeIndex = typeNames.indexOf(selectedType);

    Frontier::Location loc;
    loc.name = name.trimmed();
    loc.mapId = maps[mapIndex].id.value_or(0);
    loc.typeId = types[typeIndex].id.value_or(0);

    if (m_database->addLocation(loc) > 0) {
        loadLocations();
        updateSummary();
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Failed to add location."));
    }
}

void LocationsTab::onEditLocation()
{
    auto selected = m_locationsTable->selectedItems();
    if (selected.isEmpty()) {
        return;
    }

    int row = selected.first()->row();
    int locId = m_locationsTable->item(row, 0)->data(Qt::UserRole).toInt();
    QString currentName = m_locationsTable->item(row, 1)->text();

    bool ok;
    QString newName = QInputDialog::getText(this, tr("Edit Location"),
                                            tr("Location Name:"),
                                            QLineEdit::Normal, currentName, &ok);
    if (!ok || newName.trimmed().isEmpty()) {
        return;
    }

    auto loc = m_database->getLocation(locId);
    if (!loc.has_value()) {
        return;
    }

    // For now, just update the name
    // A full edit dialog could change map/type too
    Frontier::Location updated = loc.value();
    updated.name = newName.trimmed();

    // Note: We don't have updateLocation() yet - you'd need to add it
    // For now, delete and re-add
    m_database->deleteLocation(locId);
    m_database->addLocation(updated);

    loadLocations();
}

void LocationsTab::onDeleteLocation()
{
    auto selected = m_locationsTable->selectedItems();
    if (selected.isEmpty()) {
        return;
    }

    int row = selected.first()->row();
    int locId = m_locationsTable->item(row, 0)->data(Qt::UserRole).toInt();
    QString locName = m_locationsTable->item(row, 1)->text();

    auto result = QMessageBox::question(this, tr("Delete Location"),
                                        tr("Delete location '%1'?").arg(locName));
    if (result == QMessageBox::Yes) {
        if (m_database->deleteLocation(locId)) {
            loadLocations();
            updateSummary();
        }
    }
}

// =============================================================================
// Filter Slots
// =============================================================================

void LocationsTab::onMapFilterChanged(int /*index*/)
{
    loadLocations();
}

void LocationsTab::onTypeFilterChanged(int /*index*/)
{
    loadLocations();
}
