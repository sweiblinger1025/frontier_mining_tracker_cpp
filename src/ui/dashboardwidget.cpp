#include "dashboardwidget.h"

#include <QLabel>
#include <QVBoxLayout>

DashboardWidget::DashboardWidget(QWidget *parent)
    : QWidget{parent}
{
    // Temporary: just a label to prove it works
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel("Dashboard - Coming Soon", this);
    layout->addWidget(label);
}
