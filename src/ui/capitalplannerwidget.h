/**
 * @file capitalplannerwidget.h
 * @brief Capital Planner - Plan equipment and facility purchases
 */

#ifndef CAPITALPLANNERWIDGET_H
#define CAPITALPLANNERWIDGET_H

#include <QWidget>
#include <QTabWidget>

namespace Frontier {
class Database;
}

class BudgetOverviewTab;
class EquipmentPlannerTab;
class FacilityPlannerTab;

class CapitalPlannerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CapitalPlannerWidget(Frontier::Database *database, QWidget *parent = nullptr);

public slots:
    void refreshData();

private:
    void setupUi();

    Frontier::Database *m_database;
    QTabWidget *m_tabs;

    BudgetOverviewTab *m_overviewTab;
    EquipmentPlannerTab *m_equipmentTab;
    FacilityPlannerTab *m_facilityTab;
};

#endif // CAPITALPLANNERWIDGET_H
