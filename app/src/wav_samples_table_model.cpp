#include "pdv/wav_samples_table_model.h"

namespace pdv {

WavSamplesTableModel::WavSamplesTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

void WavSamplesTableModel::setWavData(const std::optional<pdt::WavData> &wavData)
{
    beginResetModel();
    m_wavData = wavData;
    endResetModel();
}

void WavSamplesTableModel::clear()
{
    beginResetModel();
    m_wavData.reset();
    endResetModel();
}

int WavSamplesTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || !m_wavData.has_value()) {
        return 0;
    }

    return static_cast<int>(m_wavData->samples.size());
}

int WavSamplesTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return 3;
}

QVariant WavSamplesTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !m_wavData.has_value()) {
        return {};
    }

    if (role != Qt::DisplayRole) {
        return {};
    }

    const auto row = static_cast<std::size_t>(index.row());
    const auto& samples = m_wavData->samples;

    if (row >= samples.size()) {
        return {};
    }

    switch (index.column()) {
    case 0:
        return static_cast<qulonglong>(row);
    case 1: {
        const double timeSeconds =
            static_cast<double>(row) / static_cast<double>(m_wavData->sample_rate);
        return timeSeconds;
    }
    case 2:
        return samples[row];
    default:
        return {};
    }
}

QVariant WavSamplesTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return {};
    }

    if (orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
            return "Index";
        case 1:
            return "Time [s]";
        case 2:
            return "Amplitude";
        default:
            return {};
        }
    }

    return section + 1;
}

} // namespace pdv
