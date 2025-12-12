/**
 * @file shiftlogtab.h
 * @brief Shift Log - gameplay session diary
 */

#ifndef SHIFTLOGTAB_H
#define SHIFTLOGTAB_H

#include <QWidget>
#include <QDateTimeEdit>
#include <QComboBox>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>

#include "core/types.h"

namespace Frontier {
class Database;
}

class ShiftLogTab : public QWidget
{
    Q_OBJECT

public:
    explicit ShiftLogTab(Frontier::Database *database, QWidget *parent = nullptr);

public slots:
    void refreshData();

private slots:
    void onStartShift();
    void onEndShift();
    void onSaveShift();
    void onNewShift();
    void onEditShift();
    void onDeleteShift();
    void onSelectionChanged();

private:
    void setupUi();
    QWidget* createEntryPanel();
    QWidget* createSummaryPanel();

    void loadShifts();
    void updateSummary();
    void clearForm();
    void populateForm(const Frontier::Shift &shift);
    Frontier::Shift getFormData() const;

    Frontier::Database *m_database;

    // Current state
    std::optional<int> m_editingShiftId;
    QVector<Frontier::Shift> m_shifts;

    // Entry form
    QDateTimeEdit *m_startTimeEdit;
    QDateTimeEdit *m_endTimeEdit;
    QComboBox *m_weatherCombo;
    QTextEdit *m_activitiesEdit;
    QTextEdit *m_notesEdit;

    // Action buttons
    QPushButton *m_startShiftBtn;
    QPushButton *m_endShiftBtn;
    QPushButton *m_saveBtn;
    QPushButton *m_newBtn;

    // Summary labels
    QLabel *m_totalShiftsLabel;
    QLabel *m_totalTimeLabel;
    QLabel *m_avgDurationLabel;

    // History table
    QTableWidget *m_historyTable;
    QPushButton *m_editBtn;
    QPushButton *m_deleteBtn;
};

#endif // SHIFTLOGTAB_H
