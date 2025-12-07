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

    // Example box showing conversions
    QGroupBox *previewBox = new QGroupBox("Reference Conversions");
    QFormLayout *previewLayout = new QFormLayout(previewBox);
    previewLayout->addRow("Volume:", new QLabel("1 m³ = 1.308 yd³"));
    previewLayout->addRow("Fuel:", new QLabel("1 L = 0.264 gal"));
    previewLayout->addRow("Distance:", new QLabel("1 km = 0.621 mi"));
    unitLayout->addWidget(previewBox);

    mainLayout->addWidget(unitGroup);

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

    // === Info ===
    QGroupBox *infoGroup = new QGroupBox("Information");
    QFormLayout *infoLayout = new QFormLayout(infoGroup);

    infoLayout->addRow("Data Storage:", new QLabel("All values stored internally in metric (m³, L)"));
    infoLayout->addRow("Conversion:", new QLabel("Display converted based on your preference"));

    mainLayout->addWidget(infoGroup);

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

    // Save other settings
    QSettings settings("FrontierMining", "Tracker");
    settings.beginGroup("Operations");

    settings.setValue("defaultMap", m_defaultMapEdit->text());
    settings.setValue("autoEndSession", m_autoEndSessionCheckbox->isChecked());
    settings.setValue("autoGenerateFuelLog", m_autoGenerateFuelLogCheckbox->isChecked());

    settings.endGroup();

    QMessageBox::information(this, "Settings Saved",
                             "Operations settings have been saved.\n\n"
                             "Unit changes will apply to all Operations tabs.");
}

void OperationsSettingsTab::onResetDefaults()
{
    m_unitSystemCombo->setCurrentIndex(0);  // Imperial
    m_defaultMapEdit->clear();
    m_autoEndSessionCheckbox->setChecked(true);
    m_autoGenerateFuelLogCheckbox->setChecked(false);

    onSaveSettings();
}
