#pragma once

#include <QString>

#include <pdt/csv/dataset.h>
#include <pdt/wav/wav_reader.h>
#include <pdt/csv/csv_reader.h>

namespace pdv {

struct SessionData
{
    enum class FileKind {
        Unknown,
        Csv,
        Wav
    };

    FileKind kind = FileKind::Unknown;
    QString filePath;

    std::optional<pdt::CsvData> csvData;
    std::optional<pdt::DataSet> dataSet;
    std::optional<pdt::WavData> wavData;
};

struct LoadResult
{
    bool success = false;
    QString errorMessage;
    SessionData session;
};

} // namespace pdv
