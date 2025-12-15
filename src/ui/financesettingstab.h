/**
 * @file financesettingstab.h
 * @brief Finance Settings - configuration for finance behavior
 */

#ifndef FINANCESETTINGSTAB_H
#define FINANCESETTINGSTAB_H

#include <QWidget>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>

namespace Frontier {
class Database;
}

class FinanceSettingsTab : public QWidget
{
    Q_OBJECT

public:
    explicit FinanceSettingsTab(Frontier::Database *database, QWidget *parent = nullptr);

public slots:
    void refreshData();

private slots:
    void onSplitRatioChanged();
    void onAddCategory();
    void onDeleteCategory();

private:
    void setupUi();
    void loadSettings();

    Frontier::Database *m_database;

    // Auto-split settings
    QSpinBox *m_companySplitSpin;
    QSpinBox *m_personalSplitSpin;
    QLabel *m_splitExampleLabel;

    // Category management
    QListWidget *m_categoryList;
    QPushButton *m_addCategoryBtn;
    QPushButton *m_deleteCategoryBtn;

    // Options
    QCheckBox *m_trackPersonalCheck;
    QCheckBox *m_showCentsCheck;
};

#endif // FINANCESETTINGSTAB_H
