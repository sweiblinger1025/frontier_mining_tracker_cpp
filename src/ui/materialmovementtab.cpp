/**
 * @file materialmovementtab.cpp
 * @brief Implementation of Material Movement subtab
 */

#include "materialmovementtab.h"
#include "core/unitconverter.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QHeaderView>
#include <QMessageBox>
#include <QSettings>

MaterialMovementTab::MaterialMovementTab(Frontier::OperationsManager *manager,
                                         QWidget *parent)
    : QWidget(parent)
    , m_manager(manager)
    , m_currentSessionId(std::nullopt)
    , m_currentRole("Loader")
{
    setupUi();
    loadEquipmentCombo();
    loadSessionHistory();
    updateSessionControls();

    // Connect to manager signals
    connect(m_manager, &Frontier::OperationsManager::unitSystemChanged,
            this, &MaterialMovementTab::onUnitSystemChanged);
    connect(m_manager, &Frontier::OperationsManager::movementSessionStarted,
            this, &MaterialMovementTab::onSessionStarted);
    connect(m_manager, &Frontier::OperationsManager::movementSessionEnded,
            this, &MaterialMovementTab::onSessionEnded);
    connect(m_manager, &Frontier::OperationsManager::equipmentUsageUpdated,
            this, &MaterialMovementTab::onEquipmentUsageUpdated);
}

MaterialMovementTab::~MaterialMovementTab()
{
}

void MaterialMovementTab::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // Use splitter for flexible layout
    QSplitter *splitter = new QSplitter(Qt::Vertical);

    // Top section: Session Header + Equipment Usage side by side
    QWidget *topWidget = new QWidget();
    QHBoxLayout *topLayout = new QHBoxLayout(topWidget);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(10);

    topLayout->addWidget(createSessionHeaderPanel(), 1);
    topLayout->addWidget(createEquipmentUsagePanel(), 1);

    splitter->addWidget(topWidget);

    // Middle section: Usage Table
    QGroupBox *tableGroup = new QGroupBox("Equipment Usage for Session");
    QVBoxLayout *tableLayout = new QVBoxLayout(tableGroup);

    m_usageTableView = new QTableView();
    m_usageTableView->setAlternatingRowColors(true);
    m_usageTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_usageTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_usageTableView->horizontalHeader()->setStretchLastSection(true);
    m_usageTableView->verticalHeader()->setVisible(false);

    m_usageModel = new QStandardItemModel(this);
    m_usageTableView->setModel(m_usageModel);

    connect(m_usageTableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MaterialMovementTab::onUsageSelectionChanged);

    tableLayout->addWidget(m_usageTableView);

    splitter->addWidget(tableGroup);

    // Bottom section: Summary
    splitter->addWidget(createSessionSummaryPanel());

    // Set splitter sizes
    splitter->setSizes({200, 250, 100});

    mainLayout->addWidget(splitter);
}

