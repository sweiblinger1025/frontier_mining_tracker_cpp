/**
 * @file cycletimetab.cpp
 * @brief Cycle Time Analyzer implementation
 */

#include "cycletimetab.h"
#include "core/database.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QHeaderView>
#include <QFrame>
#include <QMessageBox>
#include <QInputDialog>
#include <QSplitter>
#include <QDialog>
#include <QDialogButtonBox>


// =============================================================================
// Constructor & Setup
// =============================================================================

CycleTimeTab::CycleTimeTab(Frontier::Database *database, QWidget *parent)
    : QWidget(parent)
    , m_database(database)
    , m_currentPhase(0)
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &CycleTimeTab::onTimerTick);

    setupUi();
    refreshData();
}

void CycleTimeTab::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(10);

    // Top: Profile selection
    mainLayout->addWidget(createProfilePanel());

    // Middle: Timer + Stats side by side
    auto *middleSplitter = new QSplitter(Qt::Horizontal);
    middleSplitter->addWidget(createTimerPanel());
    middleSplitter->addWidget(createStatsPanel());
    middleSplitter->setSizes({500, 300});
    mainLayout->addWidget(middleSplitter);

    // Bottom: History
    mainLayout->addWidget(createHistoryPanel(), 1);
}

QWidget* CycleTimeTab::createProfilePanel()
{
    auto *group = new QGroupBox(tr("Route Profile"));
    auto *layout = new QHBoxLayout(group);

    layout->addWidget(new QLabel(tr("Profile:")));

    m_profileCombo = new QComboBox();
    m_profileCombo->setMinimumWidth(250);
    connect(m_profileCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CycleTimeTab::onProfileSelected);
    layout->addWidget(m_profileCombo);

    m_newProfileBtn = new QPushButton(tr("+ New"));
    connect(m_newProfileBtn, &QPushButton::clicked, this, &CycleTimeTab::onNewProfile);
    layout->addWidget(m_newProfileBtn);

    m_editProfileBtn = new QPushButton(tr("Edit"));
    m_editProfileBtn->setEnabled(false);
    connect(m_editProfileBtn, &QPushButton::clicked, this, &CycleTimeTab::onEditProfile);
    layout->addWidget(m_editProfileBtn);

    m_deleteProfileBtn = new QPushButton(tr("Delete"));
    m_deleteProfileBtn->setEnabled(false);
    connect(m_deleteProfileBtn, &QPushButton::clicked, this, &CycleTimeTab::onDeleteProfile);
    layout->addWidget(m_deleteProfileBtn);

    layout->addStretch();

    m_profileDetailsLabel = new QLabel();
    m_profileDetailsLabel->setStyleSheet("color: #666;");
    layout->addWidget(m_profileDetailsLabel);

    return group;
}

