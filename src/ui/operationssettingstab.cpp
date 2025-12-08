/**
 * @file operationssettingstab.cpp
 * @brief Implementation of Operations Settings subtab
 */

#include "operationssettingstab.h"
#include "core/unitconverter.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSettings>
#include <QMessageBox>

OperationsSettingsTab::OperationsSettingsTab(Frontier::OperationsManager *manager,
                                             QWidget *parent)
    : QWidget(parent)
    , m_manager(manager)
{
    setupUi();
    loadSettings();
}

OperationsSettingsTab::~OperationsSettingsTab()
{
}

void OperationsSettingsTab::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(15);

    // === Unit System ===
    QGroupBox *unitGroup = new QGroupBox("Unit System");
    QVBoxLayout *unitLayout = new QVBoxLayout(unitGroup);

    QFormLayout *unitFormLayout = new QFormLayout();

    m_unitSystemCombo = new QComboBox();
    m_unitSystemCombo->addItem("Imperial (yd³, gallons, miles)",
                               static_cast<int>(Frontier::UnitSystem::Imperial));
    m_unitSystemCombo->addItem("Metric (m³, liters, km)",
                               static_cast<int>(Frontier::UnitSystem::Metric));
    unitFormLayout->addRow("Measurement System:", m_unitSystemCombo);

    unitLayout->addLayout(unitFormLayout);

    // Unit Preview
    m_unitPreviewLabel = new QLabel();
    m_unitPreviewLabel->setStyleSheet("color: gray; font-style: italic;");
    m_unitPreviewLabel->setWordWrap(true);
    unitLayout->addWidget(m_unitPreviewLabel);

    mainLayout->addWidget(unitGroup);

    // === Cycle Times ===
    QGroupBox *cycleGroup = new QGroupBox("Cycle Time Estimates");
    QVBoxLayout *cycleLayout = new QVBoxLayout(cycleGroup);

    QLabel *cycleDesc = new QLabel(
        "Used to auto-calculate equipment hours from activity counts.\n"
        "Adjust based on your typical operation distances and efficiency."
        );
    cycleDesc->setStyleSheet("color: gray;");
    cycleDesc->setWordWrap(true);
    cycleLayout->addWidget(cycleDesc);

    QFormLayout *cycleFormLayout = new QFormLayout();

    // Loader cycle time
    m_loaderCycleTimeSpinBox = new QDoubleSpinBox();
    m_loaderCycleTimeSpinBox->setRange(0.5, 10.0);
    m_loaderCycleTimeSpinBox->setDecimals(1);
    m_loaderCycleTimeSpinBox->setSuffix(" min");
    m_loaderCycleTimeSpinBox->setToolTip("Time per bucket cycle: dig → swing → dump into truck");
    cycleFormLayout->addRow("Loader/Excavator Cycle:", m_loaderCycleTimeSpinBox);

    // Truck cycle time
    m_truckCycleTimeSpinBox = new QDoubleSpinBox();
    m_truckCycleTimeSpinBox->setRange(1.0, 30.0);
    m_truckCycleTimeSpinBox->setDecimals(1);
    m_truckCycleTimeSpinBox->setSuffix(" min");
    m_truckCycleTimeSpinBox->setToolTip("Time per haul cycle: wait for load + haul + dump + return");
    cycleFormLayout->addRow("Truck Haul Cycle:", m_truckCycleTimeSpinBox);

    cycleLayout->addLayout(cycleFormLayout);

    // Example calculation
    m_cycleTimeExampleLabel = new QLabel();
    m_cycleTimeExampleLabel->setStyleSheet("background-color: #f0f0f0; padding: 8px; border-radius: 4px;");
    m_cycleTimeExampleLabel->setWordWrap(true);
    cycleLayout->addWidget(m_cycleTimeExampleLabel);

    // Connect spinboxes to update example
    connect(m_loaderCycleTimeSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double) {
                double loaderTime = m_loaderCycleTimeSpinBox->value();
                double truckTime = m_truckCycleTimeSpinBox->value();
                double loaderHours = (50 * loaderTime) / 60.0;
                double truckHours = (8 * truckTime) / 60.0;
                m_cycleTimeExampleLabel->setText(
                    QString("Example: 50 buckets × %1 min = %2 hrs | 8 dumps × %3 min = %4 hrs")
                        .arg(loaderTime, 0, 'f', 1)
                        .arg(loaderHours, 0, 'f', 2)
                        .arg(truckTime, 0, 'f', 1)
                        .arg(truckHours, 0, 'f', 2)
                    );
            });
    connect(m_truckCycleTimeSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double) {
                double loaderTime = m_loaderCycleTimeSpinBox->value();
                double truckTime = m_truckCycleTimeSpinBox->value();
                double loaderHours = (50 * loaderTime) / 60.0;
                double truckHours = (8 * truckTime) / 60.0;
                m_cycleTimeExampleLabel->setText(
                    QString("Example: 50 buckets × %1 min = %2 hrs | 8 dumps × %3 min = %4 hrs")
                        .arg(loaderTime, 0, 'f', 1)
                        .arg(loaderHours, 0, 'f', 2)
                        .arg(truckTime, 0, 'f', 1)
                        .arg(truckHours, 0, 'f', 2)
                    );
            });

    mainLayout->addWidget(cycleGroup);

    // === Fuel Price ===
    QGroupBox *fuelGroup = new QGroupBox("Fuel Pricing");
    QVBoxLayout *fuelLayout = new QVBoxLayout(fuelGroup);

    QLabel *fuelDesc = new QLabel(
        "Default price per liter used when generating fuel log entries."
        );
    fuelDesc->setStyleSheet("color: gray;");
    fuelLayout->addWidget(fuelDesc);

    QFormLayout *fuelFormLayout = new QFormLayout();

    m_fuelPriceSpinBox = new QDoubleSpinBox();
    m_fuelPriceSpinBox->setRange(0.01, 10.00);
    m_fuelPriceSpinBox->setDecimals(2);
    m_fuelPriceSpinBox->setPrefix("$");
    m_fuelPriceSpinBox->setSuffix(" / L");
    m_fuelPriceSpinBox->setValue(0.32);
    fuelFormLayout->addRow("Fuel Price:", m_fuelPriceSpinBox);

    fuelLayout->addLayout(fuelFormLayout);

    mainLayout->addWidget(fuelGroup);

    // === Session Defaults ===
    QGroupBox *defaultsGroup = new QGroupBox("Session Defaults");
    QFormLayout *defaultsLayout = new QFormLayout(defaultsGroup);

    m_defaultMapEdit = new QLineEdit();
    m_defaultMapEdit->setPlaceholderText("e.g., FOREST_QUARRY");
    defaultsLayout->addRow("Default Map:", m_defaultMapEdit);

    m_autoEndSessionCheckbox = new QCheckBox("Prompt to end session when closing application");
    m_autoEndSessionCheckbox->setChecked(true);
    defaultsLayout->addRow("", m_autoEndSessionCheckbox);

    m_autoGenerateFuelLogCheckbox = new QCheckBox("Auto-generate fuel log entries when session ends");
    m_autoGenerateFuelLogCheckbox->setChecked(false);
    defaultsLayout->addRow("", m_autoGenerateFuelLogCheckbox);

    mainLayout->addWidget(defaultsGroup);

    // === Buttons ===
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_resetButton = new QPushButton("Reset to Defaults");
    buttonLayout->addWidget(m_resetButton);

    m_saveButton = new QPushButton("Save Settings");
    buttonLayout->addWidget(m_saveButton);

    mainLayout->addLayout(buttonLayout);
    mainLayout->addStretch();

    // === Connections ===
    connect(m_unitSystemCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &OperationsSettingsTab::onUnitSystemChanged);
    connect(m_saveButton, &QPushButton::clicked,
            this, &OperationsSettingsTab::onSaveSettings);
    connect(m_resetButton, &QPushButton::clicked,
            this, &OperationsSettingsTab::onResetDefaults);
}

