#pragma once
#include <QByteArray>
#include <QString>

namespace ImageUtil
{
// Loads `path`, downscales so the long edge is at most `maxEdge`, returns base64-encoded PNG/JPEG
// bytes in `data` and the mime type. Returns false if the file can't be read.
bool encodeForUpload(const QString &path, QByteArray &base64, QString &mime, int maxEdge = 1568);

// Small JPEG data: URL for the web view transcript.
QString thumbnailDataUrl(const QString &path, int maxEdge = 480);
}
