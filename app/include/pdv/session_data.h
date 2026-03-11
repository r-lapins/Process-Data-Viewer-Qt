#pragma once

#include <QString>

#include <optional>

#include <pdt/dataset.h>
#include <pdt/wav_reader.h>

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
};

struct LoadResult
{
    bool success = false;
    QString errorMessage;
    SessionData session;
};

} // namespace pdv