QWidget* MaterialMovementTab::createSessionHeaderPanel()
{
    QGroupBox *group = new QGroupBox("Session");
    QVBoxLayout *layout = new QVBoxLayout(group);

    // Session History selector
    QHBoxLayout *historyLayout = new QHBoxLayout();
    QLabel *historyLabel = new QLabel("Load Session:");
    m_sessionHistoryCombo = new QComboBox();
    m_sessionHistoryCombo->setMinimumWidth(200);
    QPushButton *loadButton = new QPushButton("Load");
    connect(loadButton, &QPushButton::clicked, this, &MaterialMovementTab::onLoadSessionClicked);

    historyLayout->addWidget(historyLabel);
    historyLayout->addWidget(m_sessionHistoryCombo, 1);
    historyLayout->addWidget(loadButton);
    layout->addLayout(historyLayout);

    // Separator
    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addWidget(line);

    // Session info form
    QFormLayout *formLayout = new QFormLayout();

    m_sessionIdLabel = new QLabel("(No active session)");
    m_sessionIdLabel->setStyleSheet("font-weight: bold;");
    formLayout->addRow("Session ID:", m_sessionIdLabel);

    m_mapNameEdit = new QLineEdit();
    m_mapNameEdit->setPlaceholderText("e.g., FOREST_QUARRY");
    // Load default map from settings
    QSettings settings("FrontierMining", "Tracker");
    m_mapNameEdit->setText(settings.value("Operations/defaultMap", "").toString());
    formLayout->addRow("Map:", m_mapNameEdit);

    m_startTimeEdit = new QDateTimeEdit();
    m_startTimeEdit->setDisplayFormat("yyyy-MM-dd hh:mm:ss");
    m_startTimeEdit->setReadOnly(true);
    formLayout->addRow("Start Time:", m_startTimeEdit);

    m_endTimeEdit = new QDateTimeEdit();
    m_endTimeEdit->setDisplayFormat("yyyy-MM-dd hh:mm:ss");
    m_endTimeEdit->setReadOnly(true);
    formLayout->addRow("End Time:", m_endTimeEdit);

    layout->addLayout(formLayout);

    // Notes
    QLabel *notesLabel = new QLabel("Notes:");
    m_sessionNotesEdit = new QTextEdit();
    m_sessionNotesEdit->setMaximumHeight(60);
    m_sessionNotesEdit->setPlaceholderText("Optional session notes...");
    layout->addWidget(notesLabel);
    layout->addWidget(m_sessionNotesEdit);

    // Session control buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_startSessionButton = new QPushButton("Start New Session");
    m_startSessionButton->setStyleSheet("background-color: #4CAF50; color: white;");
    connect(m_startSessionButton, &QPushButton::clicked,
            this, &MaterialMovementTab::onStartSessionClicked);

    m_endSessionButton = new QPushButton("End Session");
    m_endSessionButton->setStyleSheet("background-color: #f44336; color: white;");
    connect(m_endSessionButton, &QPushButton::clicked,
            this, &MaterialMovementTab::onEndSessionClicked);

    m_saveSessionButton = new QPushButton("Save");
    connect(m_saveSessionButton, &QPushButton::clicked,
            this, &MaterialMovementTab::onSaveSessionClicked);

    buttonLayout->addWidget(m_startSessionButton);
    buttonLayout->addWidget(m_endSessionButton);
    buttonLayout->addWidget(m_saveSessionButton);

    layout->addLayout(buttonLayout);

    return group;
}

QWidget* MaterialMovementTab::createEquipmentUsagePanel()
{
    QGroupBox *group = new QGroupBox("Add Equipment Usage");
    QVBoxLayout *layout = new QVBoxLayout(group);

    // Equipment selection
    QFormLayout *selectLayout = new QFormLayout();

    m_equipmentCombo = new QComboBox();
    connect(m_equipmentCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MaterialMovementTab::onEquipmentChanged);
    selectLayout->addRow("Equipment:", m_equipmentCombo);

    // Role display (auto-detected, read-only)
    m_roleLabel = new QLabel("-");
    m_roleLabel->setStyleSheet("font-weight: bold;");
    selectLayout->addRow("Role:", m_roleLabel);

    layout->addLayout(selectLayout);

    // Equipment info display
    QGroupBox *infoBox = new QGroupBox("Equipment Info");
    QFormLayout *infoLayout = new QFormLayout(infoBox);

    m_equipCategoryLabel = new QLabel("-");
    m_equipCapacityLabel = new QLabel("-");
    m_equipFuelLabel = new QLabel("-");

    infoLayout->addRow("Category:", m_equipCategoryLabel);
    infoLayout->addRow("Capacity:", m_equipCapacityLabel);
    infoLayout->addRow("Fuel Use:", m_equipFuelLabel);

    layout->addWidget(infoBox);

    // Activity input - single field that changes based on role
    QFormLayout *activityLayout = new QFormLayout();

    m_activityLabel = new QLabel("Buckets:");
    m_activitySpinBox = new QSpinBox();
    m_activitySpinBox->setRange(0, 99999);
    connect(m_activitySpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MaterialMovementTab::onActivityCountChanged);
    activityLayout->addRow(m_activityLabel, m_activitySpinBox);

    layout->addLayout(activityLayout);

    // Calculated values display
    QGroupBox *calcBox = new QGroupBox("Calculated Values");
    QFormLayout *calcLayout = new QFormLayout(calcBox);

    m_calculatedVolumeLabel = new QLabel("0 m³");
    m_calculatedVolumeLabel->setStyleSheet("font-weight: bold; color: #2196F3;");
    calcLayout->addRow("Volume:", m_calculatedVolumeLabel);

    m_calculatedHoursLabel = new QLabel("0.00 hrs");
    m_calculatedHoursLabel->setStyleSheet("font-weight: bold; color: #4CAF50;");
    calcLayout->addRow("Est. Hours:", m_calculatedHoursLabel);

    layout->addWidget(calcBox);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_addUpdateUsageButton = new QPushButton("Add / Update Usage");
    connect(m_addUpdateUsageButton, &QPushButton::clicked,
            this, &MaterialMovementTab::onAddUpdateUsageClicked);

    m_deleteUsageButton = new QPushButton("Delete");
    m_deleteUsageButton->setEnabled(false);
    connect(m_deleteUsageButton, &QPushButton::clicked,
            this, &MaterialMovementTab::onDeleteUsageClicked);

    buttonLayout->addWidget(m_addUpdateUsageButton);
    buttonLayout->addWidget(m_deleteUsageButton);

    layout->addLayout(buttonLayout);
    layout->addStretch();

    return group;
}

