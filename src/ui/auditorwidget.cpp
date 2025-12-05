#include "auditorwidget.h"

#include <QLabel>
#include <QVBoxLayout>

AuditorWidget::AuditorWidget(QWidget *parent)
    : QWidget{parent}
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    QLabel *label = new QLabel("Auditor - Coming Soon", this);
    layout->addWidget(label);
}