void OperationsSettingsTab::loadSettings()
{
    // Unit System
    Frontier::UnitSystem currentSystem = m_manager->unitSystem();
    int index = m_unitSystemCombo->findData(static_cast<int>(currentSystem));
    if (index >= 0) {
        m_unitSystemCombo->setCurrentIndex(index);
    }

    // Cycle Times
    m_loaderCycleTimeSpinBox->setValue(m_manager->loaderCycleTimeMinutes());
    m_truckCycleTimeSpinBox->setValue(m_manager->truckCycleTimeMinutes());

    // Trigger example update
    emit m_loaderCycleTimeSpinBox->valueChanged(m_loaderCycleTimeSpinBox->value());

    // Fuel Price
    m_fuelPriceSpinBox->setValue(m_manager->fuelPricePerLiter());

    // Other settings
    QSettings settings("FrontierMining", "Tracker");
    settings.beginGroup("Operations");

    m_defaultMapEdit->setText(settings.value("defaultMap", "").toString());
    m_autoEndSessionCheckbox->setChecked(settings.value("autoEndSession", true).toBool());
    m_autoGenerateFuelLogCheckbox->setChecked(settings.value("autoGenerateFuelLog", false).toBool());

    settings.endGroup();

    // Update preview
    onUnitSystemChanged(m_unitSystemCombo->currentIndex());
}