QWidget* MaterialMovementTab::createSessionSummaryPanel()
{
    QGroupBox *group = new QGroupBox("Session Summary");
    QHBoxLayout *layout = new QHBoxLayout(group);

    // Volume stats
    QGroupBox *volumeBox = new QGroupBox("Volume Moved");
    QFormLayout *volumeLayout = new QFormLayout(volumeBox);

    m_totalVolumeLabel = new QLabel("0");
    m_totalVolumeLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
    m_truckVolumeLabel = new QLabel("0");
    m_loaderVolumeLabel = new QLabel("0");

    volumeLayout->addRow("Total:", m_totalVolumeLabel);
    volumeLayout->addRow("By Trucks:", m_truckVolumeLabel);
    volumeLayout->addRow("By Loaders:", m_loaderVolumeLabel);

    layout->addWidget(volumeBox);

    // Operations stats
    QGroupBox *opsBox = new QGroupBox("Operations");
    QFormLayout *opsLayout = new QFormLayout(opsBox);

    m_totalHoursLabel = new QLabel("0 hrs");
    m_totalFuelLabel = new QLabel("0");

    opsLayout->addRow("Total Hours:", m_totalHoursLabel);
    opsLayout->addRow("Est. Fuel:", m_totalFuelLabel);

    layout->addWidget(opsBox);

    // Actions
    QGroupBox *actionsBox = new QGroupBox("Actions");
    QVBoxLayout *actionsLayout = new QVBoxLayout(actionsBox);

    m_generateFuelLogButton = new QPushButton("Generate Fuel Log Entries");
    m_generateFuelLogButton->setToolTip("Create fuel log entries from this session's equipment usage");
    connect(m_generateFuelLogButton, &QPushButton::clicked,
            this, &MaterialMovementTab::onGenerateFuelLogClicked);

    actionsLayout->addWidget(m_generateFuelLogButton);
    actionsLayout->addStretch();

    layout->addWidget(actionsBox);
    layout->addStretch();

    return group;
}

void MaterialMovementTab::loadEquipmentCombo()
{
    m_equipmentCombo->clear();
    m_equipmentCombo->addItem("-- Select Equipment --", "");

    auto vehicles = m_manager->getActiveVehicles();
    for (const auto &vehicle : vehicles) {
        m_equipmentCombo->addItem(
            QString("%1 (%2)").arg(vehicle.name, vehicle.categoryMain),
            vehicle.id
            );
    }
}

