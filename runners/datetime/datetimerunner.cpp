/*
 *   SPDX-FileCopyrightText: 2006 Aaron Seigo <aseigo@kde.org>
 *   SPDX-FileCopyrightText: 2010 Marco Martin <notmart@gmail.com>
 *   SPDX-FileCopyrightText: 2015 Vishesh Handa <vhanda@kde.org>
 *   SPDX-FileCopyrightText: 2022 Natalie Clarius <natalie_clarius@yahoo.de>
 *
 *   SPDX-License-Identifier: LGPL-2.0-only
 */

#include "datetimerunner.h"

#include <KFormat>
#include <KLocalizedString>

#include <QDateTime>
#include <QIcon>
#include <QLocale>
#include <QRegularExpression>
#include <QTimeZone>

#include <math.h>

static const QString dateWord = i18nc("Note this is a KRunner keyword", "date");
static const QString timeWord = i18nc("Note this is a KRunner keyword", "time");
static const QString conversionWords = i18nc(
    "words to specify a time in a time zone or to convert a time to a time zone, e.g. 'current time in Berlin' or '18:00 UTC to CET', separated by '|' (will "
    "be used as a regex)",
    "to|in|as|at");
static const QRegularExpression conversionWordsRegex = QRegularExpression(QString("\\s(%1)\\s").arg(conversionWords));

DateTimeRunner::DateTimeRunner(QObject *parent, const KPluginMetaData &metaData, const QVariantList &args)
    : AbstractRunner(parent, metaData, args)
{
    setObjectName(QLatin1String("DateTimeRunner"));

    addSyntax(RunnerSyntax(dateWord, i18n("Displays the current date")));
    addSyntax(RunnerSyntax(timeWord, i18n("Displays the current time")));
    addSyntax(RunnerSyntax(dateWord + i18nc("The <> and space are part of the example query", " <timezone>"), //
                           i18n("Displays the current date and difference to system date in a given timezone")));
    addSyntax(RunnerSyntax(timeWord + i18nc("The <> and space are part of the example query", " <timezone>"), //
                           i18n("Displays the current time and difference to system time in a given timezone")));
    addSyntax(RunnerSyntax(i18nc("The <> and space are part of the example query", "<timezone> <time> <timezone>"), //
                           i18n("Converts the time from the first timezone to the second timezone. If only one time zone is given, the other will be the "
                                "system time zone. If no date is given, it will be the current date.")));
}

DateTimeRunner::~DateTimeRunner()
{
}

