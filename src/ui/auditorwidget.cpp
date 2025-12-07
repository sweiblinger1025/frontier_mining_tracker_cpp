/**
 * @file auditorwidget.cpp
 * @brief Implementation of Auditor tab
 */

#include "auditorwidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QSplitter>
#include <QFile>
#include <QDataStream>
#include <QRegularExpression>

// Scale factor for monetary values in save file
const int MONEY_SCALE = 256;

AuditorWidget::AuditorWidget(Frontier::Database *database, QWidget *parent)
    : QWidget{parent}
    , m_database(database)
    , m_transactionsModel(nullptr)
    , m_validationModel(nullptr)

{
    setupUi();
}

AuditorWidget::~AuditorWidget()
{
}

void AuditorWidget::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    //Create subtabs
    m_subTabs = new QTabWidget();

    // Add Tabs
    m_subTabs->addTab(createSaveParserTab(), "Save Parser");
    m_subTabs->addTab(createValidationTab(), "Validation");
    m_subTabs->addTab(createSettingsTab(), "Settings");

    mainLayout->addWidget(m_subTabs);
}

QWidget* AuditorWidget::createSaveParserTab()
{
    QWidget *parserTab = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(parserTab);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // === File Selection ===
    QGroupBox *fileGroup = new QGroupBox("Save File");
    QHBoxLayout *fileLayout = new QHBoxLayout(fileGroup);

    m_saveFilePathEdit = new QLineEdit();
    m_saveFilePathEdit->setPlaceholderText("Select a save file...");
    m_saveFilePathEdit->setReadOnly(true);

    m_browseButton = new QPushButton("Browse...");
    m_parseButton = new QPushButton("Parse");
    m_parseButton->setEnabled(false);

    fileLayout->addWidget(m_saveFilePathEdit, 1);
    fileLayout->addWidget(m_browseButton);
    fileLayout->addWidget(m_parseButton);

    mainLayout->addWidget(fileGroup);

    // === Save File Info ===
    QGroupBox *infoGroup = new QGroupBox("Save File Information");
    QFormLayout *infoLayout = new QFormLayout(infoGroup);

    m_fileNameLabel = new QLabel("-");
    m_fileSizeLabel = new QLabel("-");
    m_currentMoneyLabel = new QLabel("-");
    m_currentMoneyLabel->setStyleSheet("font-weight: bold; color: green;");
    m_mapNameLabel = new QLabel("-");
    m_transactionCountLabel = new QLabel("-");

    infoLayout->addRow("File Name:", m_fileNameLabel);
    infoLayout->addRow("File Size:", m_fileSizeLabel);
    infoLayout->addRow("Current Money:", m_currentMoneyLabel);
    infoLayout->addRow("Map:", m_mapNameLabel);
    infoLayout->addRow("Transactions:", m_transactionCountLabel);

    mainLayout->addWidget(infoGroup);

    // === Splitter for transactions and raw data ===
    QSplitter *splitter = new QSplitter(Qt::Vertical);

    // Transactions table
    QGroupBox *transGroup = new QGroupBox("Parsed Transactions");
    QVBoxLayout *transLayout = new QVBoxLayout(transGroup);

    m_transactionsTable = new QTableView();
    m_transactionsTable->setAlternatingRowColors(true);
    m_transactionsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_transactionsTable->setSortingEnabled(true);
    m_transactionsTable->horizontalHeader()->setStretchLastSection(true);
    m_transactionsTable->verticalHeader()->setVisible(false);

    m_transactionsModel = new QStandardItemModel(this);
    m_transactionsModel->setHorizontalHeaderLabels({
        "Item Code", "Category", "Amount", "Type"
    });
    m_transactionsTable->setModel(m_transactionsModel);

    transLayout->addWidget(m_transactionsTable);
    splitter->addWidget(transGroup);

    // Raw data view
    QGroupBox *rawGroup = new QGroupBox("Raw Data (Debug)");
    QVBoxLayout *rawLayout = new QVBoxLayout(rawGroup);

    m_rawDataView = new QTextEdit();
    m_rawDataView->setReadOnly(true);
    m_rawDataView->setFont(QFont("Consolas", 9));
    m_rawDataView->setMaximumHeight(150);

    rawLayout->addWidget(m_rawDataView);
    splitter->addWidget(rawGroup);

    mainLayout->addWidget(splitter, 1);

    // === Connect Signals ===
    connect(m_browseButton, &QPushButton::clicked,
            this, &AuditorWidget::onBrowseSaveFile);
    connect(m_parseButton, &QPushButton::clicked,
            this, &AuditorWidget::onParseSaveFile);

    return parserTab;
}

