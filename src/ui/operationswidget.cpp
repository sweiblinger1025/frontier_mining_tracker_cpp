#include "operationswidget.h"

#include <QLabel>
#include <QVBoxLayout>

OperationsWidget::OperationsWidget(QWidget *parent)
    : QWidget{parent}
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel("Operations - Coming Soon", this);
    layout->addWidget(label);
}