QWidget* CycleTimeTab::createTimerPanel()
{
    auto *group = new QGroupBox(tr("Cycle Timer"));
    auto *layout = new QVBoxLayout(group);

    // Big timer display
    m_timerDisplay = new QLabel("0:00");
    m_timerDisplay->setStyleSheet("font-size: 48px; font-weight: bold; color: #1976d2;");
    m_timerDisplay->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_timerDisplay);

    // Phase indicator
    m_phaseLabel = new QLabel(tr("Ready"));
    m_phaseLabel->setStyleSheet("font-size: 16px; color: #666;");
    m_phaseLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_phaseLabel);

    // Timer control buttons
    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();

    m_startStopBtn = new QPushButton(tr("Start"));
    m_startStopBtn->setStyleSheet("background-color: #4caf50; color: white; font-weight: bold; padding: 10px 20px;");
    m_startStopBtn->setMinimumWidth(100);
    connect(m_startStopBtn, &QPushButton::clicked, this, &CycleTimeTab::onStartStop);
    btnLayout->addWidget(m_startStopBtn);

    m_nextPhaseBtn = new QPushButton(tr("Next Phase"));
    m_nextPhaseBtn->setEnabled(false);
    connect(m_nextPhaseBtn, &QPushButton::clicked, this, &CycleTimeTab::onNextPhase);
    btnLayout->addWidget(m_nextPhaseBtn);

    m_resetBtn = new QPushButton(tr("Reset"));
    connect(m_resetBtn, &QPushButton::clicked, this, &CycleTimeTab::onResetTimer);
    btnLayout->addWidget(m_resetBtn);

    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    // Phase times grid
    auto *phasesGroup = new QGroupBox(tr("Phase Times"));
    auto *phasesGrid = new QGridLayout(phasesGroup);

    phasesGrid->addWidget(new QLabel(tr("Load:")), 0, 0);
    m_loadTimeLabel = new QLabel("0:00");
    m_loadTimeLabel->setStyleSheet("font-weight: bold;");
    phasesGrid->addWidget(m_loadTimeLabel, 0, 1);

    phasesGrid->addWidget(new QLabel(tr("Haul:")), 0, 2);
    m_haulTimeLabel = new QLabel("0:00");
    m_haulTimeLabel->setStyleSheet("font-weight: bold;");
    phasesGrid->addWidget(m_haulTimeLabel, 0, 3);

    phasesGrid->addWidget(new QLabel(tr("Dump:")), 1, 0);
    m_dumpTimeLabel = new QLabel("0:00");
    m_dumpTimeLabel->setStyleSheet("font-weight: bold;");
    phasesGrid->addWidget(m_dumpTimeLabel, 1, 1);

    phasesGrid->addWidget(new QLabel(tr("Return:")), 1, 2);
    m_returnTimeLabel = new QLabel("0:00");
    m_returnTimeLabel->setStyleSheet("font-weight: bold;");
    phasesGrid->addWidget(m_returnTimeLabel, 1, 3);

    phasesGrid->addWidget(new QLabel(tr("TOTAL:")), 2, 0);
    m_totalTimeLabel = new QLabel("0:00");
    m_totalTimeLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #1976d2;");
    phasesGrid->addWidget(m_totalTimeLabel, 2, 1, 1, 3);

    layout->addWidget(phasesGroup);

    // Manual entry section
    auto *manualGroup = new QGroupBox(tr("Manual Entry"));
    auto *manualLayout = new QGridLayout(manualGroup);

    // Load
    manualLayout->addWidget(new QLabel(tr("Load:")), 0, 0);
    m_loadMinSpin = new QSpinBox();
    m_loadMinSpin->setRange(0, 59);
    m_loadMinSpin->setSuffix("m");
    manualLayout->addWidget(m_loadMinSpin, 0, 1);
    m_loadSecSpin = new QSpinBox();
    m_loadSecSpin->setRange(0, 59);
    m_loadSecSpin->setSuffix("s");
    manualLayout->addWidget(m_loadSecSpin, 0, 2);

    // Haul
    manualLayout->addWidget(new QLabel(tr("Haul:")), 0, 3);
    m_haulMinSpin = new QSpinBox();
    m_haulMinSpin->setRange(0, 59);
    m_haulMinSpin->setSuffix("m");
    manualLayout->addWidget(m_haulMinSpin, 0, 4);
    m_haulSecSpin = new QSpinBox();
    m_haulSecSpin->setRange(0, 59);
    m_haulSecSpin->setSuffix("s");
    manualLayout->addWidget(m_haulSecSpin, 0, 5);

    // Dump
    manualLayout->addWidget(new QLabel(tr("Dump:")), 1, 0);
    m_dumpMinSpin = new QSpinBox();
    m_dumpMinSpin->setRange(0, 59);
    m_dumpMinSpin->setSuffix("m");
    manualLayout->addWidget(m_dumpMinSpin, 1, 1);
    m_dumpSecSpin = new QSpinBox();
    m_dumpSecSpin->setRange(0, 59);
    m_dumpSecSpin->setSuffix("s");
    manualLayout->addWidget(m_dumpSecSpin, 1, 2);

    // Return
    manualLayout->addWidget(new QLabel(tr("Return:")), 1, 3);
    m_returnMinSpin = new QSpinBox();
    m_returnMinSpin->setRange(0, 59);
    m_returnMinSpin->setSuffix("m");
    manualLayout->addWidget(m_returnMinSpin, 1, 4);
    m_returnSecSpin = new QSpinBox();
    m_returnSecSpin->setRange(0, 59);
    m_returnSecSpin->setSuffix("s");
    manualLayout->addWidget(m_returnSecSpin, 1, 5);

    // Notes
    manualLayout->addWidget(new QLabel(tr("Notes:")), 2, 0);
    m_recordNotesEdit = new QLineEdit();
    m_recordNotesEdit->setPlaceholderText(tr("Optional notes..."));
    manualLayout->addWidget(m_recordNotesEdit, 2, 1, 1, 4);

    m_saveRecordBtn = new QPushButton(tr("Save Cycle"));
    m_saveRecordBtn->setStyleSheet("background-color: #4caf50; color: white; font-weight: bold;");
    connect(m_saveRecordBtn, &QPushButton::clicked, this, &CycleTimeTab::onSaveRecord);
    manualLayout->addWidget(m_saveRecordBtn, 2, 5);

    layout->addWidget(manualGroup);

    return group;
}

