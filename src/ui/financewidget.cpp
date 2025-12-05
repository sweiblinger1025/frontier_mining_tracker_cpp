#include "financewidget.h"

#include <QLabel>
#include <QVBoxLayout>

FinanceWidget::FinanceWidget(QWidget *parent)
    : QWidget{parent}
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel("Finance - Coming Soon", this);
    layout->addWidget(label);
}
