#pragma once

#include <QAbstractTableModel>

#include <optional>

#include <pdt/core/dataset.h>

namespace pdv {

class CsvSamplesTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit CsvSamplesTableModel(QObject* parent = nullptr);

    void setDataSet(const std::optional<pdt::DataSet>& dataSet);
    void clear();

    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
    std::optional<pdt::DataSet> m_dataSet;
};

} // namespace pdv
