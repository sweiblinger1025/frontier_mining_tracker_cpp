/**
 * @file operationswidget.h
 * @brief Operations tab with subtabs for equipment, fuel, material movement
 */

#ifndef OPERATIONSWIDGET_H
#define OPERATIONSWIDGET_H

#include <QWidget>
#include <QTabWidget>

#include "core/operationsmanager.h"

class OperationsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OperationsWidget(Frontier::OperationsManager *manager, QWidget *parent = nullptr);
    ~OperationsWidget();

private:
    void setupUi();

    Frontier::OperationsManager *m_manager;
    QTabWidget *m_subTabs;
};

#endif // OPERATIONSWIDGET_H
