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
    m_parsedData.valid = false;  // Add this
    setupUi();
    loadSettings();
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
    QGroupBox *controlGroup = new QGroupBox("Validation");
    QHBoxLayout *controlLayout = new QHBoxLayout(controlGroup);

    QPushButton *validateButton = new QPushButton("Run Validation");
    connect(validateButton, &QPushButton::clicked,
            this, &AuditorWidget::onRunValidation);

    m_validationSummaryLabel = new QLabel("Load a save file first, then run validation to compare with ledger.");
    m_validationSummaryLabel->setWordWrap(true);

    controlLayout->addWidget(validateButton);
    controlLayout->addWidget(m_validationSummaryLabel, 1);

    mainLayout->addWidget(controlGroup);

    // === Discrepancies Table ===
    QGroupBox *resultsGroup = new QGroupBox("Discrepancies Found");
    QVBoxLayout *resultsLayout = new QVBoxLayout(resultsGroup);

    m_validationTable = new QTableView();
    m_validationTable->setAlternatingRowColors(true);
    m_validationTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_validationTable->horizontalHeader()->setStretchLastSection(true);
    m_validationTable->verticalHeader()->setVisible(false);

    m_validationModel = new QStandardItemModel(this);
    m_validationModel->setHorizontalHeaderLabels({
        "Field", "Ledger Value", "Save Value", "Difference"
    });
    m_validationTable->setModel(m_validationModel);

    resultsLayout->addWidget(m_validationTable);

    mainLayout->addWidget(resultsGroup, 1);

    // === Legend ===
    QGroupBox *legendGroup = new QGroupBox("Legend");
    QHBoxLayout *legendLayout = new QHBoxLayout(legendGroup);

    QLabel *matchLabel = new QLabel("● Match");
    matchLabel->setStyleSheet("color: green;");

    QLabel *diffLabel = new QLabel("● Discrepancy");
    diffLabel->setStyleSheet("color: red;");

    legendLayout->addWidget(matchLabel);
    legendLayout->addWidget(diffLabel);
    legendLayout->addStretch();

    mainLayout->addWidget(legendGroup);

    return validationTab;
}

QWidget* AuditorWidget::createSettingsTab()
{
    QWidget *settingsTab = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(settingsTab);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(15);

    // === Default Paths ===
    QGroupBox *pathsGroup = new QGroupBox("Default Paths");
    QVBoxLayout *pathsLayout = new QVBoxLayout(pathsGroup);

    QLabel *pathLabel = new QLabel("Default Save File Location:");
    pathsLayout->addWidget(pathLabel);

    QHBoxLayout *pathEditLayout = new QHBoxLayout();
    m_defaultSavePathEdit = new QLineEdit();
    m_defaultSavePathEdit->setPlaceholderText("e.g., C:/Users/YourName/AppData/Local/OutOfOre/Saved/SaveGames/");

    QPushButton *browseDefaultBtn = new QPushButton("Browse...");
    connect(browseDefaultBtn, &QPushButton::clicked,
            this, &AuditorWidget::onBrowseDefaultPath);

    pathEditLayout->addWidget(m_defaultSavePathEdit, 1);
    pathEditLayout->addWidget(browseDefaultBtn);
    pathsLayout->addLayout(pathEditLayout);

    QLabel *pathHint = new QLabel(
        "<i>Tip: Save files are usually in:<br>"
        "C:\\Users\\[Name]\\AppData\\Local\\OutOfOre\\Saved\\SaveGames\\[SaveName]\\</i>");
    pathHint->setStyleSheet("color: gray;");
    pathsLayout->addWidget(pathHint);

    mainLayout->addWidget(pathsGroup);

    // === Parser Options ===
    QGroupBox *optionsGroup = new QGroupBox("Parser Options");
    QFormLayout *optionsLayout = new QFormLayout(optionsGroup);

    m_autoParseCheckbox = new QCheckBox("Auto-parse when file is selected");
    optionsLayout->addRow("", m_autoParseCheckbox);

    m_showRawDataCheckbox = new QCheckBox("Show raw data debug panel");
    m_showRawDataCheckbox->setChecked(true);
    optionsLayout->addRow("", m_showRawDataCheckbox);

    m_maxTransactionsSpin = new QSpinBox();
    m_maxTransactionsSpin->setRange(10, 500);
    m_maxTransactionsSpin->setValue(50);
    m_maxTransactionsSpin->setSuffix(" transactions");
    optionsLayout->addRow("Max transactions to parse:", m_maxTransactionsSpin);

    mainLayout->addWidget(optionsGroup);

    // === Info ===
    QGroupBox *infoGroup = new QGroupBox("Parser Information");
    QFormLayout *infoLayout = new QFormLayout(infoGroup);

    infoLayout->addRow("File Format:", new QLabel("GVAS (Unreal Engine 4)"));
    infoLayout->addRow("Money Scale:", new QLabel("1:1 (no scaling)"));
    infoLayout->addRow("Supported Maps:", new QLabel("All Out of Ore maps"));

    mainLayout->addWidget(infoGroup);

    // === Buttons ===
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    QPushButton *resetBtn = new QPushButton("Reset to Defaults");
    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        m_defaultSavePathEdit->clear();
        m_autoParseCheckbox->setChecked(false);
        m_showRawDataCheckbox->setChecked(true);
        m_maxTransactionsSpin->setValue(50);
        saveSettings();
    });

    QPushButton *saveBtn = new QPushButton("Save Settings");
    connect(saveBtn, &QPushButton::clicked, this, &AuditorWidget::saveSettings);

    buttonLayout->addStretch();
    buttonLayout->addWidget(resetBtn);
    buttonLayout->addWidget(saveBtn);

    mainLayout->addLayout(buttonLayout);

    mainLayout->addStretch();

    // Connect settings changes
    connect(m_defaultSavePathEdit, &QLineEdit::textChanged,
            this, &AuditorWidget::onSettingsChanged);
    connect(m_autoParseCheckbox, &QCheckBox::toggled,
            this, &AuditorWidget::onSettingsChanged);
    connect(m_showRawDataCheckbox, &QCheckBox::toggled,
            this, &AuditorWidget::onSettingsChanged);
    connect(m_maxTransactionsSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &AuditorWidget::onSettingsChanged);

    return settingsTab;
}

