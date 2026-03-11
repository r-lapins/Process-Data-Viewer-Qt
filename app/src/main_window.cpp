#include "pdv/main_window.h"

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

namespace pdv {

MainWindow::MainWindow()
{
    resize(1000, 700);

    m_csvSamplesModel = new CsvSamplesTableModel(this);
    m_wavSamplesModel = new WavSamplesTableModel(this);

    createMenu();
    createCentralWorkspace();
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
    auto* statisticsLayout = new QVBoxLayout(statisticsGroup);

    m_statisticsPlaceholderLabel = new QLabel("No statistics available", statisticsGroup);
    m_statisticsPlaceholderLabel->setWordWrap(true);
    statisticsLayout->addWidget(m_statisticsPlaceholderLabel);
    statisticsLayout->addStretch();

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
        this, "Open file",
        QString(), "Supported files (*.csv *.wav);;CSV files (*.csv);;WAV files (*.wav);;All files (*)"
        );

    if (filePath.isEmpty()) {
        statusBar()->showMessage("Open file canceled", 2000);
        return;
    }

    const LoadResult result = m_fileLoaderService.loadFile(filePath);

    if (!result.success) {
        m_currentSession.reset();
        clearLoadedData();
        updateWindowTitle();

        if (m_statisticsPlaceholderLabel != nullptr) {
            m_statisticsPlaceholderLabel->setText("No statistics available");
        }

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
    updateWindowTitle();

    if (m_statisticsPlaceholderLabel != nullptr) {
        m_statisticsPlaceholderLabel->setText("Statistics panel is ready");
    }

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

    if (m_samplesTableView!= nullptr) {
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

} // namespace pdv