QWidget* CycleTimeTab::createStatsPanel()
{
    auto *group = new QGroupBox(tr("Profile Statistics"));
    auto *layout = new QVBoxLayout(group);

    auto *formLayout = new QFormLayout();

    m_recordCountLabel = new QLabel("0");
    m_recordCountLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    formLayout->addRow(tr("Total Records:"), m_recordCountLabel);

    m_avgTimeLabel = new QLabel("-");
    m_avgTimeLabel->setStyleSheet("font-size: 16px; font-weight: bold;");
    formLayout->addRow(tr("Average Cycle:"), m_avgTimeLabel);

    m_bestTimeLabel = new QLabel("-");
    m_bestTimeLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #2e7d32;");
    formLayout->addRow(tr("Best Cycle:"), m_bestTimeLabel);

    m_worstTimeLabel = new QLabel("-");
    m_worstTimeLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #c62828;");
    formLayout->addRow(tr("Worst Cycle:"), m_worstTimeLabel);

    layout->addLayout(formLayout);
    layout->addStretch();

    return group;
}

QWidget* CycleTimeTab::createHistoryPanel()
{
    auto *group = new QGroupBox(tr("Cycle History"));
    auto *layout = new QVBoxLayout(group);

    m_historyTable = new QTableWidget();
    m_historyTable->setColumnCount(8);
    m_historyTable->setHorizontalHeaderLabels({
        tr("Date/Time"), tr("Load"), tr("Haul"), tr("Dump"),
        tr("Return"), tr("Total"), tr("vs Avg"), tr("Notes")
    });
    m_historyTable->setAlternatingRowColors(true);
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_historyTable->setSortingEnabled(true);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->verticalHeader()->setVisible(false);

    auto *header = m_historyTable->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(6, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(7, QHeaderView::Stretch);

    connect(m_historyTable, &QTableWidget::itemSelectionChanged,
            this, &CycleTimeTab::onRecordSelected);

    layout->addWidget(m_historyTable);

    // Buttons
    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    m_deleteRecordBtn = new QPushButton(tr("Delete Record"));
    m_deleteRecordBtn->setEnabled(false);
    connect(m_deleteRecordBtn, &QPushButton::clicked, this, &CycleTimeTab::onDeleteRecord);
    btnLayout->addWidget(m_deleteRecordBtn);
    layout->addLayout(btnLayout);

    return group;
}

// =============================================================================
// Data Loading
// =============================================================================

void CycleTimeTab::refreshData()
{
    loadProfiles();
}

void CycleTimeTab::loadProfiles()
{
    m_profileCombo->blockSignals(true);
    int currentId = m_profileCombo->currentData().toInt();

    m_profileCombo->clear();
    m_profiles = m_database->getCycleProfilesWithStats();

    m_profileCombo->addItem(tr("-- Select Profile --"), 0);
    for (const auto &profile : m_profiles) {
        QString display = profile.name;
        if (profile.recordCount > 0) {
            display += QString(" (%1 records)").arg(profile.recordCount);
        }
        m_profileCombo->addItem(display, profile.id.value_or(0));
    }

    // Restore selection
    int idx = m_profileCombo->findData(currentId);
    if (idx >= 0) {
        m_profileCombo->setCurrentIndex(idx);
    }

    m_profileCombo->blockSignals(false);
    onProfileSelected();
}

void CycleTimeTab::loadRecordsForProfile(int profileId)
{
    m_records = m_database->getCycleRecordsByProfile(profileId);

    // Find the profile for avg calculation
    int avgSeconds = 0;
    for (const auto &profile : m_profiles) {
        if (profile.id.value_or(0) == profileId) {
            avgSeconds = profile.avgTotalSeconds;
            break;
        }
    }

    m_historyTable->setSortingEnabled(false);
    m_historyTable->setRowCount(m_records.size());

    for (int row = 0; row < m_records.size(); ++row) {
        const auto &record = m_records[row];

        // Date/Time
        auto *dateItem = new QTableWidgetItem(record.timestamp.toString("yyyy-MM-dd hh:mm"));
        dateItem->setData(Qt::UserRole, record.id.value_or(0));
        m_historyTable->setItem(row, 0, dateItem);

        // Load
        auto *loadItem = new QTableWidgetItem(formatSeconds(record.loadSeconds));
        loadItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_historyTable->setItem(row, 1, loadItem);

        // Haul
        auto *haulItem = new QTableWidgetItem(formatSeconds(record.haulSeconds));
        haulItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_historyTable->setItem(row, 2, haulItem);

        // Dump
        auto *dumpItem = new QTableWidgetItem(formatSeconds(record.dumpSeconds));
        dumpItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_historyTable->setItem(row, 3, dumpItem);

        // Return
        auto *returnItem = new QTableWidgetItem(formatSeconds(record.returnSeconds));
        returnItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_historyTable->setItem(row, 4, returnItem);

        // Total
        auto *totalItem = new QTableWidgetItem(formatSeconds(record.totalSeconds));
        totalItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        totalItem->setData(Qt::UserRole, record.totalSeconds);
        m_historyTable->setItem(row, 5, totalItem);

        // vs Avg
        QString vsAvg = "-";
        QColor vsColor;
        if (avgSeconds > 0) {
            int diff = record.totalSeconds - avgSeconds;
            if (diff < 0) {
                vsAvg = QString("-%1").arg(formatSeconds(-diff));
                vsColor = QColor("#2e7d32");  // Green - faster
            } else if (diff > 0) {
                vsAvg = QString("+%1").arg(formatSeconds(diff));
                vsColor = QColor("#c62828");  // Red - slower
            } else {
                vsAvg = "=";
                vsColor = QColor("#666");
            }
        }
        auto *vsItem = new QTableWidgetItem(vsAvg);
        vsItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (vsColor.isValid()) {
            vsItem->setForeground(vsColor);
        }
        m_historyTable->setItem(row, 6, vsItem);

        // Notes
        m_historyTable->setItem(row, 7, new QTableWidgetItem(record.notes));
    }

    m_historyTable->setSortingEnabled(true);
}

void CycleTimeTab::updateStats()
{
    int profileId = m_profileCombo->currentData().toInt();

    for (const auto &profile : m_profiles) {
        if (profile.id.value_or(0) == profileId) {
            m_recordCountLabel->setText(QString::number(profile.recordCount));

            if (profile.recordCount > 0) {
                m_avgTimeLabel->setText(formatSeconds(profile.avgTotalSeconds));
                m_bestTimeLabel->setText(formatSeconds(profile.bestTotalSeconds));
                m_worstTimeLabel->setText(formatSeconds(profile.worstTotalSeconds));
            } else {
                m_avgTimeLabel->setText("-");
                m_bestTimeLabel->setText("-");
                m_worstTimeLabel->setText("-");
            }

            // Update profile details
            QStringList details;
            if (!profile.sourceLocationName.isEmpty()) {
                details << QString("From: %1").arg(profile.sourceLocationName);
            }
            if (!profile.destLocationName.isEmpty()) {
                details << QString("To: %1").arg(profile.destLocationName);
            }
            if (!profile.vehicleName.isEmpty()) {
                details << QString("Vehicle: %1").arg(profile.vehicleName);
            }
            m_profileDetailsLabel->setText(details.join(" | "));

            return;
        }
    }

    // No profile selected
    m_recordCountLabel->setText("0");
    m_avgTimeLabel->setText("-");
    m_bestTimeLabel->setText("-");
    m_worstTimeLabel->setText("-");
    m_profileDetailsLabel->clear();
}

void CycleTimeTab::updateTimerDisplay()
{
    int currentSeconds = 0;
    if (m_currentPhase > 0 && m_currentPhase <= 4) {
        currentSeconds = m_phaseSeconds[m_currentPhase - 1];
    }

    m_timerDisplay->setText(formatSeconds(currentSeconds));

    m_loadTimeLabel->setText(formatSeconds(m_phaseSeconds[0]));
    m_haulTimeLabel->setText(formatSeconds(m_phaseSeconds[1]));
    m_dumpTimeLabel->setText(formatSeconds(m_phaseSeconds[2]));
    m_returnTimeLabel->setText(formatSeconds(m_phaseSeconds[3]));

    int total = m_phaseSeconds[0] + m_phaseSeconds[1] + m_phaseSeconds[2] + m_phaseSeconds[3];
    m_totalTimeLabel->setText(formatSeconds(total));

    // Update phase label
    static const QString phaseNames[] = {tr("Ready"), tr("LOADING..."), tr("HAULING..."),
                                         tr("DUMPING..."), tr("RETURNING...")};
    m_phaseLabel->setText(phaseNames[m_currentPhase]);

    // Color the current phase
    static const QString phaseColors[] = {"#666", "#f57c00", "#1976d2", "#7b1fa2", "#388e3c"};
    m_phaseLabel->setStyleSheet(QString("font-size: 16px; color: %1; font-weight: bold;")
                                    .arg(phaseColors[m_currentPhase]));
}

QString CycleTimeTab::formatSeconds(int seconds) const
{
    int mins = seconds / 60;
    int secs = seconds % 60;
    return QString("%1:%2").arg(mins).arg(secs, 2, 10, QChar('0'));
}

// =============================================================================
// Profile Management
// =============================================================================

void CycleTimeTab::onProfileSelected()
{
    int profileId = m_profileCombo->currentData().toInt();
    bool hasProfile = (profileId > 0);

    m_editProfileBtn->setEnabled(hasProfile);
    m_deleteProfileBtn->setEnabled(hasProfile);
    m_saveRecordBtn->setEnabled(hasProfile);
    m_startStopBtn->setEnabled(hasProfile);

    if (hasProfile) {
        loadRecordsForProfile(profileId);
    } else {
        m_historyTable->setRowCount(0);
        m_records.clear();
    }

    updateStats();
}

void CycleTimeTab::showProfileDialog(bool isEdit)
{
    QDialog dialog(this);
    dialog.setWindowTitle(isEdit ? tr("Edit Profile") : tr("New Profile"));
    dialog.setMinimumWidth(400);

    auto *layout = new QFormLayout(&dialog);

    auto *nameEdit = new QLineEdit();
    layout->addRow(tr("Profile Name:"), nameEdit);

    auto *sourceCombo = new QComboBox();
    sourceCombo->addItem(tr("-- None --"), 0);
    auto locations = m_database->getAllLocations();
    for (const auto &loc : locations) {
        sourceCombo->addItem(loc.name, loc.id.value_or(0));
    }
    layout->addRow(tr("Source Location:"), sourceCombo);

    auto *destCombo = new QComboBox();
    destCombo->addItem(tr("-- None --"), 0);
    for (const auto &loc : locations) {
        destCombo->addItem(loc.name, loc.id.value_or(0));
    }
    layout->addRow(tr("Destination:"), destCombo);

    auto *vehicleCombo = new QComboBox();
    vehicleCombo->addItem(tr("-- Any --"), 0);
    auto vehicles = m_database->getAllVehicles();
    for (const auto &v : vehicles) {
        vehicleCombo->addItem(v.name, v.id);
    }
    layout->addRow(tr("Vehicle:"), vehicleCombo);

    auto *notesEdit = new QLineEdit();
    layout->addRow(tr("Notes:"), notesEdit);

    // Populate if editing
    Frontier::CycleProfile profile;
    if (isEdit) {
        int profileId = m_profileCombo->currentData().toInt();
        auto existing = m_database->getCycleProfile(profileId);
        if (existing.has_value()) {
            profile = existing.value();
            nameEdit->setText(profile.name);

            int srcIdx = sourceCombo->findData(profile.sourceLocationId);
            if (srcIdx >= 0) sourceCombo->setCurrentIndex(srcIdx);

            int destIdx = destCombo->findData(profile.destLocationId);
            if (destIdx >= 0) destCombo->setCurrentIndex(destIdx);

            int vehIdx = vehicleCombo->findData(profile.vehicleId);
            if (vehIdx >= 0) vehicleCombo->setCurrentIndex(vehIdx);

            notesEdit->setText(profile.notes);
        }
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addRow(buttons);

    if (dialog.exec() == QDialog::Accepted) {
        profile.name = nameEdit->text().trimmed();
        if (profile.name.isEmpty()) {
            QMessageBox::warning(this, tr("Invalid Name"), tr("Please enter a profile name."));
            return;
        }

        profile.sourceLocationId = sourceCombo->currentData().toInt();
        profile.destLocationId = destCombo->currentData().toInt();
        profile.vehicleId = vehicleCombo->currentData().toInt();
        profile.notes = notesEdit->text();

        if (isEdit) {
            m_database->updateCycleProfile(profile);
        } else {
            m_database->addCycleProfile(profile);
        }

        loadProfiles();
    }
}

void CycleTimeTab::onNewProfile()
{
    showProfileDialog(false);
}

void CycleTimeTab::onEditProfile()
{
    showProfileDialog(true);
}

void CycleTimeTab::onDeleteProfile()
{
    int profileId = m_profileCombo->currentData().toInt();
    if (profileId <= 0) return;

    auto result = QMessageBox::question(this, tr("Delete Profile"),
                                        tr("Delete this profile and ALL its cycle records?\n\nThis cannot be undone."));

    if (result == QMessageBox::Yes) {
        m_database->deleteCycleProfile(profileId);
        loadProfiles();
    }
}

// =============================================================================
// Timer Controls
// =============================================================================

void CycleTimeTab::onStartStop()
{
    if (m_currentPhase == 0) {
        // Start timer - begin with Load phase
        m_currentPhase = 1;
        m_timer->start(1000);
        m_startStopBtn->setText(tr("Pause"));
        m_startStopBtn->setStyleSheet("background-color: #f57c00; color: white; font-weight: bold; padding: 10px 20px;");
        m_nextPhaseBtn->setEnabled(true);
    } else if (m_timer->isActive()) {
        // Pause
        m_timer->stop();
        m_startStopBtn->setText(tr("Resume"));
        m_startStopBtn->setStyleSheet("background-color: #4caf50; color: white; font-weight: bold; padding: 10px 20px;");
    } else {
        // Resume
        m_timer->start(1000);
        m_startStopBtn->setText(tr("Pause"));
        m_startStopBtn->setStyleSheet("background-color: #f57c00; color: white; font-weight: bold; padding: 10px 20px;");
    }

    updateTimerDisplay();
}

void CycleTimeTab::onNextPhase()
{
    if (m_currentPhase >= 1 && m_currentPhase < 4) {
        m_currentPhase++;
        updateTimerDisplay();
    } else if (m_currentPhase == 4) {
        // Cycle complete - stop and populate manual entry
        m_timer->stop();

        m_loadMinSpin->setValue(m_phaseSeconds[0] / 60);
        m_loadSecSpin->setValue(m_phaseSeconds[0] % 60);
        m_haulMinSpin->setValue(m_phaseSeconds[1] / 60);
        m_haulSecSpin->setValue(m_phaseSeconds[1] % 60);
        m_dumpMinSpin->setValue(m_phaseSeconds[2] / 60);
        m_dumpSecSpin->setValue(m_phaseSeconds[2] % 60);
        m_returnMinSpin->setValue(m_phaseSeconds[3] / 60);
        m_returnSecSpin->setValue(m_phaseSeconds[3] % 60);

        m_currentPhase = 0;
        m_startStopBtn->setText(tr("Start"));
        m_startStopBtn->setStyleSheet("background-color: #4caf50; color: white; font-weight: bold; padding: 10px 20px;");
        m_nextPhaseBtn->setEnabled(false);
        m_phaseLabel->setText(tr("Cycle Complete - Review & Save"));
        m_phaseLabel->setStyleSheet("font-size: 16px; color: #2e7d32; font-weight: bold;");
    }
}

void CycleTimeTab::onResetTimer()
{
    m_timer->stop();
    m_currentPhase = 0;
    for (int i = 0; i < 4; i++) {
        m_phaseSeconds[i] = 0;
    }

    m_startStopBtn->setText(tr("Start"));
    m_startStopBtn->setStyleSheet("background-color: #4caf50; color: white; font-weight: bold; padding: 10px 20px;");
    m_nextPhaseBtn->setEnabled(false);

    updateTimerDisplay();
}

void CycleTimeTab::onTimerTick()
{
    if (m_currentPhase >= 1 && m_currentPhase <= 4) {
        m_phaseSeconds[m_currentPhase - 1]++;
        updateTimerDisplay();
    }
}

// =============================================================================
// Record Management
// =============================================================================

void CycleTimeTab::onSaveRecord()
{
    int profileId = m_profileCombo->currentData().toInt();
    if (profileId <= 0) {
        QMessageBox::warning(this, tr("No Profile"), tr("Please select a profile first."));
        return;
    }

    Frontier::CycleRecord record;
    record.profileId = profileId;
    record.loadSeconds = m_loadMinSpin->value() * 60 + m_loadSecSpin->value();
    record.haulSeconds = m_haulMinSpin->value() * 60 + m_haulSecSpin->value();
    record.dumpSeconds = m_dumpMinSpin->value() * 60 + m_dumpSecSpin->value();
    record.returnSeconds = m_returnMinSpin->value() * 60 + m_returnSecSpin->value();
    record.totalSeconds = record.computeTotal();
    record.timestamp = QDateTime::currentDateTime();
    record.notes = m_recordNotesEdit->text();

    if (record.totalSeconds == 0) {
        QMessageBox::warning(this, tr("Invalid Data"), tr("Please enter cycle times."));
        return;
    }

    int id = m_database->addCycleRecord(record);
    if (id > 0) {
        // Clear manual entry
        m_loadMinSpin->setValue(0);
        m_loadSecSpin->setValue(0);
        m_haulMinSpin->setValue(0);
        m_haulSecSpin->setValue(0);
        m_dumpMinSpin->setValue(0);
        m_dumpSecSpin->setValue(0);
        m_returnMinSpin->setValue(0);
        m_returnSecSpin->setValue(0);
        m_recordNotesEdit->clear();

        // Refresh
        loadProfiles();  // To update stats
        loadRecordsForProfile(profileId);
        updateStats();
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Failed to save cycle record."));
    }
}

void CycleTimeTab::onDeleteRecord()
{
    auto selected = m_historyTable->selectedItems();
    if (selected.isEmpty()) return;

    int row = selected.first()->row();
    int recordId = m_historyTable->item(row, 0)->data(Qt::UserRole).toInt();

    auto result = QMessageBox::question(this, tr("Delete Record"),
                                        tr("Delete this cycle record?"));

    if (result == QMessageBox::Yes) {
        if (m_database->deleteCycleRecord(recordId)) {
            int profileId = m_profileCombo->currentData().toInt();
            loadProfiles();  // To update stats
            loadRecordsForProfile(profileId);
            updateStats();
        }
    }
}

void CycleTimeTab::onRecordSelected()
{
    bool hasSelection = !m_historyTable->selectedItems().isEmpty();
    m_deleteRecordBtn->setEnabled(hasSelection);
}