void AuditorWidget::onBrowseDefaultPath()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        "Select Default Save Folder",
        m_defaultSavePathEdit->text(),
        QFileDialog::ShowDirsOnly
        );

    if (!dir.isEmpty()) {
        m_defaultSavePathEdit->setText(dir);
        saveSettings();
    }
}

void AuditorWidget::onSettingsChanged()
{
    // Update raw data panel visibility
    if (m_rawDataView) {
        m_rawDataView->parentWidget()->setVisible(m_showRawDataCheckbox->isChecked());
    }
}

void AuditorWidget::loadSettings()
{
    QSettings settings("FrontierMining", "Tracker");

    settings.beginGroup("Auditor");
    m_defaultSavePathEdit->setText(settings.value("defaultSavePath", "").toString());
    m_autoParseCheckbox->setChecked(settings.value("autoParse", false).toBool());
    m_showRawDataCheckbox->setChecked(settings.value("showRawData", true).toBool());
    m_maxTransactionsSpin->setValue(settings.value("maxTransactions", 50).toInt());
    settings.endGroup();

    // Apply settings
    onSettingsChanged();
}

void AuditorWidget::saveSettings()
{
    QSettings settings("FrontierMining", "Tracker");

    settings.beginGroup("Auditor");
    settings.setValue("defaultSavePath", m_defaultSavePathEdit->text());
    settings.setValue("autoParse", m_autoParseCheckbox->isChecked());
    settings.setValue("showRawData", m_showRawDataCheckbox->isChecked());
    settings.setValue("maxTransactions", m_maxTransactionsSpin->value());
    settings.endGroup();
}

void AuditorWidget::onBrowseSaveFile()
{
    // Use default path if set
    QString startPath = m_defaultSavePathEdit->text();
    if (startPath.isEmpty()) {
        startPath = QString();
    }

    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Select Save File",
        startPath,
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

        // Auto-parse if enabled
        if (m_autoParseCheckbox->isChecked()) {
            onParseSaveFile();
        }
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

        if (transactionCount >= m_maxTransactionsSpin->value()) break;  // Use setting
    }

    m_transactionCountLabel->setText(QString("%1").arg(transactionCount));

    // Store parsed data for validation
    m_parsedData.valid = true;
    m_parsedData.transactionCount = transactionCount;
    m_parsedData.totalSales = totalSales;
    m_parsedData.totalPurchases = totalPurchases;

    // Get money value
    if (moneyPos != -1) {
        qint32 money;
        memcpy(&money, data.constData() + moneyPos + 34, sizeof(qint32));
        m_parsedData.money = money;
    }

    m_parsedData.map = m_mapNameLabel->text();

    // Resize columns
    m_transactionsTable->resizeColumnsToContents();

    m_rawDataView->append(QString("\nTransactions parsed: %1").arg(transactionCount));
    m_rawDataView->append(QString("Total Sales: $%L1").arg(totalSales));
    m_rawDataView->append(QString("Total Purchases: $%L1").arg(totalPurchases));
    m_rawDataView->append("\nParsing complete.");
}