void MaterialMovementTab::loadSessionHistory()
{
    m_sessionHistoryCombo->clear();
    m_sessionHistoryCombo->addItem("-- Select Session --", -1);

    auto sessions = m_manager->getAllSessions();
    for (const auto &session : sessions) {
        QString status = session.endTime.isValid() ? "Closed" : "Active";
        QString text = QString("#%1 - %2 [%3]")
                           .arg(session.id.value())
                           .arg(session.startTime.toString("yyyy-MM-dd hh:mm"))
                           .arg(status);

        if (!session.mapName.isEmpty()) {
            text += QString(" - %1").arg(session.mapName);
        }

        m_sessionHistoryCombo->addItem(text, session.id.value());
    }
}

void MaterialMovementTab::loadSession(int sessionId)
{
    auto session = m_manager->getSession(sessionId);
    if (!session.has_value()) {
        clearSession();
        return;
    }

    m_currentSessionId = sessionId;

    m_sessionIdLabel->setText(QString("#%1").arg(sessionId));
    m_mapNameEdit->setText(session->mapName);
    m_startTimeEdit->setDateTime(session->startTime);

    if (session->endTime.isValid()) {
        m_endTimeEdit->setDateTime(session->endTime);
    } else {
        m_endTimeEdit->clear();
    }

    m_sessionNotesEdit->setPlainText(session->notes);

    loadEquipmentUsage();
    updateSessionControls();
    updateSummary();
}

void MaterialMovementTab::loadEquipmentUsage()
{
    using UC = Frontier::UnitConverter;
    Frontier::UnitSystem units = m_manager->unitSystem();

    m_usageModel->clear();

    // Set headers
    QStringList headers;
    headers << "Equipment" << "Role" << "Count" << "Volume" << "Hours" << "Est. Fuel";
    m_usageModel->setHorizontalHeaderLabels(headers);

    if (!m_currentSessionId.has_value()) {
        return;
    }

    auto usages = m_manager->getEquipmentUsageForSession(m_currentSessionId.value());

    for (const auto &usage : usages) {
        QList<QStandardItem*> row;

        // Equipment name
        QString equipName = usage.equipmentId;
        auto vehicle = m_manager->getVehicle(usage.equipmentId);
        if (vehicle.has_value()) {
            equipName = vehicle->name;
        }
        QStandardItem *equipItem = new QStandardItem(equipName);
        equipItem->setData(usage.id.value(), Qt::UserRole);  // Store usage ID
        equipItem->setData(usage.equipmentId, Qt::UserRole + 1);  // Store equipment ID
        row << equipItem;

        // Role
        QString roleDisplay = (usage.role == "HaulTruck") ? "Hauler" : "Loader";
        row << new QStandardItem(roleDisplay);

        // Count (buckets or dumps)
        int count = (usage.role == "HaulTruck") ? usage.dumps : usage.buckets;
        QString countLabel = (usage.role == "HaulTruck") ?
                                 QString("%1 dumps").arg(count) : QString("%1 buckets").arg(count);
        QStandardItem *countItem = new QStandardItem(countLabel);
        countItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row << countItem;

        // Volume (converted)
        QStandardItem *volumeItem = new QStandardItem(UC::formatVolume(usage.volumeM3, units));
        volumeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row << volumeItem;

        // Hours
        QStandardItem *hoursItem = new QStandardItem(QString("%1 hrs").arg(usage.hoursUsed, 0, 'f', 2));
        hoursItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row << hoursItem;

        // Estimated Fuel (converted)
        QStandardItem *fuelItem = new QStandardItem(UC::formatFuel(usage.estimatedFuelL, units));
        fuelItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        row << fuelItem;

        m_usageModel->appendRow(row);
    }

    m_usageTableView->resizeColumnsToContents();
}

