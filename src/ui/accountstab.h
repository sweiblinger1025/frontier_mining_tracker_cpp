/**
 * @file accountstab.h
 * @brief Accounts - Company vs Personal balance tracking
 */

#ifndef ACCOUNTSTAB_H
#define ACCOUNTSTAB_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QTableWidget>
#include <QProgressBar>

#include "core/types.h"

namespace Frontier {
class Database;
}

class AccountsTab : public QWidget
{
    Q_OBJECT

public:
    explicit AccountsTab(Frontier::Database *database, QWidget *parent = nullptr);

public slots:
    void refreshData();

signals:
    void transferCompleted();

private slots:
    void onTransfer();

private:
    void setupUi();
    QWidget* createBalanceCard(const QString &title, QLabel **valueLabel, QLabel **percentLabel, const QString &color);
    void updateBalances();

    Frontier::Database *m_database;

    // Balance displays
    QLabel *m_companyBalanceLabel;
    QLabel *m_companyPercentLabel;
    QLabel *m_personalBalanceLabel;
    QLabel *m_personalPercentLabel;
    QLabel *m_totalBalanceLabel;

    // Transfer controls
    QDoubleSpinBox *m_transferAmountSpin;
    QComboBox *m_fromAccountCombo;
    QComboBox *m_toAccountCombo;
    QPushButton *m_transferBtn;

    // Split ratio display
    QProgressBar *m_splitBar;
    QLabel *m_splitLabel;

    // Recent transactions
    QTableWidget *m_recentTable;
};

#endif // ACCOUNTSTAB_H
