/**
 * @file shiftlogtab.cpp
 * @brief Shift Log implementation - gameplay session diary
 */

#include "shiftlogtab.h"
#include "core/database.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QHeaderView>
#include <QFrame>
#include <QMessageBox>
#include <QSplitter>

// =============================================================================
// Constructor & Setup
// =============================================================================

ShiftLogTab::ShiftLogTab(Frontier::Database *database, QWidget *parent)
    : QWidget(parent)
    , m_database(database)
{
    setupUi();
    refreshData();
}

void ShiftLogTab::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(10);

    // Top: Summary
    mainLayout->addWidget(createSummaryPanel());

    // Main splitter: Entry form + History
    auto *splitter = new QSplitter(Qt::Horizontal);

    // Left: Entry form
    splitter->addWidget(createEntryPanel());

    // Right: History table
    auto *historyGroup = new QGroupBox(tr("Shift History"));
    auto *historyLayout = new QVBoxLayout(historyGroup);

    m_historyTable = new QTableWidget();
    m_historyTable->setColumnCount(6);
    m_historyTable->setHorizontalHeaderLabels({
        tr("Date"), tr("Start"), tr("End"), tr("Duration"), tr("Weather"), tr("Activities")
    });
    m_historyTable->setAlternatingRowColors(true);
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_historyTable->setSortingEnabled(true);
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->verticalHeader()->setVisible(false);

    auto *header = m_historyTable->horizontalHeader();
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);  // Date
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);  // Start
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);  // End
    header->setSectionResizeMode(3, QHeaderView::ResizeToContents);  // Duration
    header->setSectionResizeMode(4, QHeaderView::ResizeToContents);  // Weather
    header->setSectionResizeMode(5, QHeaderView::Stretch);           // Activities

    connect(m_historyTable, &QTableWidget::itemSelectionChanged,
            this, &ShiftLogTab::onSelectionChanged);
    connect(m_historyTable, &QTableWidget::itemDoubleClicked,
            this, &ShiftLogTab::onEditShift);

    historyLayout->addWidget(m_historyTable);

    // History buttons
    auto *historyBtnLayout = new QHBoxLayout();
    historyBtnLayout->addStretch();

    m_editBtn = new QPushButton(tr("Edit"));
    m_editBtn->setEnabled(false);
    connect(m_editBtn, &QPushButton::clicked, this, &ShiftLogTab::onEditShift);
    historyBtnLayout->addWidget(m_editBtn);

    m_deleteBtn = new QPushButton(tr("Delete"));
    m_deleteBtn->setEnabled(false);
    connect(m_deleteBtn, &QPushButton::clicked, this, &ShiftLogTab::onDeleteShift);
    historyBtnLayout->addWidget(m_deleteBtn);

    historyLayout->addLayout(historyBtnLayout);

    splitter->addWidget(historyGroup);
    splitter->setSizes({350, 650});

    mainLayout->addWidget(splitter, 1);
}

