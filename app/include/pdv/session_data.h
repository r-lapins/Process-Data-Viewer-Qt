#pragma once

#include <QString>

#include <optional>

#include <pdt/core/dataset.h>
#include <pdt/signal/wav_reader.h>
#include <pdt/core/csv_reader.h>

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

    std::optional<pdt::DataSet> dataSet;
    std::optional<pdt::WavData> wavData;

    std::size_t skipped{0};
    std::vector<pdt::SkippedRow> skippedRows;
};

struct LoadResult
{
    bool success = false;
    QString errorMessage;
    SessionData session;
};

} // namespace pdv
