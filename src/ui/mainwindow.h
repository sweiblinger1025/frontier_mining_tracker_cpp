#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QLabel>

#include "core/database.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(Frontier::Database *database, QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    // Database reference
    Frontier::Database *m_database;

    // Tab widget
    QTabWidget *m_tabWidget;

    // Status bar widgets
    QLabel *m_dayLabel;
    QLabel *m_balanceLabel;
};

#endif // MAINWINDOW_H