QWidget* ShiftLogTab::createEntryPanel()
{
    auto *group = new QGroupBox(tr("Log Shift"));
    auto *layout = new QVBoxLayout(group);

    auto *formLayout = new QFormLayout();

    // Start time
    m_startTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime());
    m_startTimeEdit->setCalendarPopup(true);
    m_startTimeEdit->setDisplayFormat("yyyy-MM-dd hh:mm");
    formLayout->addRow(tr("Start:"), m_startTimeEdit);

    // End time
    m_endTimeEdit = new QDateTimeEdit();
    m_endTimeEdit->setCalendarPopup(true);
    m_endTimeEdit->setDisplayFormat("yyyy-MM-dd hh:mm");
    m_endTimeEdit->setSpecialValueText(tr("(ongoing)"));
    m_endTimeEdit->setDateTime(m_endTimeEdit->minimumDateTime());  // Show as ongoing
    formLayout->addRow(tr("End:"), m_endTimeEdit);

    // Quick buttons for start/end
    auto *quickBtnLayout = new QHBoxLayout();
    m_startShiftBtn = new QPushButton(tr("Start Now"));
    m_startShiftBtn->setToolTip(tr("Set start time to current time"));
    connect(m_startShiftBtn, &QPushButton::clicked, this, &ShiftLogTab::onStartShift);
    quickBtnLayout->addWidget(m_startShiftBtn);

    m_endShiftBtn = new QPushButton(tr("End Now"));
    m_endShiftBtn->setToolTip(tr("Set end time to current time"));
    connect(m_endShiftBtn, &QPushButton::clicked, this, &ShiftLogTab::onEndShift);
    quickBtnLayout->addWidget(m_endShiftBtn);
    quickBtnLayout->addStretch();
    formLayout->addRow("", quickBtnLayout);

    // Weather
    m_weatherCombo = new QComboBox();
    m_weatherCombo->setEditable(true);
    m_weatherCombo->addItems({
        "", "Clear", "Cloudy", "Overcast", "Light Rain", "Rain", "Heavy Rain",
        "Storm", "Fog", "Snow", "Windy"
    });
    formLayout->addRow(tr("Weather:"), m_weatherCombo);

    layout->addLayout(formLayout);

    // Activities
    auto *activitiesLabel = new QLabel(tr("What I did:"));
    layout->addWidget(activitiesLabel);

    m_activitiesEdit = new QTextEdit();
    m_activitiesEdit->setPlaceholderText(tr("Mining at QRY-01, hauled 50 loads of iron ore..."));
    m_activitiesEdit->setMaximumHeight(80);
    layout->addWidget(m_activitiesEdit);

    // Notes
    auto *notesLabel = new QLabel(tr("Issues / Thoughts:"));
    layout->addWidget(notesLabel);

    m_notesEdit = new QTextEdit();
    m_notesEdit->setPlaceholderText(tr("Truck got stuck near the ramp, need to fix that route..."));
    m_notesEdit->setMaximumHeight(80);
    layout->addWidget(m_notesEdit);

    // Action buttons
    auto *btnLayout = new QHBoxLayout();

    m_newBtn = new QPushButton(tr("New"));
    m_newBtn->setToolTip(tr("Clear form for a new shift"));
    connect(m_newBtn, &QPushButton::clicked, this, &ShiftLogTab::onNewShift);
    btnLayout->addWidget(m_newBtn);

    btnLayout->addStretch();

    m_saveBtn = new QPushButton(tr("Save Shift"));
    m_saveBtn->setStyleSheet("background-color: #4caf50; color: white; font-weight: bold; padding: 8px 16px;");
    connect(m_saveBtn, &QPushButton::clicked, this, &ShiftLogTab::onSaveShift);
    btnLayout->addWidget(m_saveBtn);

    layout->addLayout(btnLayout);

    return group;
}

QWidget* ShiftLogTab::createSummaryPanel()
{
    auto *group = new QGroupBox(tr("Shift Summary"));
    auto *layout = new QHBoxLayout(group);

    // Total Shifts
    auto *shiftsLayout = new QVBoxLayout();
    shiftsLayout->addWidget(new QLabel(tr("Total Shifts")));
    m_totalShiftsLabel = new QLabel("0");
    m_totalShiftsLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    shiftsLayout->addWidget(m_totalShiftsLabel);
    layout->addLayout(shiftsLayout);

    // Separator
    auto *sep1 = new QFrame();
    sep1->setFrameShape(QFrame::VLine);
    layout->addWidget(sep1);

    // Total Time
    auto *timeLayout = new QVBoxLayout();
    timeLayout->addWidget(new QLabel(tr("Total Play Time")));
    m_totalTimeLabel = new QLabel("0h 0m");
    m_totalTimeLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #1976d2;");
    timeLayout->addWidget(m_totalTimeLabel);
    layout->addLayout(timeLayout);

    // Separator
    auto *sep2 = new QFrame();
    sep2->setFrameShape(QFrame::VLine);
    layout->addWidget(sep2);

    // Avg Duration
    auto *avgLayout = new QVBoxLayout();
    avgLayout->addWidget(new QLabel(tr("Avg Shift Duration")));
    m_avgDurationLabel = new QLabel("0m");
    m_avgDurationLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    avgLayout->addWidget(m_avgDurationLabel);
    layout->addLayout(avgLayout);

    layout->addStretch();

    return group;
}

