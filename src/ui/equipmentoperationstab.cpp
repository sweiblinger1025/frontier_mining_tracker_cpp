/**
 * @file equipmentoperationstab.cpp
 * @brief Implementation of Equipment Operations subtab
 */

#include "equipmentoperationstab.h"
#include "core/unitconverter.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>

EquipmentOperationsTab::EquipmentOperationsTab(Frontier::OperationsManager *manager,
                                               QWidget *parent)
    : QWidget(parent)
    , m_manager(manager)
{
    setupUi();
    loadEquipment();

    // Listen for unit system changes
    connect(m_manager, &Frontier::OperationsManager::unitSystemChanged,
            this, &EquipmentOperationsTab::onUnitSystemChanged);
}

EquipmentOperationsTab::~EquipmentOperationsTab()
{
}

void EquipmentOperationsTab::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(15);

    // Equipment Selection
    QGroupBox *selectionGroup = new QGroupBox("Select Equipment");
    QVBoxLayout *selectionLayout = new QVBoxLayout(selectionGroup);

    m_equipmentCombo = new QComboBox();
    m_equipmentCombo->addItem("-- Select Equipment --", "");
    selectionLayout->addWidget(m_equipmentCombo);

    mainLayout->addWidget(selectionGroup);

    // Equipment Info
    QGroupBox *infoGroup = new QGroupBox("Equipment Information");
    QFormLayout *infoLayout = new QFormLayout(infoGroup);

    m_categoryLabel = new QLabel("-");
    m_bucketCapacityLabel = new QLabel("-");
    m_truckCapacityLabel = new QLabel("-");
    m_tankCapacityLabel = new QLabel("-");
    m_fuelUseLabel = new QLabel("-");

    infoLayout->addRow("Category:", m_categoryLabel);
    infoLayout->addRow("Bucket Capacity:", m_bucketCapacityLabel);
    infoLayout->addRow("Truck Capacity:", m_truckCapacityLabel);
    infoLayout->addRow("Tank Capacity:", m_tankCapacityLabel);
    infoLayout->addRow("Fuel Consumption:", m_fuelUseLabel);

    mainLayout->addWidget(infoGroup);

    // Usage Entry
    QGroupBox *usageGroup = new QGroupBox("Usage Entry");
    QFormLayout *usageLayout = new QFormLayout(usageGroup);

    m_hoursSpinBox = new QDoubleSpinBox();
    m_hoursSpinBox->setRange(0, 24);
    m_hoursSpinBox->setDecimals(2);
    m_hoursSpinBox->setSuffix(" hrs");
    usageLayout->addRow("Hours Used:", m_hoursSpinBox);

    m_notesEdit = new QTextEdit();
    m_notesEdit->setMaximumHeight(80);
    m_notesEdit->setPlaceholderText("Optional notes...");
    usageLayout->addRow("Notes:", m_notesEdit);

    mainLayout->addWidget(usageGroup);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_saveButton = new QPushButton("Save Usage");
    m_saveButton->setEnabled(false);
    buttonLayout->addWidget(m_saveButton);

    mainLayout->addLayout(buttonLayout);
    mainLayout->addStretch();

    // Connections
    connect(m_equipmentCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &EquipmentOperationsTab::onEquipmentChanged);
    connect(m_saveButton, &QPushButton::clicked,
            this, &EquipmentOperationsTab::onSaveUsage);
}

void EquipmentOperationsTab::loadEquipment()
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

void EquipmentOperationsTab::onEquipmentChanged(int index)
{
    Q_UNUSED(index);
    updateEquipmentInfo();
    m_saveButton->setEnabled(!m_equipmentCombo->currentData().toString().isEmpty());
}

void EquipmentOperationsTab::onUnitSystemChanged(Frontier::UnitSystem system)
{
    Q_UNUSED(system);
    updateEquipmentInfo();
}

void EquipmentOperationsTab::updateEquipmentInfo()
{
    QString equipmentId = m_equipmentCombo->currentData().toString();

    if (equipmentId.isEmpty()) {
        m_categoryLabel->setText("-");
        m_bucketCapacityLabel->setText("-");
        m_truckCapacityLabel->setText("-");
        m_tankCapacityLabel->setText("-");
        m_fuelUseLabel->setText("-");
        return;
    }

    auto spec = m_manager->getVehicle(equipmentId);
    if (!spec.has_value()) return;

    using UC = Frontier::UnitConverter;
    Frontier::UnitSystem units = m_manager->unitSystem();

    m_categoryLabel->setText(QString("%1 - %2").arg(spec->categoryMain, spec->categorySub));

    if (spec->bucketCapacityM3 > 0) {
        m_bucketCapacityLabel->setText(UC::formatVolume(spec->bucketCapacityM3, units));
    } else {
        m_bucketCapacityLabel->setText("N/A");
    }

    if (spec->truckCapacityM3 > 0) {
        m_truckCapacityLabel->setText(UC::formatVolume(spec->truckCapacityM3, units));
    } else {
        m_truckCapacityLabel->setText("N/A");
    }

    if (spec->tankCapacityL > 0) {
        m_tankCapacityLabel->setText(UC::formatFuel(spec->tankCapacityL, units));
    } else {
        m_tankCapacityLabel->setText("N/A");
    }

    m_fuelUseLabel->setText(UC::formatFuelRate(spec->fuelUseLPerHour, units));
}

void EquipmentOperationsTab::onSaveUsage()
{
    QMessageBox::information(this, "Save Usage",
                             "Usage tracking will be integrated with Material Movement.\n\n"
                             "For now, please use the Material Movement tab to track equipment usage within sessions.");
}
