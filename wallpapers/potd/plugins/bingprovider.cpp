/*
 *   SPDX-FileCopyrightText: 2017 Weng Xuetian <wengxt@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "bingprovider.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>

#include <KIO/Job>
#include <KPluginFactory>

BingProvider::BingProvider(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : PotdProvider(parent, data, args)
    , m_screenDPI(args.size() >= 3 ? args[2].toDouble() : 1)
    , m_screenWidth(args.size() >= 2 ? args[0].toInt() * m_screenDPI : 0)
    , m_screenHeight(args.size() >= 2 ? args[1].toInt() * m_screenDPI : 0)
{
    const QUrl url(QStringLiteral("https://www.bing.com/HPImageArchive.aspx?format=js&idx=0&n=1"));

    KIO::StoredTransferJob *job = KIO::storedGet(url, KIO::NoReload, KIO::HideProgressInfo);
    connect(job, &KIO::StoredTransferJob::finished, this, &BingProvider::pageRequestFinished);
}

BingProvider::~BingProvider() = default;

void BingProvider::pageRequestFinished(KJob *_job)
{
    KIO::StoredTransferJob *job = static_cast<KIO::StoredTransferJob *>(_job);
    if (job->error()) {
        Q_EMIT error(this);
        return;
    }

    auto json = QJsonDocument::fromJson(job->data());
    do {
        if (json.isNull()) {
            break;
        }
        auto imagesArray = json.object().value(QLatin1String("images"));
        if (!imagesArray.isArray() || imagesArray.toArray().size() <= 0) {
            break;
        }
        auto imageObj = imagesArray.toArray().at(0);
        if (!imageObj.isObject()) {
            break;
        }
        auto url = imageObj.toObject().value(QLatin1String("urlbase"));
        QString urlString = url.isString() ? url.toString() : QString();
        if (urlString.isEmpty()) {
            break;
        }

        urlString = QStringLiteral("https://www.bing.com/") + urlString;

        if (m_screenWidth > 1920 || m_screenHeight > 1080) {
            // Use 4k wallpaper
            urlString += QStringLiteral("_UHD.jpg");
        } else {
            urlString += QStringLiteral("_1920x1080.jpg");
        }

        KIO::StoredTransferJob *imageJob = KIO::storedGet(QUrl(urlString), KIO::NoReload, KIO::HideProgressInfo);
        connect(imageJob, &KIO::StoredTransferJob::finished, this, &BingProvider::imageRequestFinished);
        return;
    } while (0);

    Q_EMIT error(this);
    return;
}

void BingProvider::imageRequestFinished(KJob *_job)
{
    KIO::StoredTransferJob *job = static_cast<KIO::StoredTransferJob *>(_job);
    if (job->error()) {
        Q_EMIT error(this);
        return;
    }
    QByteArray data = job->data();
    potdProviderData()->wallpaperImage = QImage::fromData(data);
    Q_EMIT finished(this);
}

K_PLUGIN_CLASS_WITH_JSON(BingProvider, "bingprovider.json")

#include "bingprovider.moc"