// =============================================================================
// Data Loading
// =============================================================================

void ShiftLogTab::refreshData()
{
    loadShifts();
    updateSummary();
}

void ShiftLogTab::loadShifts()
{
    m_shifts = m_database->getAllShifts();

    m_historyTable->setSortingEnabled(false);
    m_historyTable->setRowCount(m_shifts.size());

    for (int row = 0; row < m_shifts.size(); ++row) {
        const auto &shift = m_shifts[row];

        // Date
        auto *dateItem = new QTableWidgetItem(shift.startTime.date().toString("yyyy-MM-dd"));
        dateItem->setData(Qt::UserRole, shift.id.value_or(0));
        m_historyTable->setItem(row, 0, dateItem);

        // Start time
        m_historyTable->setItem(row, 1, new QTableWidgetItem(shift.startTime.time().toString("hh:mm")));

        // End time
        QString endStr = shift.endTime.isValid() ? shift.endTime.time().toString("hh:mm") : tr("(ongoing)");
        auto *endItem = new QTableWidgetItem(endStr);
        if (!shift.endTime.isValid()) {
            endItem->setForeground(QColor("#f57c00"));
            endItem->setFont(QFont(endItem->font().family(), -1, -1, true));  // Italic
        }
        m_historyTable->setItem(row, 2, endItem);

        // Duration
        QString durationStr = shift.endTime.isValid() ? shift.durationFormatted() : "-";
        auto *durationItem = new QTableWidgetItem(durationStr);
        durationItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_historyTable->setItem(row, 3, durationItem);

        // Weather
        m_historyTable->setItem(row, 4, new QTableWidgetItem(shift.weather));

        // Activities (truncated)
        QString activities = shift.activities;
        if (activities.length() > 50) {
            activities = activities.left(47) + "...";
        }
        m_historyTable->setItem(row, 5, new QTableWidgetItem(activities));

        // Color ongoing shifts
        if (!shift.endTime.isValid()) {
            QColor bgColor(255, 243, 224);  // Light orange
            for (int col = 0; col < 6; ++col) {
                if (auto *item = m_historyTable->item(row, col)) {
                    item->setBackground(bgColor);
                }
            }
        }
    }

    m_historyTable->setSortingEnabled(true);
}

void ShiftLogTab::updateSummary()
{
    int totalShifts = m_shifts.size();
    int totalMinutes = 0;

    for (const auto &shift : m_shifts) {
        if (shift.endTime.isValid()) {
            totalMinutes += shift.durationMinutes();
        }
    }

    int hours = totalMinutes / 60;
    int mins = totalMinutes % 60;

    m_totalShiftsLabel->setText(QString::number(totalShifts));
    m_totalTimeLabel->setText(QString("%1h %2m").arg(hours).arg(mins));

    // Average duration (only count completed shifts)
    int completedShifts = 0;
    for (const auto &shift : m_shifts) {
        if (shift.endTime.isValid()) {
            completedShifts++;
        }
    }

    if (completedShifts > 0) {
        int avgMins = totalMinutes / completedShifts;
        int avgHours = avgMins / 60;
        int avgRemain = avgMins % 60;
        if (avgHours > 0) {
            m_avgDurationLabel->setText(QString("%1h %2m").arg(avgHours).arg(avgRemain));
        } else {
            m_avgDurationLabel->setText(QString("%1m").arg(avgMins));
        }
    } else {
        m_avgDurationLabel->setText("-");
    }
}

// =============================================================================
// Form Handling
// =============================================================================

void ShiftLogTab::clearForm()
{
    m_editingShiftId = std::nullopt;
    m_startTimeEdit->setDateTime(QDateTime::currentDateTime());
    m_endTimeEdit->setDateTime(m_endTimeEdit->minimumDateTime());
    m_weatherCombo->setCurrentIndex(0);
    m_activitiesEdit->clear();
    m_notesEdit->clear();
    m_saveBtn->setText(tr("Save Shift"));
}

