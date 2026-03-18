#include "pdv/main_window.h"
#include <pdt/core/anomaly.h>
#include <pdt/signal/fft.h>
#include <pdt/signal/peak_detection.h>
#include <pdt/signal/window.h>

#include <algorithm>
#include <cmath>
#include <numeric>

#include <QApplication>
#include <QtConcurrent/QtConcurrentRun>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QTableView>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

#include <QToolBar>
#include <QAction>
#include <QDir>

namespace pdv {

MainWindow::MainWindow()
{
    resize(1000, 700);

    m_csvSamplesModel = new CsvSamplesTableModel(this);
    m_wavSamplesModel = new WavSamplesTableModel(this);
    m_loadWatcher = new QFutureWatcher<LoadResult>(this);

    connect(m_loadWatcher, &QFutureWatcher<LoadResult>::finished,
            this, &MainWindow::handleLoadFinished);

    createMenu();
    createToolbar();
    createCentralWorkspace();
    resetStatisticsPanel();
    resetAlertsPanel();
    statusBar()->showMessage("Ready");
    updateWindowTitle();
}

void MainWindow::createMenu()
{
    auto* fileMenu = menuBar()->addMenu("File");
    m_openAction = fileMenu->addAction("Open");

    connect(m_openAction, &QAction::triggered,
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

    m_dataPlaceholderLabel = new QLabel("No data loaded", dataGroup);
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

    m_dataPlaceholderLabel->show();
    m_samplesTableView->hide();

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

    m_alertsListWidget = new QListWidget(alertsGroup);
    alertsLayout->addWidget(m_alertsListWidget);
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
    const QString filePath = QFileDialog::getOpenFileName(this, "Open file", QString(),
                                                          "Supported files (*.csv *.wav);;CSV files (*.csv);;WAV files (*.wav);;All files (*)");

    if (filePath.isEmpty()) {
        statusBar()->showMessage("Open file canceled", 2000);
        return;
    }

    loadFileAsync(filePath);
}

void MainWindow::loadFileAsync(const QString &filePath)
{
    if (m_isLoading) {
        return;
    }

    setLoadingUiState(true);

    auto future = QtConcurrent::run([this, filePath]() {
        return m_fileLoaderService.loadFile(filePath);
    });

    m_loadWatcher->setFuture(future);
}

void MainWindow::handleLoadFinished()
{
    setLoadingUiState(false);

    const LoadResult result = m_loadWatcher->result();

    if (!result.success) {
        m_currentSession.reset();
        clearLoadedData();
        resetStatisticsPanel();
        resetAlertsPanel();
        updateWindowTitle();


        statusBar()->showMessage(
            QString("Failed to load file: %1").arg(result.errorMessage),
            5000
            );
        return;
    }

    m_currentSession = result.session;
    displaySessionData();
    updateStatisticsPanel();
    updateAlertsPanel();
    updateWindowTitle();

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

void MainWindow::setLoadingUiState(bool loading)
{
    m_isLoading = loading;

    if (m_openAction != nullptr) {
        m_openAction->setEnabled(!loading);
    }

    if (loading) {
        statusBar()->showMessage("Loading file...");
        QApplication::setOverrideCursor(Qt::WaitCursor);

        if (m_dataPlaceholderLabel != nullptr) {
            m_dataPlaceholderLabel->setText("Loading file...");
            m_dataPlaceholderLabel->show();
        }

        if (m_samplesTableView != nullptr) {
            m_samplesTableView->hide();
        }
    } else {
        QApplication::restoreOverrideCursor();
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
        // m_samplesTableView->hide();
    }

    if (m_dataPlaceholderLabel != nullptr) {
        m_dataPlaceholderLabel->setText("No data loaded");
        m_dataPlaceholderLabel->show();
    }
}

void MainWindow::displaySessionData()
{
    if (!m_currentSession.has_value()) {
        clearLoadedData();
        return;
    }

    switch (m_currentSession->kind) {
    case SessionData::FileKind::Csv:
        m_wavSamplesModel->clear();
        m_csvSamplesModel->setDataSet(m_currentSession->dataSet);

        if (m_samplesTableView != nullptr) {
            m_samplesTableView->setModel(m_csvSamplesModel);
            m_samplesTableView->show();
            m_samplesTableView->resizeColumnsToContents();

            const int col = 0;
            int width = m_samplesTableView->columnWidth(col);
            m_samplesTableView->setColumnWidth(col, width + 20);
        }

        if (m_dataPlaceholderLabel != nullptr) {
            m_dataPlaceholderLabel->hide();
        }
        break;

    case SessionData::FileKind::Wav:
        m_csvSamplesModel->clear();
        m_wavSamplesModel->clear();

        if (m_samplesTableView != nullptr) {
            m_samplesTableView->hide();
        }

        if (m_dataPlaceholderLabel != nullptr) {
            m_dataPlaceholderLabel->setText(
                "WAV sample table is hidden.\n"
                "See statistics and spectral peak results on the right."
                );
            m_dataPlaceholderLabel->show();
        }
        break;

    case SessionData::FileKind::Unknown:
    default:
        clearLoadedData();
        break;
    }

    // if (!m_currentSession.has_value() || m_samplesTableView == nullptr) {
    //     clearLoadedData();
    //     return;
    // }

    // switch (m_currentSession->kind) {
    // case SessionData::FileKind::Csv:
    //     m_wavSamplesModel->clear();
    //     m_csvSamplesModel->setDataSet(m_currentSession->dataSet);
    //     m_samplesTableView->setModel(m_csvSamplesModel);
    //     break;

    // case SessionData::FileKind::Wav:
    //     m_csvSamplesModel->clear();
    //     m_wavSamplesModel->setWavData(m_currentSession->wavData);
    //     m_samplesTableView->setModel(m_wavSamplesModel);
    //     break;

    // case SessionData::FileKind::Unknown:
    // default:
    //     clearLoadedData();
    //     break;
    // }

    // m_samplesTableView->resizeColumnsToContents();
}

void MainWindow::createToolbar()
{
    auto* toolbar = addToolBar("Quick Access");
    toolbar->setMovable(false);

    auto* quickOpenAction = toolbar->addAction("Quick Open");

    connect(quickOpenAction, &QAction::triggered,
            this, &MainWindow::openFileFromDataFolder);
}

void MainWindow::openFileFromDataFolder()
{
    const QString startDir = QDir::homePath() + "/home/lapin/Documents/QtProjekty/process_data_viewer_qt/examples/";

    const QString filePath = QFileDialog::getOpenFileName(
        this,
        "Open file from data folder",
        startDir,
        "Supported files (*.csv *.wav);;CSV files (*.csv);;WAV files (*.wav);;All files (*)"
        );

    if (filePath.isEmpty()) {
        statusBar()->showMessage("Open file canceled", 2000);
        return;
    }

    loadFileAsync(filePath);
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

void MainWindow::resetAlertsPanel()
{
    if (m_alertsListWidget == nullptr) {
        return;
    }

    m_alertsListWidget->clear();
    m_alertsListWidget->addItem("No alerts available");
}

void MainWindow::updateAlertsPanel()
{
    resetAlertsPanel();

    if (!m_currentSession.has_value() || m_alertsListWidget == nullptr) {
        return;
    }

    m_alertsListWidget->clear();

    switch (m_currentSession->kind) {
    case SessionData::FileKind::Csv: {
        if (!m_currentSession->dataSet.has_value()) {
            m_alertsListWidget->addItem("No CSV dataset available");
            return;
        }

        const auto summary = pdt::detect_zscore_global(*m_currentSession->dataSet, 3.0, 20);

        if (summary.count == 0 || summary.top.empty()) {
            m_alertsListWidget->addItem("No anomalies detected");
            return;
        }

        m_alertsListWidget->addItem(
            QString("Detected anomalies: %1").arg(static_cast<qulonglong>(summary.count))
            );

        for (const auto& anomaly : summary.top) {
            const auto ts = std::chrono::system_clock::to_time_t(anomaly.timestamp);

            const QString timestampText = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(ts)).toString(Qt::ISODate);
            // const QString timestampText = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(ts), Qt::UTC).toString(Qt::ISODate);

            const QString itemText = QString("time=%1 | sensor=%2 | value=%3 | z=%4")
                                         .arg(timestampText).arg(QString::fromStdString(anomaly.sensor))
                                         .arg(anomaly.value, 0, 'g', 10).arg(anomaly.zscore, 0, 'g', 10);

            m_alertsListWidget->addItem(itemText);
        }

        break;
    }

    case SessionData::FileKind::Wav: {
        if (!m_currentSession->wavData.has_value()) {
            m_alertsListWidget->addItem("No WAV data available");
            return;
        }

        const auto& wav = *m_currentSession->wavData;
        const auto& samples = wav.samples;

        if (samples.size() < 8 || wav.sample_rate == 0) {
            m_alertsListWidget->addItem("Not enough samples for spectrum analysis");
            return;
        }

        constexpr std::size_t kMaxSpectrumSamples = 4096;
        const std::size_t n = std::min(samples.size(), kMaxSpectrumSamples);

        std::vector<double> segment(samples.begin(), samples.begin() + static_cast<std::ptrdiff_t>(n));

        const auto windowed = pdt::apply_window(segment, pdt::WindowType::Hann);
        const auto spectrum = pdt::compute_spectrum(windowed, static_cast<double>(wav.sample_rate));

        if (spectrum.frequencies.empty() || spectrum.magnitudes.empty()) {
            m_alertsListWidget->addItem("Spectrum analysis failed");
            return;
        }

        const auto peaks = pdt::detect_dominant_peaks(
            spectrum,
            0.20,
            pdt::PeakDetectionMode::LocalMaxima,
            10
            );

        if (peaks.empty()) {
            m_alertsListWidget->addItem("No dominant spectral peaks detected");
            return;
        }

        m_alertsListWidget->addItem(
            QString("Detected spectral peaks: %1 (first %2 samples)")
                .arg(static_cast<qulonglong>(peaks.size()))
                .arg(static_cast<qulonglong>(n))
            );

        for (const auto& peak : peaks) {
            const QString itemText = QString("bin=%1 | freq=%2 Hz | mag=%3")
            .arg(static_cast<qulonglong>(peak.index))
                .arg(peak.frequency, 0, 'g', 10)
                .arg(peak.magnitude, 0, 'g', 10);

            m_alertsListWidget->addItem(itemText);
        }

        break;
    }

    case SessionData::FileKind::Unknown:
        m_alertsListWidget->addItem("No alerts available");
        break;
    }
}

} // namespace pdv
