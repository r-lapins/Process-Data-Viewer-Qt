#include "pdv/csv/csv_samples_table_model.h"

#include <QString>

#include <chrono>
#include <iomanip>
#include <sstream>

namespace pdv {
namespace {

QString formatTimestamp(std::chrono::sys_seconds ts)
{
    using namespace std::chrono;

    const auto dayPoint = floor<days>(ts);
    const year_month_day ymd{dayPoint};
    const hh_mm_ss timeOfDay{ts - dayPoint};

    std::ostringstream oss;
    oss << std::setw(4) << std::setfill('0') << int(ymd.year()) << "-"
        << std::setw(2) << unsigned(ymd.month()) << "-"
        << std::setw(2) << unsigned(ymd.day()) << " - "
        << std::setw(2) << timeOfDay.hours().count() << ":"
        << std::setw(2) << timeOfDay.minutes().count() << ":"
        << std::setw(2) << timeOfDay.seconds().count();

    return QString::fromStdString(oss.str());
}

} // namespace

CsvSamplesTableModel::CsvSamplesTableModel(QObject *parent) : QAbstractTableModel(parent)
{
}

void CsvSamplesTableModel::setDataSet(const pdt::DataSet &dataSet)
{
    beginResetModel();
    m_dataSet = dataSet;
    endResetModel();
}

void CsvSamplesTableModel::clear()
{
    beginResetModel();
    m_dataSet.reset();
    endResetModel();
}

int CsvSamplesTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || !m_dataSet.has_value()) { return 0; }
    return static_cast<int>(m_dataSet->samples().size());
}

int CsvSamplesTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) { return 0; }
    return 3;
}

QVariant CsvSamplesTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !m_dataSet.has_value()) { return {}; }

    if (role != Qt::DisplayRole) { return {}; }

    const auto& samples = m_dataSet->samples();
    const auto row      = static_cast<std::size_t>(index.row());

    if (row >= samples.size()) { return {}; }

    const auto& sample = samples[row];

    switch (index.column()) {
    case 0:     return formatTimestamp(sample.timestamp);
    case 1:     return QString::fromStdString(sample.sensor);
    case 2:     return sample.value;
    default:    return {};
    }
}

QVariant CsvSamplesTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) { return {}; }

    if (orientation == Qt::Horizontal) {
        switch (section) {
        case 0:     return "Timestamp";
        case 1:     return "Sensor";
        case 2:     return "Value";
        default:    return {};
        }
    }

    return section + 1;
}
} // namespace pdv
