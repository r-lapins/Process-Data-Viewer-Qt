#include "pdv/main_window.h"

#include <algorithm>
#include <cmath>
#include <numeric>

#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QLabel>
#include <QMenuBar>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QTableView>
#include <QHeaderView>
#include <QFormLayout>

namespace pdv {

MainWindow::MainWindow()
{
    resize(1000, 700);

    m_csvSamplesModel = new CsvSamplesTableModel(this);
    m_wavSamplesModel = new WavSamplesTableModel(this);

    createMenu();
    createCentralWorkspace();
    resetStatisticsPanel();
    statusBar()->showMessage("Ready");
    updateWindowTitle();
}

void MainWindow::createMenu()
{
    auto fileMenu = menuBar()->addMenu("File");
    auto openAction = fileMenu->addAction("Open");

    connect(openAction, &QAction::triggered,
            this, &MainWindow::openFile);
}

void MainWindow::createCentralWorkspace()
{
    auto* centralWidget = new QWidget(this);
    auto* centralLayout = new QVBoxLayout(centralWidget);
    centralLayout->setContentsMargins(8, 8, 8, 8);

    auto* splitter = new QSplitter(Qt::Horizontal, centralWidget);

    // Left panel - data area
    auto* dataGroup = new QGroupBox("Data", splitter);
    auto* dataLayout = new QVBoxLayout(dataGroup);

    m_samplesTableView = new QTableView(dataGroup);
    m_samplesTableView->setModel(m_csvSamplesModel);
    m_samplesTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_samplesTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_samplesTableView->setAlternatingRowColors(true);
    m_samplesTableView->setSortingEnabled(false);
    m_samplesTableView->horizontalHeader()->setStretchLastSection(true);
    m_samplesTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

    dataLayout->addWidget(m_samplesTableView);

    // Right panel container
    auto* rightPanel = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    // Statistics section
    auto* statisticsGroup = new QGroupBox("Statistics", rightPanel);
    auto* statisticsLayout = new QFormLayout(statisticsGroup);

    m_statsFileTypeValueLabel = new QLabel("-", statisticsGroup);
    m_statsSampleRateValueLabel = new QLabel("-", statisticsGroup);
    m_statsChannelsValueLabel = new QLabel("-", statisticsGroup);
    m_statsCountValueLabel = new QLabel("-", statisticsGroup);
    m_statsMinValueLabel = new QLabel("-", statisticsGroup);
    m_statsMaxValueLabel = new QLabel("-", statisticsGroup);
    m_statsMeanValueLabel = new QLabel("-", statisticsGroup);
    m_statsStddevValueLabel = new QLabel("-", statisticsGroup);

    statisticsLayout->addRow("File type:", m_statsFileTypeValueLabel);
    statisticsLayout->addRow("Sample rate:", m_statsSampleRateValueLabel);
    statisticsLayout->addRow("Channels:", m_statsChannelsValueLabel);
    statisticsLayout->addRow("Count:", m_statsCountValueLabel);
    statisticsLayout->addRow("Min:", m_statsMinValueLabel);
    statisticsLayout->addRow("Max:", m_statsMaxValueLabel);
    statisticsLayout->addRow("Mean:", m_statsMeanValueLabel);
    statisticsLayout->addRow("Stddev:", m_statsStddevValueLabel);

    // Alerts section
    auto* alertsGroup = new QGroupBox("Alerts", rightPanel);
    auto* alertsLayout = new QVBoxLayout(alertsGroup);

    m_alertsPlaceholderLabel = new QLabel("No alerts available", alertsGroup);
    m_alertsPlaceholderLabel->setWordWrap(true);
    alertsLayout->addWidget(m_alertsPlaceholderLabel);
    alertsLayout->addStretch();

    rightLayout->addWidget(statisticsGroup);
    rightLayout->addWidget(alertsGroup);

    splitter->addWidget(dataGroup);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    centralLayout->addWidget(splitter);
    setCentralWidget(centralWidget);
}

void MainWindow::openFile()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        "Open file",
        QString(),
        "Supported files (*.csv *.wav);;CSV files (*.csv);;WAV files (*.wav);;All files (*)"
        );

    if (filePath.isEmpty()) {
        statusBar()->showMessage("Open file canceled", 2000);
        return;
    }

    const LoadResult result = m_fileLoaderService.loadFile(filePath);

    if (!result.success) {
        m_currentSession.reset();
        clearLoadedData();
        resetStatisticsPanel();
        updateWindowTitle();

        if (m_alertsPlaceholderLabel != nullptr) {
            m_alertsPlaceholderLabel->setText("No alerts available");
        }

        statusBar()->showMessage(
            QString("Failed to load file: %1").arg(result.errorMessage),
            5000
            );
        return;
    }

    m_currentSession = result.session;
    displaySessionData();
    updateStatisticsPanel();
    updateWindowTitle();

    if (m_alertsPlaceholderLabel != nullptr) {
        m_alertsPlaceholderLabel->setText("Alerts panel is ready");
    }

    const QFileInfo fileInfo(result.session.filePath);

    switch (result.session.kind) {
    case SessionData::FileKind::Csv:
        statusBar()->showMessage(
            QString("Loaded CSV file: %1").arg(fileInfo.fileName()),
            5000
            );
        break;

    case SessionData::FileKind::Wav:
        statusBar()->showMessage(
            QString("Loaded WAV file: %1").arg(fileInfo.fileName()),
            5000
            );
        break;

    case SessionData::FileKind::Unknown:
    default:
        statusBar()->showMessage(
            QString("Loaded file: %1").arg(fileInfo.fileName()),
            5000
            );
        break;
    }
}