void AuditorWidget::onRunValidation()
{
    // Check if save file has been parsed
    if (!m_parsedData.valid) {
        QMessageBox::warning(this, "Validation",
                             "Please load and parse a save file first.");
        return;
    }

    m_validationModel->removeRows(0, m_validationModel->rowCount());

    // Get ledger data from database
    auto ledgerTransactions = m_database->getAllTransactions();

    double ledgerSales = 0;
    double ledgerPurchases = 0;

    for (const auto &trans : ledgerTransactions) {
        if (trans.totalAmount > 0) {
            ledgerSales += trans.totalAmount;
        } else {
            ledgerPurchases += qAbs(trans.totalAmount);
        }
    }

    // Get ledger balance (simplified - sum of all transactions)
    // In a full implementation, you'd track account balances properly
    double ledgerBalance = ledgerSales - ledgerPurchases;

    int discrepancyCount = 0;

    // Helper lambda to add a row
    auto addRow = [this, &discrepancyCount](const QString &field,
                                            const QString &ledgerVal,
                                            const QString &saveVal,
                                            double diff) {
        QList<QStandardItem*> row;
        row << new QStandardItem(field);
        row << new QStandardItem(ledgerVal);
        row << new QStandardItem(saveVal);

        QString diffStr;
        if (diff > 0) {
            diffStr = QString("+$%L1").arg(diff, 0, 'f', 0);
        } else if (diff < 0) {
            diffStr = QString("-$%L1").arg(qAbs(diff), 0, 'f', 0);
        } else {
            diffStr = "Match";
        }
        row << new QStandardItem(diffStr);

        // Color code
        bool isMatch = (qAbs(diff) < 0.01);
        QColor color = isMatch ? QColor(0, 128, 0) : Qt::red;

        for (auto item : row) {
            item->setForeground(color);
        }

        if (!isMatch) {
            discrepancyCount++;
        }

        m_validationModel->appendRow(row);
    };

    // Compare Personal Money / Balance
    double moneyDiff = m_parsedData.money - ledgerBalance;
    addRow("Personal Money",
           QString("$%L1").arg(ledgerBalance, 0, 'f', 0),
           QString("$%L1").arg(m_parsedData.money),
           moneyDiff);

    // Compare Transaction Count
    int countDiff = m_parsedData.transactionCount - ledgerTransactions.size();
    addRow("Transaction Count",
           QString::number(ledgerTransactions.size()),
           QString::number(m_parsedData.transactionCount),
           countDiff);

    // Compare Total Sales
    double salesDiff = m_parsedData.totalSales - ledgerSales;
    addRow("Total Sales",
           QString("$%L1").arg(ledgerSales, 0, 'f', 0),
           QString("$%L1").arg(m_parsedData.totalSales, 0, 'f', 0),
           salesDiff);

    // Compare Total Purchases
    double purchasesDiff = m_parsedData.totalPurchases - ledgerPurchases;
    addRow("Total Purchases",
           QString("$%L1").arg(ledgerPurchases, 0, 'f', 0),
           QString("$%L1").arg(m_parsedData.totalPurchases, 0, 'f', 0),
           purchasesDiff);

    // Resize columns
    m_validationTable->resizeColumnsToContents();

    // Update summary
    if (discrepancyCount == 0) {
        m_validationSummaryLabel->setText(
            "<span style='color: green; font-weight: bold;'>✓ All values match!</span>");
    } else {
        m_validationSummaryLabel->setText(
            QString("<span style='color: red; font-weight: bold;'>⚠ Found %1 discrepanc%2</span>")
                .arg(discrepancyCount)
                .arg(discrepancyCount == 1 ? "y" : "ies"));
    }
}
