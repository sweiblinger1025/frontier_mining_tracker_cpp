/**
 * @file auditorwidger.h
 * @brief Auditor tab - Save file parsing and validation
 */

#ifndef AUDITORWIDGET_H
#define AUDITORWIDGET_H

#include <QWidget>
#include <QTabWidget>
#include <QTableView>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QStandardItemModel>
#include <QGroupBox>
#include <QCheckBox>
#include <QSettings>
#include <QSpinBox>

#include "core/database.h"

// Forward declarration
class SaveParser;

class AuditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AuditorWidget(Frontier::Database *database, QWidget *parent = nullptr);
    ~AuditorWidget();

private slots:
    void onBrowseSaveFile();
    void onParseSaveFile();
    void onRunValidation();
    void onBrowseDefaultPath();
    void onSettingsChanged();

private:
    void setupUi();
    void loadSettings();
    void saveSettings();

    // Subtab creation
    QWidget* createSaveParserTab();
    QWidget* createValidationTab();
    QWidget* createSettingsTab();

    void updateSaveFileInfo();
    void displayTransactions();

    // Database reference
    Frontier::Database *m_database;

    // Subtabs
    QTabWidget *m_subTabs;

    // Save Parser tab widgets
    QLineEdit *m_saveFilePathEdit;
    QPushButton *m_browseButton;
    QPushButton *m_parseButton;

    // Save file info display
    QLabel *m_fileNameLabel;
    QLabel *m_fileSizeLabel;
    QLabel *m_currentMoneyLabel;
    QLabel *m_mapNameLabel;
    QLabel *m_transactionCountLabel;

    // Transactions Table
    QTableView *m_transactionsTable;
    QStandardItemModel *m_transactionsModel;

    // Raw data view
    QTextEdit *m_rawDataView;

    // Validation tab widgets
    QTableView *m_validationTable;
    QStandardItemModel *m_validationModel;
    QLabel *m_validationSummaryLabel;

    // Current save file path
    QString m_currentSaveFilePath;

    // Parsed save file data (for validation)
    struct ParsedSaveData {
        bool valid = false;
        int money = 0;
        QString map;
        int transactionCount = 0;
        double totalSales = 0;
        double totalPurchases = 0;
    };

    ParsedSaveData m_parsedData;

    // Settings tab widgets
    QLineEdit *m_defaultSavePathEdit;
    QCheckBox *m_autoParseCheckbox;
    QCheckBox *m_showRawDataCheckbox;
    QSpinBox *m_maxTransactionsSpin;

};

#endif // AUDITORWIDGET_H
