#include "imageutil.h"

#include <QBuffer>
#include <QImage>

namespace ImageUtil
{
static QImage load(const QString &path, int maxEdge)
{
    QImage img(path);
    if (img.isNull())
        return img;
    if (qMax(img.width(), img.height()) > maxEdge)
        img = img.scaled(maxEdge, maxEdge, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    return img;
}

bool encodeForUpload(const QString &path, QByteArray &base64, QString &mime, int maxEdge)
{
    const QImage img = load(path, maxEdge);
    if (img.isNull())
        return false;
    QByteArray bytes;
    QBuffer buf(&bytes);
    buf.open(QIODevice::WriteOnly);
    // PNG keeps small text crisp, which is what screenshots are mostly made of.
    img.save(&buf, "PNG");
    mime = QStringLiteral("image/png");
    base64 = bytes.toBase64();
    return true;
}

QString thumbnailDataUrl(const QString &path, int maxEdge)
{
    const QImage img = load(path, maxEdge);
    if (img.isNull())
        return {};
    QByteArray bytes;
    QBuffer buf(&bytes);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "JPEG", 85);
    return QStringLiteral("data:image/jpeg;base64,") + QString::fromLatin1(bytes.toBase64());
}
}
