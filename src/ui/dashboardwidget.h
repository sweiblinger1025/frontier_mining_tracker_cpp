/**
 * @file dashboardwidget.h
 * @brief Main dashboard showing financial overview, quick actions, and recent activity
 */

#ifndef DASHBOARDWIDGET_H
#define DASHBOARDWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QDateEdit>
#include <QTextEdit>
#include <QProgressBar>

namespace Frontier {
class Database;
}

class DashboardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DashboardWidget(Frontier::Database *database, QWidget *parent = nullptr);

signals:
    void navigateToLedger();
    void navigateToCapitalPlanner();
    void navigateToDataHub();
    void addTransactionRequested();

public slots:
    void refreshData();

private slots:
    void onPrevDay();
    void onNextDay();
    void onToday();
    void onDateChanged(const QDate &date);
    void onSaveNotes();
    void onViewAllTransactions();

private:
    void setupUi();
    QWidget* createStatusBanner();
    QWidget* createFinancialSummary();
    QWidget* createCapitalPlanSummary();
    QWidget* createQuickActions();
    QWidget* createDailyJournal();
    QWidget* createRecentActivity();

    void updateFinancialSummary();
    void updateCapitalPlanSummary();
    void updateDailyJournal();
    void updateRecentActivity();
    void loadNotesForDate(const QDate &date);
    void saveNotesForDate(const QDate &date);

    Frontier::Database *m_database;

    // Status Banner
    QLabel *m_statusLabel;
    QLabel *m_dayLabel;
    QPushButton *m_refreshBtn;

    // Financial Summary
    QLabel *m_netWorthLabel;
    QLabel *m_companyLabel;
    QLabel *m_personalLabel;
    QLabel *m_transactionCountLabel;

    // Capital Plan Summary
    QLabel *m_equipmentTotalLabel;
    QLabel *m_facilityTotalLabel;
    QLabel *m_powerBalanceLabel;
    QLabel *m_affordabilityLabel;
    QProgressBar *m_affordabilityBar;

    // Quick Actions
    QPushButton *m_addTransactionBtn;
    QPushButton *m_capitalPlannerBtn;
    QPushButton *m_dataHubBtn;
    QPushButton *m_ledgerBtn;

    // Daily Journal
    QDateEdit *m_journalDateEdit;
    QTableWidget *m_dayActivitiesTable;
    QTextEdit *m_notesEdit;
    QPushButton *m_saveNotesBtn;
    QLabel *m_dayIncomeLabel;
    QLabel *m_dayExpensesLabel;
    QLabel *m_dayNetLabel;

    // Recent Activity
    QTableWidget *m_recentActivityTable;
    QPushButton *m_viewAllBtn;

    // Current journal date
    QDate m_currentJournalDate;
};

#endif // DASHBOARDWIDGET_H
