#pragma once

#include <QString>

#include "pdv/session_data.h"

namespace pdv {

class FileLoaderService {
public:
    LoadResult loadFile(const QString& filePath) const;

private:
    LoadResult loadCsv(const QString& filePath) const;
    LoadResult loadWav(const QString& filePath) const;
    static SessionData::FileKind detectFileKind(const QString& filePath);
};

} // namespace pdv