void MaterialMovementTab::updateEquipmentInfo()
{
    using UC = Frontier::UnitConverter;
    Frontier::UnitSystem units = m_manager->unitSystem();

    QString equipmentId = m_equipmentCombo->currentData().toString();

    if (equipmentId.isEmpty()) {
        m_equipCategoryLabel->setText("-");
        m_equipCapacityLabel->setText("-");
        m_equipFuelLabel->setText("-");
        m_roleLabel->setText("-");
        m_activityLabel->setText("Count:");
        m_currentRole = "Loader";
        return;
    }

    auto spec = m_manager->getVehicle(equipmentId);
    if (!spec.has_value()) return;

    // Auto-detect role from category
    m_currentRole = m_manager->determineRoleFromCategory(spec->categoryMain);

    // Update role display
    if (m_currentRole == "HaulTruck") {
        m_roleLabel->setText("Hauler");
        m_activityLabel->setText("Dumps:");
        m_equipCapacityLabel->setText(UC::formatVolume(spec->truckCapacityM3, units));
    } else {
        m_roleLabel->setText("Loader");
        m_activityLabel->setText("Buckets:");
        m_equipCapacityLabel->setText(UC::formatVolume(spec->bucketCapacityM3, units));
    }

    m_equipCategoryLabel->setText(spec->categoryMain);
    m_equipFuelLabel->setText(UC::formatFuelRate(spec->fuelUseLPerHour, units));

    // Update calculated values
    updateCalculatedValues();
}

void MaterialMovementTab::updateCalculatedValues()
{
    using UC = Frontier::UnitConverter;
    Frontier::UnitSystem units = m_manager->unitSystem();

    QString equipmentId = m_equipmentCombo->currentData().toString();
    int count = m_activitySpinBox->value();

    if (equipmentId.isEmpty() || count == 0) {
        m_calculatedVolumeLabel->setText("0 " + UC::volumeUnitLabel(units));
        m_calculatedHoursLabel->setText("0.00 hrs");
        return;
    }

    auto spec = m_manager->getVehicle(equipmentId);
    if (!spec.has_value()) return;

    // Calculate volume
    double volumeM3 = 0;
    if (m_currentRole == "HaulTruck") {
        volumeM3 = count * spec->truckCapacityM3;
    } else {
        volumeM3 = count * spec->bucketCapacityM3;
    }

    // Calculate hours from activity
    double hours = m_manager->calculateHoursFromActivity(m_currentRole, count);

    m_calculatedVolumeLabel->setText(UC::formatVolume(volumeM3, units));
    m_calculatedHoursLabel->setText(QString("%1 hrs").arg(hours, 0, 'f', 2));
}

void MaterialMovementTab::updateSummary()
{
    using UC = Frontier::UnitConverter;
    Frontier::UnitSystem units = m_manager->unitSystem();

    if (!m_currentSessionId.has_value()) {
        m_totalVolumeLabel->setText("0 " + UC::volumeUnitLabel(units));
        m_truckVolumeLabel->setText("0 " + UC::volumeUnitLabel(units));
        m_loaderVolumeLabel->setText("0 " + UC::volumeUnitLabel(units));
        m_totalHoursLabel->setText("0 hrs");
        m_totalFuelLabel->setText("0 " + UC::fuelUnitLabel(units));
        return;
    }

    auto usages = m_manager->getEquipmentUsageForSession(m_currentSessionId.value());

    double totalVolumeM3 = 0;
    double truckVolumeM3 = 0;
    double loaderVolumeM3 = 0;
    double totalHours = 0;
    double totalFuelL = 0;

    for (const auto &usage : usages) {
        totalVolumeM3 += usage.volumeM3;
        totalHours += usage.hoursUsed;
        totalFuelL += usage.estimatedFuelL;

        if (usage.role == "HaulTruck") {
            truckVolumeM3 += usage.volumeM3;
        } else {
            loaderVolumeM3 += usage.volumeM3;
        }
    }

    m_totalVolumeLabel->setText(UC::formatVolume(totalVolumeM3, units));
    m_truckVolumeLabel->setText(UC::formatVolume(truckVolumeM3, units));
    m_loaderVolumeLabel->setText(UC::formatVolume(loaderVolumeM3, units));
    m_totalHoursLabel->setText(QString("%1 hrs").arg(totalHours, 0, 'f', 2));
    m_totalFuelLabel->setText(UC::formatFuel(totalFuelL, units));
}

