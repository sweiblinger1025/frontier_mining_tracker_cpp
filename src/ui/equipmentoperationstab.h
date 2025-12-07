/**
 * @file equipmentoperationstab.h
 * @brief Equipment Operations subtab
 */

#ifndef EQUIPMENTOPERATIONSTAB_H
#define EQUIPMENTOPERATIONSTAB_H

#include <QWidget>
#include <QComboBox>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QTextEdit>
#include <QPushButton>

#include "core/operationsmanager.h"

class EquipmentOperationsTab : public QWidget
{
    Q_OBJECT

public:
    explicit EquipmentOperationsTab(Frontier::OperationsManager *manager,
                                    QWidget *parent = nullptr);
    ~EquipmentOperationsTab();

private slots:
    void onEquipmentChanged(int index);
    void onUnitSystemChanged(Frontier::UnitSystem system);
    void onSaveUsage();

private:
    void setupUi();
    void loadEquipment();
    void updateEquipmentInfo();

    Frontier::OperationsManager *m_manager;

    QComboBox *m_equipmentCombo;
    QLabel *m_categoryLabel;
    QLabel *m_bucketCapacityLabel;
    QLabel *m_truckCapacityLabel;
    QLabel *m_fuelUseLabel;
    QLabel *m_tankCapacityLabel;
    QDoubleSpinBox *m_hoursSpinBox;
    QTextEdit *m_notesEdit;
    QPushButton *m_saveButton;
};

#endif // EQUIPMENTOPERATIONSTAB_H