void MainWindow::updateWindowTitle()
{
    constexpr auto kAppTitle = "Process Data Viewer";

    if (!m_currentSession.has_value()) {
        setWindowTitle(kAppTitle);
        return;
    }

    const QFileInfo fileInfo(m_currentSession->filePath);
    setWindowTitle(QString("%1 - %2").arg(kAppTitle, fileInfo.fileName()));
}

void MainWindow::clearLoadedData()
{
    if (m_csvSamplesModel != nullptr) {
        m_csvSamplesModel->clear();
    }

    if (m_wavSamplesModel != nullptr) {
        m_wavSamplesModel->clear();
    }

    if (m_samplesTableView != nullptr) {
        m_samplesTableView->setModel(m_csvSamplesModel);
    }
}

void MainWindow::displaySessionData()
{
    if (!m_currentSession.has_value() || m_samplesTableView == nullptr) {
        clearLoadedData();
        return;
    }

    switch (m_currentSession->kind) {
    case SessionData::FileKind::Csv:
        m_wavSamplesModel->clear();
        m_csvSamplesModel->setDataSet(m_currentSession->dataSet);
        m_samplesTableView->setModel(m_csvSamplesModel);
        break;

    case SessionData::FileKind::Wav:
        m_csvSamplesModel->clear();
        m_wavSamplesModel->setWavData(m_currentSession->wavData);
        m_samplesTableView->setModel(m_wavSamplesModel);
        break;

    case SessionData::FileKind::Unknown:
    default:
        clearLoadedData();
        break;
    }

    m_samplesTableView->resizeColumnsToContents();
}

void MainWindow::resetStatisticsPanel()
{
    if (m_statsFileTypeValueLabel != nullptr) {
        m_statsFileTypeValueLabel->setText("-");
    }

    if (m_statsSampleRateValueLabel != nullptr) {
        m_statsSampleRateValueLabel->setText("-");
    }

    if (m_statsChannelsValueLabel != nullptr) {
        m_statsChannelsValueLabel->setText("-");
    }

    if (m_statsCountValueLabel != nullptr) {
        m_statsCountValueLabel->setText("-");
    }

    if (m_statsMinValueLabel != nullptr) {
        m_statsMinValueLabel->setText("-");
    }

    if (m_statsMaxValueLabel != nullptr) {
        m_statsMaxValueLabel->setText("-");
    }

    if (m_statsMeanValueLabel != nullptr) {
        m_statsMeanValueLabel->setText("-");
    }

    if (m_statsStddevValueLabel != nullptr) {
        m_statsStddevValueLabel->setText("-");
    }
}

void MainWindow::updateStatisticsPanel()
{
    resetStatisticsPanel();

    if (!m_currentSession.has_value()) {
        return;
    }

    switch (m_currentSession->kind) {
    case SessionData::FileKind::Csv: {
        m_statsFileTypeValueLabel->setText("CSV");

        if (!m_currentSession->dataSet.has_value()) {
            return;
        }

        const auto stats = m_currentSession->dataSet->stats();

        m_statsCountValueLabel->setText(QString::number(static_cast<qulonglong>(stats.count)));
        m_statsMinValueLabel->setText(QString::number(stats.min, 'g', 10));
        m_statsMaxValueLabel->setText(QString::number(stats.max, 'g', 10));
        m_statsMeanValueLabel->setText(QString::number(stats.mean, 'g', 10));
        m_statsStddevValueLabel->setText(QString::number(stats.stddev, 'g', 10));

        break;
    }

    case SessionData::FileKind::Wav: {
        m_statsFileTypeValueLabel->setText("WAV");

        if (!m_currentSession->wavData.has_value()) {
            return;
        }

        const auto& wav = *m_currentSession->wavData;
        const auto& samples = wav.samples;

        m_statsSampleRateValueLabel->setText(QString::number(wav.sample_rate));
        m_statsChannelsValueLabel->setText(QString::number(wav.channels));
        m_statsCountValueLabel->setText(QString::number(static_cast<qulonglong>(samples.size())));

        if (samples.empty()) {
            return;
        }

        const auto [minIt, maxIt] = std::minmax_element(samples.begin(), samples.end());
        const double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
        const double mean = sum / static_cast<double>(samples.size());

        double sqSum = 0.0;
        for (double sample : samples) {
            const double diff = sample - mean;
            sqSum += diff * diff;
        }

        const double stddev = std::sqrt(sqSum / static_cast<double>(samples.size()));

        m_statsMinValueLabel->setText(QString::number(*minIt, 'g', 10));
        m_statsMaxValueLabel->setText(QString::number(*maxIt, 'g', 10));
        m_statsMeanValueLabel->setText(QString::number(mean, 'g', 10));
        m_statsStddevValueLabel->setText(QString::number(stddev, 'g', 10));

        break;
    }

    case SessionData::FileKind::Unknown:
    default:
        break;
    }
}

} // namespace pdv