void MaterialMovementTab::updateSessionControls()
{
    bool hasSession = m_currentSessionId.has_value();
    bool sessionActive = false;

    if (hasSession) {
        auto session = m_manager->getSession(m_currentSessionId.value());
        sessionActive = session.has_value() && !session->endTime.isValid();
    }

    // Session controls
    m_startSessionButton->setEnabled(!sessionActive);
    m_endSessionButton->setEnabled(sessionActive);
    m_saveSessionButton->setEnabled(hasSession);

    // Equipment usage controls
    m_equipmentCombo->setEnabled(sessionActive);
    m_activitySpinBox->setEnabled(sessionActive);
    m_addUpdateUsageButton->setEnabled(sessionActive);

    // Fuel log generation
    m_generateFuelLogButton->setEnabled(hasSession && !sessionActive);
}

void MaterialMovementTab::clearSession()
{
    m_currentSessionId = std::nullopt;
    m_sessionIdLabel->setText("(No active session)");
    m_mapNameEdit->clear();
    m_startTimeEdit->clear();
    m_endTimeEdit->clear();
    m_sessionNotesEdit->clear();

    m_usageModel->clear();

    updateSessionControls();
    updateSummary();
}

void MaterialMovementTab::clearUsageForm()
{
    m_equipmentCombo->setCurrentIndex(0);
    m_activitySpinBox->setValue(0);
    m_calculatedVolumeLabel->setText("0");
    m_calculatedHoursLabel->setText("0.00 hrs");
}

// === Slots ===

void MaterialMovementTab::onStartSessionClicked()
{
    QString mapName = m_mapNameEdit->text().trimmed();
    QString notes = m_sessionNotesEdit->toPlainText().trimmed();

    int sessionId = m_manager->startSession(mapName, notes);

    if (sessionId > 0) {
        loadSession(sessionId);
        loadSessionHistory();
    } else {
        QMessageBox::warning(this, "Error", "Failed to start session.");
    }
}

void MaterialMovementTab::onEndSessionClicked()
{
    if (!m_currentSessionId.has_value()) return;

    int result = QMessageBox::question(this, "End Session",
                                       "Are you sure you want to end this session?\n\n"
                                       "Equipment hours will be auto-calculated from activity counts.\n"
                                       "You can still view the session data, but cannot add new equipment usage.",
                                       QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes) {
        m_manager->endSession(m_currentSessionId.value());
        loadSession(m_currentSessionId.value());  // Reload to get end time and recalculated hours
        loadSessionHistory();

        // Check if auto-generate fuel log is enabled
        QSettings settings("FrontierMining", "Tracker");
        if (settings.value("Operations/autoGenerateFuelLog", false).toBool()) {
            onGenerateFuelLogClicked();
        }
    }
}

void MaterialMovementTab::onSaveSessionClicked()
{
    if (!m_currentSessionId.has_value()) return;

    auto session = m_manager->getSession(m_currentSessionId.value());
    if (!session.has_value()) return;

    Frontier::MovementSession updated = session.value();
    updated.mapName = m_mapNameEdit->text().trimmed();
    updated.notes = m_sessionNotesEdit->toPlainText().trimmed();

    if (m_manager->updateSession(updated)) {
        loadSessionHistory();
        QMessageBox::information(this, "Saved", "Session saved successfully.");
    } else {
        QMessageBox::warning(this, "Error", "Failed to save session.");
    }
}

void MaterialMovementTab::onLoadSessionClicked()
{
    int sessionId = m_sessionHistoryCombo->currentData().toInt();
    if (sessionId <= 0) {
        clearSession();
        return;
    }

    loadSession(sessionId);
}

void MaterialMovementTab::onEquipmentChanged(int index)
{
    Q_UNUSED(index);
    updateEquipmentInfo();
}

void MaterialMovementTab::onActivityCountChanged(int value)
{
    Q_UNUSED(value);
    updateCalculatedValues();
}

