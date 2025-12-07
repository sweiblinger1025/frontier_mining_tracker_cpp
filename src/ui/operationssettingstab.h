/**
 * @file operationssettingstab.h
 * @brief Operations Settings subtab
 */

#ifndef OPERATIONSSETTINGSTAB_H
#define OPERATIONSSETTINGSTAB_H

#include <QWidget>
#include <QComboBox>
#include <QLabel>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>

#include "core/operationsmanager.h"

class OperationsSettingsTab : public QWidget
{
    Q_OBJECT

public:
    explicit OperationsSettingsTab(Frontier::OperationsManager *manager,
                                   QWidget *parent = nullptr);
    ~OperationsSettingsTab();

private slots:
    void onUnitSystemChanged(int index);
    void onSaveSettings();
    void onResetDefaults();

private:
    void setupUi();
    void loadSettings();

    Frontier::OperationsManager *m_manager;

    // Unit System
    QComboBox *m_unitSystemCombo;
    QLabel *m_unitPreviewLabel;

    // Default Values
    QLineEdit *m_defaultMapEdit;
    QCheckBox *m_autoEndSessionCheckbox;
    QCheckBox *m_autoGenerateFuelLogCheckbox;

    // Buttons
    QPushButton *m_saveButton;
    QPushButton *m_resetButton;
};

#endif // OPERATIONSSETTINGSTAB_H