void DateTimeRunner::match(RunnerContext &context)
{
    const QString term = context.query().replace(conversionWordsRegex, QStringLiteral(" ")); // strip away conversion words

    if (term.compare(dateWord, Qt::CaseInsensitive) == 0) {
        // current date in local time zone

        const QDate date = QDate::currentDate();
        const QString dateStr = QLocale().toString(date);
        addMatch(i18n("Today's date is %1", dateStr), dateStr, 1.0, QStringLiteral("view-calendar-day"), context);
    } else if (term.startsWith(dateWord + QLatin1Char(' '), Qt::CaseInsensitive)) {
        // current date in remote time zone

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        const auto zoneTerm = term.rightRef(term.length() - dateWord.length() - 1);
#else
        const auto zoneTerm = QStringView(term).right(term.length() - dateWord.length() - 1);
#endif
        const auto zones = matchingTimeZones(zoneTerm);
        for (auto it = zones.constBegin(), itEnd = zones.constEnd(); it != itEnd; ++it) {
            const QTimeZone zone = it.value();
            const QString zoneStr = it.key();
            const QDateTime datetime = QDateTime::currentDateTime().toTimeZone(zone);
            const QString dateStr = QLocale().toString(datetime.date());

            const qint64 dateDiff = QDateTime::currentDateTime().daysTo(QDateTime(datetime.date(), datetime.time())) * (24 * 60 * 60 * 1000); // full days in ms
            const QString dateDiffNumStr = KFormat().formatSpelloutDuration(abs(dateDiff));
            const QString dateDiffLaterStr =
                i18nc("time difference between time zones, e.g. in Stockholm it's 1 calendar day later than in Brasilia", "%1 later", dateDiffNumStr);
            const QString dateDiffEarlierStr =
                i18nc("date difference between time zones, e.g. in Brasilia it's 1 calendar day earlier than in Stockholm", "%1 earlier", dateDiffNumStr);
            const QString dateDiffSameStr =
                i18nc("no date difference between time zones, e.g. in Stockholm it's the same calendar day as in Berlin", "no date difference");
            const QString dateDiffStr = dateDiff > 0 ? dateDiffLaterStr : dateDiff < 0 ? dateDiffEarlierStr : dateDiffSameStr;

            addMatch(QStringLiteral("%1: %2 (%3)").arg(zoneStr, dateStr, dateDiffStr),
                     dateStr,
                     ((qreal)(zoneStr.count(zoneTerm, Qt::CaseInsensitive)) * zoneTerm.length() - (qreal)zoneStr.indexOf(zoneTerm, Qt::CaseInsensitive))
                         / zoneStr.length(),
                     QStringLiteral("view-calendar-day"),
                     context);
        }
    } else if (term.compare(timeWord, Qt::CaseInsensitive) == 0) {
        // current time in local time zone

        const QTime time = QTime::currentTime();
        const QString timeStr = QLocale().toString(time);
        addMatch(i18n("Current time is %1", timeStr), timeStr, 1.0, QStringLiteral("clock"), context);
    } else if (term.startsWith(timeWord + QLatin1Char(' '), Qt::CaseInsensitive)) {
        // current time in remote time zone

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        const auto zoneTerm = term.rightRef(term.length() - timeWord.length() - 1);
#else
        const auto zoneTerm = QStringView(term).right(term.length() - timeWord.length() - 1);
#endif
        const auto zones = matchingTimeZones(zoneTerm);
        for (auto it = zones.constBegin(), itEnd = zones.constEnd(); it != itEnd; ++it) {
            const QTimeZone zone = it.value();
            const QString zoneStr = it.key();
            const QDateTime datetime = QDateTime::currentDateTime().toTimeZone(zone);
            const QString timeStr = QLocale().toString(datetime.time(), QLocale::ShortFormat);

            const qint64 dateDiff = QDateTime::currentDateTime().daysTo(QDateTime(datetime.date(), datetime.time())) * (24 * 60 * 60 * 1000); // full days in ms
            const QString dayDiffNumStr = KFormat().formatSpelloutDuration(abs(dateDiff));
            const QString timeDayLaterStr = i18nc(
                "time difference with calendar date difference between time zones, e.g. 22:00 Brasilia time in Stockholm = "
                "02:00 + 1 day, where %1 is the time and %2 is "
                "the days later",
                "%1 + %2",
                timeStr,
                dayDiffNumStr);
            const QString timeDayEarlierStr = i18nc(
                "time difference with calendar date difference between time zones, e.g. 02:00 Stockholm time in Brasilia "
                "= 22:00 - 1 day, where %1 is the time and %2 is "
                "the days earlier",
                "%1 - %2",
                timeStr,
                dayDiffNumStr);
            const QString timeDayStr = dateDiff > 0 ? timeDayLaterStr : dateDiff < 0 ? timeDayEarlierStr : timeStr;

            const qint64 timeDiff = round((double)QDateTime::currentDateTime().secsTo(QDateTime(datetime.date(), datetime.time())) / 60)
                * (60 * 1000); // time in ms rounded to the nearest full
            const QString timeDiffNumStr = KFormat().formatSpelloutDuration(abs(timeDiff));
            const QString timeDiffLaterStr =
                i18nc("time difference between time zones, e.g. in Stockholm it's 4 hours later than in Brasilia", "%1 later", timeDiffNumStr);
            const QString timeDiffEarlierStr =
                i18nc("time difference between time zones, e.g. in Brasilia it's 4 hours earlier than in Stockholm", "%1 earlier", timeDiffNumStr);
            const QString timeDiffSameStr =
                i18nc("no time difference between time zones, e.g. in Stockholm it's the same time as in Berlin", "no time difference");
            const QString timeDiffStr = timeDiff > 0 ? timeDiffLaterStr : timeDiff < 0 ? timeDiffEarlierStr : timeDiffSameStr;

            addMatch(QStringLiteral("%1: %2 (%3)").arg(zoneStr, timeDayStr, timeDiffStr),
                     timeStr,
                     ((qreal)(zoneStr.count(zoneTerm, Qt::CaseInsensitive)) * zoneTerm.length() - (qreal)zoneStr.indexOf(zoneTerm, Qt::CaseInsensitive))
                         / zoneStr.length(),
                     QStringLiteral("clock"),
                     context);
        }
    } else {
        // convert user-given time between user-given timezones

        const int minLen = QLocale().timeFormat(QLocale::ShortFormat).length();
        if (term.length() < minLen) {
            return; // query is too short to contain a time specification: not a match
        } else if (std::none_of(term.constBegin(), term.constEnd(), [](QChar c) {
                       return c.isDigit();
                   })) {
            return; // query does not contain any digits: not a match
        }
        QDate date;
        QTime time;
        QString dtTerm;
        // check all query substrings starting after whitespace and long enough to contain a time specification
        for (int i = 0; i + minLen < term.length() + 1 && time.isNull(); ++i) {
            if (!(i == 0 || term.mid(i - 1, 1).compare(QStringLiteral(" ")) == 0))
                continue;
            for (int n = minLen; i + n < term.length() + 1 && time.isNull(); ++n) {
                dtTerm = term.mid(i, n);
                // try to parse substring as datetime or time
                if (QDateTime dateTimeParse = QLocale().toDateTime(dtTerm, QLocale::ShortFormat); dateTimeParse.isValid()) {
                    date = dateTimeParse.date();
                    time = dateTimeParse.time();
                } else if (QTime timeParse = QLocale().toTime(dtTerm, QLocale::ShortFormat); timeParse.isValid()) {
                    time = timeParse;
                    // unspecified date will later be initialized to current date in from time zone
                }
            }
        }
        if (time.isNull()) {
            return; // no valid time specification in the query: not a match
        }

        // time zone to convert from: left of time spec, otherwise default to current time zone
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QStringRef fromZoneTerm = term.leftRef(term.indexOf(dtTerm)).trimmed();
#else
        QStringView fromZoneTerm = QStringView(term).left(term.indexOf(dtTerm)).trimmed();
#endif
        QHash<QString, QTimeZone> fromZones = matchingTimeZones(fromZoneTerm, QDateTime(date, time));

        // time zone to convert to: right of time spec, otherwise default to current time zone
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QStringRef toZoneTerm = term.rightRef(term.length() - dtTerm.length() - term.indexOf(dtTerm)).trimmed();
#else
        QStringView toZoneTerm = QStringView(term).right(term.length() - dtTerm.length() - term.indexOf(dtTerm)).trimmed();
#endif
        QHash<QString, QTimeZone> toZones = matchingTimeZones(toZoneTerm, QDateTime(date, time));

        if (fromZoneTerm.isEmpty() && toZoneTerm.isEmpty()) {
            return;
        }

        for (auto it = fromZones.constBegin(), itEnd = fromZones.constEnd(); it != itEnd; ++it) {
            const QTimeZone fromZone = it.value();
            const QString fromZoneStr = it.key();
            const QDateTime fromDatetime = QDateTime(date.isValid() ? date : QDateTime::currentDateTimeUtc().toTimeZone(fromZone).date(), time, fromZone);
            const QString fromTimeStr = QLocale().toString(fromDatetime.time(), QLocale::ShortFormat);

            for (auto jt = toZones.constBegin(), itEnd = toZones.constEnd(); jt != itEnd; ++jt) {
                const QTimeZone toZone = jt.value();
                const QString toZoneStr = jt.key();
                const QDateTime toDatetime = fromDatetime.toTimeZone(toZone);
                const QString toTimeStr = QLocale().toString(toDatetime.time(), QLocale::ShortFormat);

                const qint64 dateDiff = QDateTime(fromDatetime.date(), fromDatetime.time()).daysTo(QDateTime(toDatetime.date(), toDatetime.time()))
                    * (24 * 60 * 60 * 1000); // full days in ms
                const QString dayDiffNumStr = KFormat().formatSpelloutDuration(abs(dateDiff));
                const QString toTimeDayLaterStr = i18nc(
                    "time difference with calendar date difference between time zones, e.g. 22:00 Brasilia time in Stockholm = "
                    "02:00 + 1 day, where %1 is the time and %2 is "
                    "the days later",
                    "%1 + %2",
                    toTimeStr,
                    dayDiffNumStr);
                const QString toTimeDayEarlierStr = i18nc(
                    "time difference with calendar date difference between time zones, e.g. 02:00 Stockholm time in Brasilia "
                    "= 22:00 - 1 day, where %1 is the time and %2 is "
                    "the days earlier",
                    "%1 - %2",
                    toTimeStr,
                    dayDiffNumStr);
                const QString toTimeDayStr = dateDiff > 0 ? toTimeDayLaterStr : dateDiff < 0 ? toTimeDayEarlierStr : toTimeStr;

                const qint64 timeDiff =
                    round((double)QDateTime(fromDatetime.date(), fromDatetime.time()).secsTo(QDateTime(toDatetime.date(), toDatetime.time())) / 60)
                    * (60 * 1000); // time in ms rounded to the nearest full minutes
                const QString timeDiffNumStr = KFormat().formatSpelloutDuration(abs(timeDiff));
                const QString timeDiffLaterStr =
                    i18nc("time difference between time zones, e.g. in Stockholm it's 4 hours later than in Brasilia", "%1 later", timeDiffNumStr);
                const QString timeDiffEarlierStr =
                    i18nc("time difference between time zones, e.g. in Brasilia it's 4 hours earlier than in Stockholm", "%1 earlier", timeDiffNumStr);
                const QString timeDiffSameStr =
                    i18nc("no time difference between time zones, e.g. in Stockholm it's the same time as in Berlin", "no time difference");
                const QString timeDiffStr = timeDiff > 0 ? timeDiffLaterStr : timeDiff < 0 ? timeDiffEarlierStr : timeDiffSameStr;

                const qreal toZoneRelevance = ((qreal)(toZoneStr.count(toZoneTerm, Qt::CaseInsensitive)) * toZoneTerm.length()
                                               - (qreal)toZoneStr.indexOf(toZoneTerm, Qt::CaseInsensitive))
                    / toZoneStr.length();
                const qreal fromZoneRelevance = ((qreal)(fromZoneStr.count(fromZoneTerm, Qt::CaseInsensitive)) * fromZoneTerm.length()
                                                 - (qreal)fromZoneStr.indexOf(fromZoneTerm, Qt::CaseInsensitive))
                    / fromZoneStr.length();
                const qreal relevance = toZoneRelevance / 2 + fromZoneRelevance / 2;

                addMatch(QStringLiteral("%1: %2 (%3)<br>%4: %5").arg(toZoneStr, toTimeDayStr, timeDiffStr, fromZoneStr, fromTimeStr),
                         toTimeStr,
                         relevance,
                         QStringLiteral("clock"),
                         context);
            }
        }
    }
}