QWidget* AuditorWidget::createValidationTab()
{
    QWidget *validationTab = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(validationTab);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // === Validation Controls ===
    QHBoxLayout *controlLayout = new QHBoxLayout();

    QPushButton *validateButton = new QPushButton("Run Validation");
    connect(validateButton, &QPushButton::clicked,
            this, &AuditorWidget::onRunValidation);

    m_validationSummaryLabel = new QLabel("Load a save file and run validation to compare with ledger.");

    controlLayout->addWidget(validateButton);
    controlLayout->addWidget(m_validationSummaryLabel, 1);

    mainLayout->addLayout(controlLayout);

    // === Validation Results ===
    QGroupBox *resultsGroup = new QGroupBox("Validation Results");
    QVBoxLayout *resultsLayout = new QVBoxLayout(resultsGroup);

    m_validationTable = new QTableView();
    m_validationTable->setAlternatingRowColors(true);
    m_validationTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_validationTable->horizontalHeader()->setStretchLastSection(true);
    m_validationTable->verticalHeader()->setVisible(false);

    m_validationModel = new QStandardItemModel(this);
    m_validationModel->setHorizontalHeaderLabels({
        "Status", "Item", "Save Amount", "Ledger Amount", "Difference"
    });
    m_validationTable->setModel(m_validationModel);

    resultsLayout->addWidget(m_validationTable);

    mainLayout->addWidget(resultsGroup, 1);

    return validationTab;
}

QWidget* AuditorWidget::createSettingsTab()
{
    QWidget *settingsTab = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(settingsTab);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    QGroupBox *pathsGroup = new QGroupBox("Default Paths");
    QFormLayout *pathsLayout = new QFormLayout(pathsGroup);

    QLineEdit *defaultSavePathEdit = new QLineEdit();
    defaultSavePathEdit->setPlaceholderText("Default save file location...");
    pathsLayout->addRow("Save Files:", defaultSavePathEdit);

    mainLayout->addWidget(pathsGroup);

    QGroupBox *optionsGroup = new QGroupBox("Parser Options");
    QFormLayout *optionsLayout = new QFormLayout(optionsGroup);

    QLabel *scaleLabel = new QLabel(QString("Money Scale Factor: %1").arg(MONEY_SCALE));
    optionsLayout->addRow("", scaleLabel);

    mainLayout->addWidget(optionsGroup);

    mainLayout->addStretch();

    return settingsTab;
}

void AuditorWidget::onBrowseSaveFile()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Select Save File",
        QString(),
        "Save Files (*.sav);;All Files (*.*)"
        );

    if (!fileName.isEmpty()) {
        m_saveFilePathEdit->setText(fileName);
        m_currentSaveFilePath = fileName;
        m_parseButton->setEnabled(true);

        // Show basic file info
        QFileInfo fileInfo(fileName);
        m_fileNameLabel->setText(fileInfo.fileName());
        m_fileSizeLabel->setText(QString("%L1 bytes").arg(fileInfo.size()));
    }
}

