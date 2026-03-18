#include "pdv/file_loader_service.h"

#include <QFileInfo>

#include <exception>
#include <fstream>

#include <pdt/core/csv_reader.h>
#include <pdt/core/dataset.h>
#include <pdt/signal/wav_reader.h>

namespace pdv {

LoadResult FileLoaderService::loadFile(const QString &filePath) const
{
    switch (detectFileKind(filePath)) {
    case SessionData::FileKind::Csv:
        return loadCsv(filePath);

    case SessionData::FileKind::Wav:
        return loadWav(filePath);

    case SessionData::FileKind::Unknown:
    default:
        return LoadResult{
            .success = false,
            .errorMessage = "Unsupported file type.",
            .session = {}
        };
    }
}

LoadResult FileLoaderService::loadCsv(const QString &filePath) const
{
    try {
        std::ifstream file(filePath.toStdString());
        if (!file.is_open()) {
            return LoadResult{
                .success = false,
                .errorMessage = "Failed to open CSV file.",
                .session = {}
            };
        }

        const auto importResult = pdt::read_csv(file);
        SessionData session;
        session.kind = SessionData::FileKind::Csv;
        session.filePath = filePath;
        session.dataSet = pdt::DataSet(importResult.samples);

        return LoadResult{
            .success = true,
            .errorMessage = {},
            .session = std::move(session)
        };

    } catch(const std::exception& ex) {
        return LoadResult{
            .success = false,
            .errorMessage = QString::fromUtf8(ex.what()),
            .session = {}
        };
    }
}

LoadResult FileLoaderService::loadWav(const QString &filePath) const
{
    try {
        const auto wavData = pdt::read_wav_pcm16_mono(filePath.toStdString());
        if (!wavData.has_value()) {
            return LoadResult{
                .success = false,
                .errorMessage = "Failed to read WAV file or unsupported format.",
                .session = {}
            };
        }

        SessionData session;
        session.kind = SessionData::FileKind::Wav;
        session.filePath = filePath;
        session.wavData = std::move(wavData);

        return LoadResult{
            .success = true,
            .errorMessage = {},
            .session = std::move(session)
        };
    } catch (const std::exception& ex) {
        return LoadResult{
            .success = false,
            .errorMessage = QString::fromUtf8(ex.what()),
            .session = {}
        };
    }
}

SessionData::FileKind FileLoaderService::detectFileKind(const QString &filePath)
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();

    if (suffix == "csv") {
        return SessionData::FileKind::Csv;
    }

    if (suffix == "wav") {
        return SessionData::FileKind::Wav;
    }

    return SessionData::FileKind::Unknown;
}

} // namespace pdv