void ShiftLogTab::populateForm(const Frontier::Shift &shift)
{
    m_editingShiftId = shift.id;
    m_startTimeEdit->setDateTime(shift.startTime);

    if (shift.endTime.isValid()) {
        m_endTimeEdit->setDateTime(shift.endTime);
    } else {
        m_endTimeEdit->setDateTime(m_endTimeEdit->minimumDateTime());
    }

    int weatherIdx = m_weatherCombo->findText(shift.weather);
    if (weatherIdx >= 0) {
        m_weatherCombo->setCurrentIndex(weatherIdx);
    } else {
        m_weatherCombo->setCurrentText(shift.weather);
    }

    m_activitiesEdit->setPlainText(shift.activities);
    m_notesEdit->setPlainText(shift.notes);
    m_saveBtn->setText(tr("Update Shift"));
}

Frontier::Shift ShiftLogTab::getFormData() const
{
    Frontier::Shift shift;
    shift.id = m_editingShiftId;
    shift.startTime = m_startTimeEdit->dateTime();

    // Check if end time is set (not at minimum)
    if (m_endTimeEdit->dateTime() > m_endTimeEdit->minimumDateTime()) {
        shift.endTime = m_endTimeEdit->dateTime();
    }

    shift.weather = m_weatherCombo->currentText();
    shift.activities = m_activitiesEdit->toPlainText();
    shift.notes = m_notesEdit->toPlainText();

    return shift;
}

// =============================================================================
// Slots
// =============================================================================

void ShiftLogTab::onStartShift()
{
    m_startTimeEdit->setDateTime(QDateTime::currentDateTime());
}

void ShiftLogTab::onEndShift()
{
    m_endTimeEdit->setDateTime(QDateTime::currentDateTime());
}

void ShiftLogTab::onSaveShift()
{
    Frontier::Shift shift = getFormData();

    // Validate
    if (!shift.startTime.isValid()) {
        QMessageBox::warning(this, tr("Invalid Data"), tr("Please set a valid start time."));
        return;
    }

    if (shift.endTime.isValid() && shift.endTime < shift.startTime) {
        QMessageBox::warning(this, tr("Invalid Data"), tr("End time cannot be before start time."));
        return;
    }

    bool success = false;
    if (shift.id.has_value()) {
        // Update existing
        success = m_database->updateShift(shift);
    } else {
        // Add new
        int id = m_database->addShift(shift);
        success = (id > 0);
    }

    if (success) {
        clearForm();
        refreshData();
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Failed to save shift."));
    }
}

void ShiftLogTab::onNewShift()
{
    clearForm();
    m_historyTable->clearSelection();
}

void ShiftLogTab::onEditShift()
{
    auto selected = m_historyTable->selectedItems();
    if (selected.isEmpty()) return;

    int row = selected.first()->row();
    int shiftId = m_historyTable->item(row, 0)->data(Qt::UserRole).toInt();

    auto shift = m_database->getShift(shiftId);
    if (shift.has_value()) {
        populateForm(shift.value());
    }
}

void ShiftLogTab::onDeleteShift()
{
    auto selected = m_historyTable->selectedItems();
    if (selected.isEmpty()) return;

    int row = selected.first()->row();
    int shiftId = m_historyTable->item(row, 0)->data(Qt::UserRole).toInt();

    auto result = QMessageBox::question(this, tr("Delete Shift"),
                                        tr("Delete this shift from the log?"));

    if (result == QMessageBox::Yes) {
        if (m_database->deleteShift(shiftId)) {
            // If we were editing this shift, clear the form
            if (m_editingShiftId.has_value() && m_editingShiftId.value() == shiftId) {
                clearForm();
            }
            refreshData();
        }
    }
}

void ShiftLogTab::onSelectionChanged()
{
    bool hasSelection = !m_historyTable->selectedItems().isEmpty();
    m_editBtn->setEnabled(hasSelection);
    m_deleteBtn->setEnabled(hasSelection);
}