void DateTimeRunner::run(const RunnerContext &context, const QueryMatch &match)
{
    const QString clipboardText = match.data().toString();
    context.requestQueryStringUpdate(clipboardText, clipboardText.length());
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QHash<QString, QTimeZone> DateTimeRunner::matchingTimeZones(const QStringRef &zoneTerm, const QDateTime referenceDatetime)
#else
QHash<QString, QTimeZone> DateTimeRunner::matchingTimeZones(const QStringView &zoneTerm, const QDateTime referenceDatetime)
#endif
{
    QHash<QString, QTimeZone> ret;

    if (zoneTerm.isEmpty()) {
        const QDate atDate = referenceDatetime.date().isValid() ? referenceDatetime.date() : QDateTime::currentDateTime().date();
        const QTime atTime = referenceDatetime.time().isValid() ? referenceDatetime.time() : QDateTime::currentDateTime().time();
        const QDateTime atDatetime(atDate, atTime);
        ret[QTimeZone::systemTimeZone().abbreviation(atDatetime)] = QTimeZone::systemTimeZone();
        return ret;
    }

    const QList<QByteArray> timeZoneIds = QTimeZone::availableTimeZoneIds();
    for (const QByteArray &zoneId : timeZoneIds) {
        QTimeZone timeZone(zoneId);
        const QDate atDate = referenceDatetime.date().isValid() ? referenceDatetime.date() : QDateTime::currentDateTime().toTimeZone(timeZone).date();
        const QTime atTime = referenceDatetime.time().isValid() ? referenceDatetime.time() : QDateTime::currentDateTime().toTimeZone(timeZone).time();
        const QDateTime atDatetime(atDate, atTime);

        const QString zoneName = QString::fromUtf8(zoneId);
        if (zoneName.startsWith(QStringLiteral("UTC+")) || zoneName.startsWith(QStringLiteral("UTC-"))) {
            // Qt time zones are either of the form
            // (where {zone name} {long name} {abbreviation} {short name} {offset name} {country})
            // - "Europe/Stockholm" "Central European Standard Time" "CET" "GMT+1" "UTC+01:00" "Sweden"
            // - "UTC+01:00" "UTC+01:00" "UTC+01:00" "UTC+01:00" "UTC+01:00" "Default"
            // The latter are already covered by the offset name of the former, which we want to match exactly, so skip these
            continue;
        }

        // eg "Sweden"
        const QString country = QLocale::countryToString(timeZone.country());
        const QString comment = timeZone.comment();
        if (country.contains(zoneTerm, Qt::CaseInsensitive) || comment.contains(zoneTerm, Qt::CaseInsensitive)) {
            const QString regionName = comment.isEmpty() ? country : QLatin1String("%1 - %2").arg(country, comment);
            ret[regionName] = timeZone;
            continue;
        }

        // eg "Stockholm"
        const QString city = zoneName.mid(zoneName.indexOf(QStringLiteral("/")) + 1).replace("_", " ");
        if (city.contains(zoneTerm, Qt::CaseInsensitive)) {
            ret[city] = timeZone;
            continue;
        }

        // eg "Central European Standard Time"
        const QString longName = timeZone.displayName(atDatetime, QTimeZone::LongName);
        if (longName.contains(zoneTerm, Qt::CaseInsensitive)) {
            ret[longName] = timeZone;
            continue;
        }

        // eg "CET"
        // FIXME: This only includes the current abbreviation and not old abbreviation or other possible names.
        // Eg - depending on the current date, only CET or CEST will work
        const QString abbr = timeZone.abbreviation(atDatetime);
        if (abbr.contains(zoneTerm, Qt::CaseInsensitive)) {
            // Combine abbreviation with display name to disambiguate
            // Eg - Pacific Standard Time (PST) and Philippine Standard Time (PST)
            const QString abbrName = QString("%1 (%2)").arg(longName, abbr);
            ret[abbrName] = timeZone;
            continue;
        }

        // eg "GMT+1"
        const QString shortName = timeZone.displayName(atDatetime, QTimeZone::ShortName);
        if (shortName.compare(zoneTerm, Qt::CaseInsensitive) == 0) {
            ret[shortName] = timeZone;
            continue;
        }

        // eg "UTC+01:00"
        const QString offsetName = timeZone.displayName(atDatetime, QTimeZone::OffsetName);
        if (offsetName.compare(zoneTerm, Qt::CaseInsensitive) == 0) {
            ret[offsetName] = timeZone;
            continue;
        }
    }

    return ret;
}

void DateTimeRunner::addMatch(const QString &text, const QString &clipboardText, const qreal &relevance, const QString &iconName, RunnerContext &context)
{
    QueryMatch match(this);
    match.setText(text);
    match.setData(clipboardText);
    match.setType(QueryMatch::HelperMatch);
    match.setRelevance(relevance);
    match.setIconName(iconName);
    match.setMultiLine(true);

    context.addMatch(match);
}

K_PLUGIN_CLASS_WITH_JSON(DateTimeRunner, "plasma-runner-datetime.json")

#include "datetimerunner.moc"