void MaterialMovementTab::onAddUpdateUsageClicked()
{
    if (!m_currentSessionId.has_value()) {
        QMessageBox::warning(this, "Error", "No active session.");
        return;
    }

    QString equipmentId = m_equipmentCombo->currentData().toString();
    if (equipmentId.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select equipment.");
        return;
    }

    int count = m_activitySpinBox->value();
    if (count == 0) {
        QMessageBox::warning(this, "Error", "Please enter a count greater than 0.");
        return;
    }

    Frontier::MovementEquipmentUsage usage;
    usage.sessionId = m_currentSessionId.value();
    usage.equipmentId = equipmentId;
    usage.role = m_currentRole;

    // Set count based on role
    if (m_currentRole == "HaulTruck") {
        usage.dumps = count;
        usage.buckets = 0;
    } else {
        usage.buckets = count;
        usage.dumps = 0;
    }
    usage.loads = 0;  // Deprecated

    // Hours and fuel will be calculated by manager
    m_manager->addOrUpdateEquipmentUsage(usage);

    clearUsageForm();
    loadEquipmentUsage();
    updateSummary();
}

void MaterialMovementTab::onDeleteUsageClicked()
{
    QModelIndexList selection = m_usageTableView->selectionModel()->selectedRows();
    if (selection.isEmpty()) return;

    int usageId = m_usageModel->item(selection.first().row(), 0)->data(Qt::UserRole).toInt();

    int result = QMessageBox::question(this, "Delete Usage",
                                       "Are you sure you want to delete this equipment usage entry?",
                                       QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes) {
        m_manager->deleteEquipmentUsage(usageId);
        loadEquipmentUsage();
        updateSummary();
    }
}

void MaterialMovementTab::onUsageSelectionChanged()
{
    QModelIndexList selection = m_usageTableView->selectionModel()->selectedRows();
    bool hasSelection = !selection.isEmpty();

    // Check if session is still active
    bool sessionActive = false;
    if (m_currentSessionId.has_value()) {
        auto session = m_manager->getSession(m_currentSessionId.value());
        sessionActive = session.has_value() && !session->endTime.isValid();
    }

    m_deleteUsageButton->setEnabled(hasSelection && sessionActive);

    if (hasSelection && sessionActive) {
        // Populate form with selected usage for editing
        int row = selection.first().row();
        QString equipmentId = m_usageModel->item(row, 0)->data(Qt::UserRole + 1).toString();

        // Find and select equipment in combo
        int equipIndex = m_equipmentCombo->findData(equipmentId);
        if (equipIndex >= 0) {
            m_equipmentCombo->setCurrentIndex(equipIndex);
        }

        // Get the count from the usage
        auto usages = m_manager->getEquipmentUsageForSession(m_currentSessionId.value());
        int usageId = m_usageModel->item(row, 0)->data(Qt::UserRole).toInt();
        for (const auto &usage : usages) {
            if (usage.id.has_value() && usage.id.value() == usageId) {
                int count = (usage.role == "HaulTruck") ? usage.dumps : usage.buckets;
                m_activitySpinBox->setValue(count);
                break;
            }
        }
    }
}

void MaterialMovementTab::onUnitSystemChanged(Frontier::UnitSystem system)
{
    Q_UNUSED(system);
    loadEquipmentUsage();
    updateEquipmentInfo();
    updateSummary();
}

void MaterialMovementTab::onSessionStarted(int sessionId)
{
    loadSession(sessionId);
}

void MaterialMovementTab::onSessionEnded(int sessionId)
{
    if (m_currentSessionId.has_value() && m_currentSessionId.value() == sessionId) {
        loadSession(sessionId);
    }
}

void MaterialMovementTab::onEquipmentUsageUpdated(int sessionId)
{
    if (m_currentSessionId.has_value() && m_currentSessionId.value() == sessionId) {
        loadEquipmentUsage();
        updateSummary();
    }
}

void MaterialMovementTab::onGenerateFuelLogClicked()
{
    if (!m_currentSessionId.has_value()) return;

    int result = QMessageBox::question(this, "Generate Fuel Log",
                                       "This will create fuel log entries for all equipment used in this session.\n\n"
                                       "Continue?",
                                       QMessageBox::Yes | QMessageBox::No);

    if (result == QMessageBox::Yes) {
        m_manager->generateFuelLogFromSession(m_currentSessionId.value());
        QMessageBox::information(this, "Complete",
                                 "Fuel log entries have been generated.\n\n"
                                 "You can view them in the Fuel Log tab.");
    }
}
