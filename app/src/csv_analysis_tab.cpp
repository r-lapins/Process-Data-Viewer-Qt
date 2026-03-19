#include "pdv/csv_analysis_tab.h"

#include <pdt/core/anomaly.h>

#include <QDateTime>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QSplitter>
#include <QTableView>
#include <QVBoxLayout>

namespace pdv {

CsvAnalysisTab::CsvAnalysisTab(const SessionData& session, QWidget* parent)
    : AnalysisTab(session, parent)
{
    m_csvSamplesModel = new CsvSamplesTableModel(this);

    createUi();
    displaySessionData();
    updateStatisticsPanel();
    updateAlertsPanel();
}

void CsvAnalysisTab::createUi()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(6, 6, 6, 6);

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    auto* dataGroup = new QGroupBox("Data", splitter);
    auto* dataLayout = new QVBoxLayout(dataGroup);

    m_dataPlaceholderLabel = new QLabel("No CSV data", dataGroup);
    m_dataPlaceholderLabel->setWordWrap(true);

    m_samplesTableView = new QTableView(dataGroup);
    m_samplesTableView->setModel(m_csvSamplesModel);
    m_samplesTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_samplesTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_samplesTableView->setAlternatingRowColors(true);
    m_samplesTableView->setSortingEnabled(false);
    m_samplesTableView->horizontalHeader()->setStretchLastSection(true);
    m_samplesTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    dataLayout->addWidget(m_dataPlaceholderLabel);
    dataLayout->addWidget(m_samplesTableView);

    auto* rightPanel = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(rightPanel);

    auto* statsGroup = new QGroupBox("Statistics", rightPanel);
    auto* statsLayout = new QFormLayout(statsGroup);

    m_statsFileTypeValueLabel = new QLabel("-", statsGroup);
    m_statsCountValueLabel = new QLabel("-", statsGroup);
    m_statsMinValueLabel = new QLabel("-", statsGroup);
    m_statsMaxValueLabel = new QLabel("-", statsGroup);
    m_statsMeanValueLabel = new QLabel("-", statsGroup);
    m_statsStddevValueLabel = new QLabel("-", statsGroup);

    statsLayout->addRow("File type:", m_statsFileTypeValueLabel);
    statsLayout->addRow("Count:", m_statsCountValueLabel);
    statsLayout->addRow("Min:", m_statsMinValueLabel);
    statsLayout->addRow("Max:", m_statsMaxValueLabel);
    statsLayout->addRow("Mean:", m_statsMeanValueLabel);
    statsLayout->addRow("Stddev:", m_statsStddevValueLabel);

    auto* alertsGroup = new QGroupBox("Alerts", rightPanel);
    auto* alertsLayout = new QVBoxLayout(alertsGroup);

    m_alertsListWidget = new QListWidget(alertsGroup);
    alertsLayout->addWidget(m_alertsListWidget);

    rightLayout->addWidget(statsGroup);
    rightLayout->addWidget(alertsGroup);

    splitter->addWidget(dataGroup);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    rootLayout->addWidget(splitter);

    m_samplesTableView->hide();
}

void CsvAnalysisTab::displaySessionData()
{
    if (!m_session.dataSet.has_value()) {
        return;
    }

    m_csvSamplesModel->setDataSet(*m_session.dataSet);
    m_samplesTableView->show();
    m_dataPlaceholderLabel->hide();

    const int col = 0;
    const int width = m_samplesTableView->columnWidth(col);
    m_samplesTableView->setColumnWidth(col, width + 20);
}

void CsvAnalysisTab::resetStatisticsPanel()
{
    m_statsFileTypeValueLabel->setText("-");
    m_statsCountValueLabel->setText("-");
    m_statsMinValueLabel->setText("-");
    m_statsMaxValueLabel->setText("-");
    m_statsMeanValueLabel->setText("-");
    m_statsStddevValueLabel->setText("-");
}

void CsvAnalysisTab::updateStatisticsPanel()
{
    resetStatisticsPanel();

    if (!m_session.dataSet.has_value()) {
        return;
    }

    const auto stats = m_session.dataSet->stats();

    m_statsFileTypeValueLabel->setText("CSV");
    m_statsCountValueLabel->setText(QString::number(static_cast<qulonglong>(stats.count)));
    m_statsMinValueLabel->setText(QString::number(stats.min, 'g', 10));
    m_statsMaxValueLabel->setText(QString::number(stats.max, 'g', 10));
    m_statsMeanValueLabel->setText(QString::number(stats.mean, 'g', 10));
    m_statsStddevValueLabel->setText(QString::number(stats.stddev, 'g', 10));
}

void CsvAnalysisTab::resetAlertsPanel()
{
    m_alertsListWidget->clear();
    m_alertsListWidget->addItem("No alerts");
}

void CsvAnalysisTab::updateAlertsPanel()
{
    resetAlertsPanel();

    if (!m_session.dataSet.has_value()) {
        return;
    }

    const auto summary = pdt::detect_zscore_global(*m_session.dataSet, 3.0, 20);

    m_alertsListWidget->clear();

    if (summary.top.empty()) {
        m_alertsListWidget->addItem("No anomalies");
        return;
    }

    for (const auto& a : summary.top) {
        const auto ts = std::chrono::system_clock::to_time_t(a.timestamp);
        const QString timestampText =
            QDateTime::fromSecsSinceEpoch(static_cast<qint64>(ts)).toString(Qt::ISODate);

        m_alertsListWidget->addItem(
            QString("time=%1 | sensor=%2 | value=%3 | z=%4")
                .arg(timestampText,
                     QString::fromStdString(a.sensor),
                     QString::number(a.value, 'g', 10),
                     QString::number(a.zscore, 'g', 10))
        );
    }
}

} // namespace pdv
