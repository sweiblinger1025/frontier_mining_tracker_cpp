#include "datahubwidget.h"

#include <QLabel>
#include <QVBoxLayout>

DataHubWidget::DataHubWidget(QWidget *parent)
    : QWidget{parent}
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel("DataHub - Coming Soon", this);
    layout->addWidget(label);
}
