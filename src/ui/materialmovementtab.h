/**
 * @file materialmovementtab.h
 * @brief Material Movement subtab for Operations - Core tracking tab
 */

#ifndef MATERIALMOVEMENTTAB_H
#define MATERIALMOVEMENTTAB_H

#include <QWidget>
#include <QTableView>
#include <QStandardItemModel>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDateTimeEdit>
#include <QGroupBox>

#include "core/operationsmanager.h"

class MaterialMovementTab : public QWidget
{
    Q_OBJECT

public:
    explicit MaterialMovementTab(Frontier::OperationsManager *manager,
                                 QWidget *parent = nullptr);
    ~MaterialMovementTab();

private slots:
    // Session controls
    void onStartSessionClicked();
    void onEndSessionClicked();
    void onSaveSessionClicked();
    void onLoadSessionClicked();

    // Equipment usage
    void onEquipmentChanged(int index);
    void onRoleChanged(int index);
    void onAddUpdateUsageClicked();
    void onDeleteUsageClicked();
    void onUsageSelectionChanged();

    // Manager signals
    void onUnitSystemChanged(Frontier::UnitSystem system);
    void onSessionStarted(int sessionId);
    void onSessionEnded(int sessionId);
    void onEquipmentUsageUpdated(int sessionId);

    // Other
    void onGenerateFuelLogClicked();

private:
    void setupUi();
    QWidget* createSessionHeaderPanel();
    QWidget* createEquipmentUsagePanel();
    QWidget* createSessionSummaryPanel();

    void loadEquipmentCombo();
    void loadSessionHistory();
    void loadSession(int sessionId);
    void loadEquipmentUsage();
    void updateEquipmentInfo();
    void updateSummary();
    void updateSessionControls();
    void clearSession();
    void clearUsageForm();

    Frontier::OperationsManager *m_manager;
    std::optional<int> m_currentSessionId;

    // Session Header widgets
    QComboBox *m_sessionHistoryCombo;
    QLabel *m_sessionIdLabel;
    QLineEdit *m_mapNameEdit;
    QDateTimeEdit *m_startTimeEdit;
    QDateTimeEdit *m_endTimeEdit;
    QTextEdit *m_sessionNotesEdit;
    QPushButton *m_startSessionButton;
    QPushButton *m_endSessionButton;
    QPushButton *m_saveSessionButton;

    // Equipment Usage widgets
    QComboBox *m_equipmentCombo;
    QLabel *m_equipCategoryLabel;
    QLabel *m_equipCapacityLabel;
    QLabel *m_equipFuelLabel;
    QComboBox *m_roleCombo;
    QDoubleSpinBox *m_hoursSpinBox;
    QSpinBox *m_bucketsSpinBox;
    QSpinBox *m_loadsSpinBox;
    QSpinBox *m_dumpsSpinBox;
    QLabel *m_bucketsLabel;
    QLabel *m_loadsLabel;
    QLabel *m_dumpsLabel;
    QPushButton *m_addUpdateUsageButton;
    QPushButton *m_deleteUsageButton;

    // Usage Table
    QTableView *m_usageTableView;
    QStandardItemModel *m_usageModel;

    // Summary widgets
    QLabel *m_totalVolumeLabel;
    QLabel *m_truckVolumeLabel;
    QLabel *m_loaderVolumeLabel;
    QLabel *m_totalFuelLabel;
    QLabel *m_totalHoursLabel;
    QPushButton *m_generateFuelLogButton;
};

#endif // MATERIALMOVEMENTTAB_H
