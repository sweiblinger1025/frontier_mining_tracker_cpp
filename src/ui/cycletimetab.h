/**
 * @file cycletimetab.h
 * @brief Cycle Time Analyzer - track and optimize haul cycles
 */

#ifndef CYCLETIMETAB_H
#define CYCLETIMETAB_H

#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QLineEdit>
#include <QTimer>

#include "core/types.h"

namespace Frontier {
class Database;
}

class CycleTimeTab : public QWidget
{
    Q_OBJECT

public:
    explicit CycleTimeTab(Frontier::Database *database, QWidget *parent = nullptr);

public slots:
    void refreshData();

private slots:
    // Profile management
    void onNewProfile();
    void onEditProfile();
    void onDeleteProfile();
    void onProfileSelected();

    // Timer controls
    void onStartStop();
    void onNextPhase();
    void onResetTimer();
    void onTimerTick();

    // Record management
    void onSaveRecord();
    void onDeleteRecord();
    void onRecordSelected();

private:
    void setupUi();
    QWidget* createProfilePanel();
    QWidget* createTimerPanel();
    QWidget* createStatsPanel();
    QWidget* createHistoryPanel();

    void loadProfiles();
    void loadRecordsForProfile(int profileId);
    void updateStats();
    void updateTimerDisplay();
    void showProfileDialog(bool isEdit);

    QString formatSeconds(int seconds) const;

    Frontier::Database *m_database;

    // Current state
    QVector<Frontier::CycleProfile> m_profiles;
    QVector<Frontier::CycleRecord> m_records;
    int m_currentPhase = 0;  // 0=stopped, 1=load, 2=haul, 3=dump, 4=return
    int m_phaseSeconds[4] = {0, 0, 0, 0};  // load, haul, dump, return
    QTimer *m_timer;

    // Profile panel
    QComboBox *m_profileCombo;
    QPushButton *m_newProfileBtn;
    QPushButton *m_editProfileBtn;
    QPushButton *m_deleteProfileBtn;
    QLabel *m_profileDetailsLabel;

    // Timer panel
    QLabel *m_timerDisplay;
    QLabel *m_phaseLabel;
    QPushButton *m_startStopBtn;
    QPushButton *m_nextPhaseBtn;
    QPushButton *m_resetBtn;

    // Phase time displays
    QLabel *m_loadTimeLabel;
    QLabel *m_haulTimeLabel;
    QLabel *m_dumpTimeLabel;
    QLabel *m_returnTimeLabel;
    QLabel *m_totalTimeLabel;

    // Manual entry spinboxes
    QSpinBox *m_loadMinSpin;
    QSpinBox *m_loadSecSpin;
    QSpinBox *m_haulMinSpin;
    QSpinBox *m_haulSecSpin;
    QSpinBox *m_dumpMinSpin;
    QSpinBox *m_dumpSecSpin;
    QSpinBox *m_returnMinSpin;
    QSpinBox *m_returnSecSpin;
    QLineEdit *m_recordNotesEdit;
    QPushButton *m_saveRecordBtn;

    // Stats labels
    QLabel *m_recordCountLabel;
    QLabel *m_avgTimeLabel;
    QLabel *m_bestTimeLabel;
    QLabel *m_worstTimeLabel;

    // History table
    QTableWidget *m_historyTable;
    QPushButton *m_deleteRecordBtn;
};

#endif // CYCLETIMETAB_H