void AuditorWidget::onParseSaveFile()
{
    if (m_currentSaveFilePath.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select a save file first.");
        return;
    }

    QFile file(m_currentSaveFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Error",
                              QString("Could not open file: %1").arg(file.errorString()));
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    // Verify GVAS magic
    if (!data.startsWith("GVAS")) {
        QMessageBox::critical(this, "Error", "Not a valid GVAS save file.");
        return;
    }

    m_rawDataView->clear();
    m_rawDataView->append(QString("File size: %1 bytes").arg(data.size()));
    m_rawDataView->append(QString("Magic: %1").arg(QString(data.left(4))));

    // === Parse NewMoney ===
    int moneyPos = data.indexOf("NewMoney");
    if (moneyPos != -1) {
        qint32 money;
        memcpy(&money, data.constData() + moneyPos + 34, sizeof(qint32));
        m_currentMoneyLabel->setText(QString("$%L1").arg(money));
        m_rawDataView->append(QString("Personal Money: $%L1").arg(money));
    } else {
        m_currentMoneyLabel->setText("Not found");
    }

    // === Parse Map Name ===
    QStringList mapPatterns = {
        "FOREST_QUARRY", "DESERT_MINE", "ARCTIC_MINE", "VOLCANO_MINE",
        "GRASS_FLAT", "FOREST_FLAT", "FOREST_HILLS", "MINE_TOWN",
        "QUARRY", "SCANDINAVIA", "COAL_PLANT", "IRON_MOUNTAIN", "ARCTIC_WINTER"
    };

    m_mapNameLabel->setText("Unknown");
    for (const QString &pattern : mapPatterns) {
        if (data.contains(pattern.toUtf8())) {
            m_mapNameLabel->setText(pattern);
            m_rawDataView->append(QString("Map: %1").arg(pattern));
            break;
        }
    }

    // === Parse Transactions ===
    m_transactionsModel->removeRows(0, m_transactionsModel->rowCount());

    int transStart = data.indexOf("TransactionsHistory");
    if (transStart == -1) {
        m_transactionCountLabel->setText("Not found");
        m_rawDataView->append("TransactionsHistory not found");
        return;
    }

    m_rawDataView->append(QString("TransactionsHistory at: %1").arg(transStart));

    // Look for item codes (6 digits starting with 1-4) in the transaction area
    // Item codes appear as: ...NameProperty\0....\x07\x00\x00\x00XXXXXX\x00...

    int transactionCount = 0;
    double totalSales = 0;
    double totalPurchases = 0;

    // Search for 6-digit item codes
    QRegularExpression codePattern("[1-4]\\d{5}");
    int searchStart = transStart + 100;
    int searchEnd = qMin(transStart + 10000, data.size());

    // Convert search area to string for regex (just the relevant portion)
    QString searchArea = QString::fromLatin1(data.mid(searchStart, searchEnd - searchStart));

    QRegularExpressionMatchIterator matches = codePattern.globalMatch(searchArea);

    QSet<int> processedPositions;  // Avoid duplicates

    while (matches.hasNext()) {
        QRegularExpressionMatch match = matches.next();
        QString itemCode = match.captured(0);
        int localPos = match.capturedStart();
        int globalPos = searchStart + localPos;

        // Skip if we've processed nearby position (within 50 bytes)
        bool skip = false;
        for (int p : processedPositions) {
            if (qAbs(p - globalPos) < 50) {
                skip = true;
                break;
            }
        }
        if (skip) continue;

        // Verify this is in a transaction context (has Category and Amount nearby)
        QByteArray context = data.mid(globalPos, 300);

        if (!context.contains("Category") || !context.contains("Amount")) {
            continue;
        }

        processedPositions.insert(globalPos);

        // Extract category
        QString category = "Unknown";
        int catPos = context.indexOf("Category");
        if (catPos != -1) {
            // Find NameProperty after Category
            int catNpPos = context.indexOf("NameProperty", catPos);
            if (catNpPos != -1 && catNpPos + 30 < context.size()) {
                // String length is at NameProperty + 22
                qint32 catStrLen;
                memcpy(&catStrLen, context.constData() + catNpPos + 22, sizeof(qint32));

                // String starts at NameProperty + 26
                if (catStrLen > 0 && catStrLen < 30 && catNpPos + 26 + catStrLen <= context.size()) {
                    category = QString::fromLatin1(context.mid(catNpPos + 26, catStrLen - 1));
                }
            }
        }

        // Extract amount
        qint32 amount = 0;
        int amtPos = context.indexOf("Amount");
        if (amtPos != -1 && amtPos + 36 < context.size()) {
            memcpy(&amount, context.constData() + amtPos + 32, sizeof(qint32));
        }

        // Determine type
        QString transType = (amount >= 0) ? "Sale" : "Purchase";

        // Add to table
        QList<QStandardItem*> row;
        row << new QStandardItem(itemCode);
        row << new QStandardItem(category);
        row << new QStandardItem(QString("$%L1").arg(amount));
        row << new QStandardItem(transType);

        // Color code
        if (amount < 0) {
            row[2]->setForeground(Qt::red);
        } else {
            row[2]->setForeground(QColor(0, 128, 0));
        }

        m_transactionsModel->appendRow(row);

        // Track totals
        if (amount > 0) {
            totalSales += amount;
        } else {
            totalPurchases += qAbs(amount);
        }

        transactionCount++;

        if (transactionCount >= 50) break;  // Safety limit
    }

    m_transactionCountLabel->setText(QString("%1").arg(transactionCount));

    // Resize columns
    m_transactionsTable->resizeColumnsToContents();

    m_rawDataView->append(QString("\nTransactions parsed: %1").arg(transactionCount));
    m_rawDataView->append(QString("Total Sales: $%L1").arg(totalSales));
    m_rawDataView->append(QString("Total Purchases: $%L1").arg(totalPurchases));
    m_rawDataView->append("\nParsing complete.");
}

void AuditorWidget::onRunValidation()
{
    if (m_currentSaveFilePath.isEmpty()) {
        QMessageBox::warning(this, "Validation",
                             "Please load and parse a save file first.");
        return;
    }

    m_validationModel->removeRows(0, m_validationModel->rowCount());

    // Get ledger transactions
    auto ledgerTransactions = m_database->getAllTransactions();

    // For now, show a placeholder
    m_validationSummaryLabel->setText(
        QString("Ledger has %1 transactions. Full validation coming soon.")
            .arg(ledgerTransactions.size()));

    QMessageBox::information(this, "Validation",
                             "Full validation feature coming soon!\n\n"
                             "This will compare save file transactions with your ledger entries.");
}