void OperationsSettingsTab::onUnitSystemChanged(int index)
{
    Q_UNUSED(index);

    int systemVal = m_unitSystemCombo->currentData().toInt();
    Frontier::UnitSystem system = static_cast<Frontier::UnitSystem>(systemVal);

    using UC = Frontier::UnitConverter;

    // Update preview label with example values
    QString preview = QString(
                          "Example: A truck with 15 m³ capacity will display as %1\n"
                          "Example: Fuel consumption of 25 L/hr will display as %2"
                          ).arg(UC::formatVolume(15.0, system))
                          .arg(UC::formatFuelRate(25.0, system));

    m_unitPreviewLabel->setText(preview);
}

void OperationsSettingsTab::onSaveSettings()
{
    // Save unit system
    int systemVal = m_unitSystemCombo->currentData().toInt();
    Frontier::UnitSystem system = static_cast<Frontier::UnitSystem>(systemVal);
    m_manager->setUnitSystem(system);

    // Save cycle times
    m_manager->setLoaderCycleTimeMinutes(m_loaderCycleTimeSpinBox->value());
    m_manager->setTruckCycleTimeMinutes(m_truckCycleTimeSpinBox->value());

    // Save fuel price
    m_manager->setFuelPricePerLiter(m_fuelPriceSpinBox->value());

    // Save other settings
    QSettings settings("FrontierMining", "Tracker");
    settings.beginGroup("Operations");

    settings.setValue("defaultMap", m_defaultMapEdit->text());
    settings.setValue("autoEndSession", m_autoEndSessionCheckbox->isChecked());
    settings.setValue("autoGenerateFuelLog", m_autoGenerateFuelLogCheckbox->isChecked());

    settings.endGroup();

    QMessageBox::information(this, "Settings Saved",
                             "Operations settings have been saved.");
}

void OperationsSettingsTab::onResetDefaults()
{
    m_unitSystemCombo->setCurrentIndex(0);  // Imperial
    m_loaderCycleTimeSpinBox->setValue(1.5);
    m_truckCycleTimeSpinBox->setValue(6.0);
    m_fuelPriceSpinBox->setValue(0.32);
    m_defaultMapEdit->clear();
    m_autoEndSessionCheckbox->setChecked(true);
    m_autoGenerateFuelLogCheckbox->setChecked(false);

    onSaveSettings();
}
