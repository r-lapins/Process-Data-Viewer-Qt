#pragma once

#include <QAbstractTableModel>

#include <optional>

#include <pdt/signal/wav_reader.h>

namespace pdv {

class WavSamplesTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit WavSamplesTableModel(QObject* parent = nullptr);

    void setWavData(const std::optional<pdt::WavData>& wavData);
    void clear();

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    std::optional<pdt::WavData> m_wavData;
};

} // namespace pdv
