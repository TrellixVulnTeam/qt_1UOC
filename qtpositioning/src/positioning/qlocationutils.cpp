// Copyright (C) 2016 Jolla Ltd.
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qlocationutils_p.h"
#include "qgeopositioninfo.h"
#include "qgeosatelliteinfo.h"

#include <QTime>
#include <QList>
#include <QByteArray>
#include <QDebug>

#include <math.h>

QT_BEGIN_NAMESPACE

// converts e.g. 15306.0235 from NMEA sentence to 153.100392
static double qlocationutils_nmeaDegreesToDecimal(double nmeaDegrees)
{
    double deg;
    double min = 100.0 * modf(nmeaDegrees / 100.0, &deg);
    return deg + (min / 60.0);
}

static void qlocationutils_readGga(const char *data, int size, QGeoPositionInfo *info, double uere,
                                   bool *hasFix)
{
    QByteArray sentence(data, size);
    QList<QByteArray> parts = sentence.split(',');
    QGeoCoordinate coord;

    if (hasFix && parts.size() > 6 && !parts[6].isEmpty())
        *hasFix = parts[6].toInt() > 0;

    if (parts.size() > 1 && !parts[1].isEmpty()) {
        QTime time;
        if (QLocationUtils::getNmeaTime(parts[1], &time))
            info->setTimestamp(QDateTime(QDate(), time, Qt::UTC));
    }

    if (parts.size() > 5 && parts[3].size() == 1 && parts[5].size() == 1) {
        double lat;
        double lng;
        if (QLocationUtils::getNmeaLatLong(parts[2], parts[3][0], parts[4], parts[5][0], &lat, &lng)) {
            coord.setLatitude(lat);
            coord.setLongitude(lng);
        }
    }

    if (parts.size() > 8 && !parts[8].isEmpty()) {
        bool hasHdop = false;
        double hdop = parts[8].toDouble(&hasHdop);
        if (hasHdop)
            info->setAttribute(QGeoPositionInfo::HorizontalAccuracy, 2 * hdop * uere);
    }

    if (parts.size() > 9 && !parts[9].isEmpty()) {
        bool hasAlt = false;
        double alt = parts[9].toDouble(&hasAlt);
        if (hasAlt)
            coord.setAltitude(alt);
    }

    if (coord.type() != QGeoCoordinate::InvalidCoordinate)
        info->setCoordinate(coord);
}

static void qlocationutils_readGsa(const char *data, int size, QGeoPositionInfo *info, double uere,
                                   bool *hasFix)
{
    QList<QByteArray> parts = QByteArray::fromRawData(data, size).split(',');

    if (hasFix && parts.size() > 2 && !parts[2].isEmpty())
        *hasFix = parts[2].toInt() > 0;

    if (parts.size() > 16 && !parts[16].isEmpty()) {
        bool hasHdop = false;
        double hdop = parts[16].toDouble(&hasHdop);
        if (hasHdop)
            info->setAttribute(QGeoPositionInfo::HorizontalAccuracy, 2 * hdop * uere);
    }

    if (parts.size() > 17 && !parts[17].isEmpty()) {
        bool hasVdop = false;
        double vdop = parts[17].toDouble(&hasVdop);
        if (hasVdop)
            info->setAttribute(QGeoPositionInfo::VerticalAccuracy, 2 * vdop * uere);
    }
}

static void qlocationutils_readGsa(const char *data,
                                              int size,
                                              QList<int> &pnrsInUse)
{
    QList<QByteArray> parts = QByteArray::fromRawData(data, size).split(',');
    pnrsInUse.clear();
    if (parts.size() <= 2)
        return;
    bool ok;
    for (qsizetype i = 3; i < qMin(15, parts.size()); ++i) {
        const QByteArray &pnrString = parts.at(i);
        if (pnrString.isEmpty())
            continue;
        int pnr = pnrString.toInt(&ok);
        if (ok)
            pnrsInUse.append(pnr);
    }
}

static void qlocationutils_readGll(const char *data, int size, QGeoPositionInfo *info, bool *hasFix)
{
    QByteArray sentence(data, size);
    QList<QByteArray> parts = sentence.split(',');
    QGeoCoordinate coord;

    if (hasFix && parts.size() > 6 && !parts[6].isEmpty())
        *hasFix = (parts[6][0] == 'A');

    if (parts.size() > 5 && !parts[5].isEmpty()) {
        QTime time;
        if (QLocationUtils::getNmeaTime(parts[5], &time))
            info->setTimestamp(QDateTime(QDate(), time, Qt::UTC));
    }

    if (parts.size() > 4 && parts[2].size() == 1 && parts[4].size() == 1) {
        double lat;
        double lng;
        if (QLocationUtils::getNmeaLatLong(parts[1], parts[2][0], parts[3], parts[4][0], &lat, &lng)) {
            coord.setLatitude(lat);
            coord.setLongitude(lng);
        }
    }

    if (coord.type() != QGeoCoordinate::InvalidCoordinate)
        info->setCoordinate(coord);
}

static void qlocationutils_readRmc(const char *data, int size, QGeoPositionInfo *info, bool *hasFix)
{
    QByteArray sentence(data, size);
    QList<QByteArray> parts = sentence.split(',');
    QGeoCoordinate coord;
    QDate date;
    QTime time;

    if (hasFix && parts.size() > 2 && !parts[2].isEmpty())
        *hasFix = (parts[2][0] == 'A');

    if (parts.size() > 9 && parts[9].size() == 6) {
        date = QDate::fromString(QString::fromLatin1(parts[9]), QStringLiteral("ddMMyy"));
        if (date.isValid())
            date = date.addYears(100);     // otherwise starts from 1900
        else
            date = QDate();
    }

    if (parts.size() > 1 && !parts[1].isEmpty())
        QLocationUtils::getNmeaTime(parts[1], &time);

    if (parts.size() > 6 && parts[4].size() == 1 && parts[6].size() == 1) {
        double lat;
        double lng;
        if (QLocationUtils::getNmeaLatLong(parts[3], parts[4][0], parts[5], parts[6][0], &lat, &lng)) {
            coord.setLatitude(lat);
            coord.setLongitude(lng);
        }
    }

    bool parsed = false;
    double value = 0.0;
    if (parts.size() > 7 && !parts[7].isEmpty()) {
        value = parts[7].toDouble(&parsed);
        if (parsed)
            info->setAttribute(QGeoPositionInfo::GroundSpeed, qreal(value * 1.852 / 3.6));    // knots -> m/s
    }
    if (parts.size() > 8 && !parts[8].isEmpty()) {
        value = parts[8].toDouble(&parsed);
        if (parsed)
            info->setAttribute(QGeoPositionInfo::Direction, qreal(value));
    }
    if (parts.size() > 11 && parts[11].size() == 1
            && (parts[11][0] == 'E' || parts[11][0] == 'W')) {
        value = parts[10].toDouble(&parsed);
        if (parsed) {
            if (parts[11][0] == 'W')
                value *= -1;
            info->setAttribute(QGeoPositionInfo::MagneticVariation, qreal(value));
        }
    }

    if (coord.type() != QGeoCoordinate::InvalidCoordinate)
        info->setCoordinate(coord);

    info->setTimestamp(QDateTime(date, time, Qt::UTC));
}

static void qlocationutils_readVtg(const char *data, int size, QGeoPositionInfo *info, bool *hasFix)
{
    if (hasFix)
        *hasFix = false;

    QByteArray sentence(data, size);
    QList<QByteArray> parts = sentence.split(',');

    bool parsed = false;
    double value = 0.0;
    if (parts.size() > 1 && !parts[1].isEmpty()) {
        value = parts[1].toDouble(&parsed);
        if (parsed)
            info->setAttribute(QGeoPositionInfo::Direction, qreal(value));
    }
    if (parts.size() > 7 && !parts[7].isEmpty()) {
        value = parts[7].toDouble(&parsed);
        if (parsed)
            info->setAttribute(QGeoPositionInfo::GroundSpeed, qreal(value / 3.6));    // km/h -> m/s
    }
}

static void qlocationutils_readZda(const char *data, int size, QGeoPositionInfo *info, bool *hasFix)
{
    if (hasFix)
        *hasFix = false;

    QByteArray sentence(data, size);
    QList<QByteArray> parts = sentence.split(',');
    QDate date;
    QTime time;

    if (parts.size() > 1 && !parts[1].isEmpty())
        QLocationUtils::getNmeaTime(parts[1], &time);

    if (parts.size() > 4 && !parts[2].isEmpty() && !parts[3].isEmpty()
            && parts[4].size() == 4) {     // must be full 4-digit year
        int day = parts[2].toInt();
        int month = parts[3].toInt();
        int year = parts[4].toInt();
        if (day > 0 && month > 0 && year > 0)
            date.setDate(year, month, day);
    }

    info->setTimestamp(QDateTime(date, time, Qt::UTC));
}

QLocationUtils::NmeaSentence QLocationUtils::getNmeaSentenceType(const char *data, int size)
{
    if (size < 6 || data[0] != '$' || !hasValidNmeaChecksum(data, size))
        return NmeaSentenceInvalid;

    if (data[3] == 'G' && data[4] == 'G' && data[5] == 'A')
        return NmeaSentenceGGA;

    if (data[3] == 'G' && data[4] == 'S' && data[5] == 'A')
        return NmeaSentenceGSA;

    if (data[3] == 'G' && data[4] == 'S' && data[5] == 'V')
        return NmeaSentenceGSV;

    if (data[3] == 'G' && data[4] == 'L' && data[5] == 'L')
        return NmeaSentenceGLL;

    if (data[3] == 'R' && data[4] == 'M' && data[5] == 'C')
        return NmeaSentenceRMC;

    if (data[3] == 'V' && data[4] == 'T' && data[5] == 'G')
        return NmeaSentenceVTG;

    if (data[3] == 'Z' && data[4] == 'D' && data[5] == 'A')
        return NmeaSentenceZDA;

    return NmeaSentenceInvalid;
}

QGeoSatelliteInfo::SatelliteSystem QLocationUtils::getSatelliteSystem(const char *data, int size)
{
    if (size < 6 || data[0] != '$' || !hasValidNmeaChecksum(data, size))
        return QGeoSatelliteInfo::Undefined;

    // GPS: GP
    if (data[1] == 'G' && data[2] == 'P')
        return QGeoSatelliteInfo::GPS;

    // GLONASS: GL
    if (data[1] == 'G' && data[2] == 'L')
        return QGeoSatelliteInfo::GLONASS;

    // GALILEO: GA
    if (data[1] == 'G' && data[2] == 'A')
        return QGeoSatelliteInfo::GALILEO;

    // BeiDou: BD or GB
    if ((data[1] == 'B' && data[2] == 'D') || (data[1] == 'G' && data[2] == 'B'))
        return QGeoSatelliteInfo::BEIDOU;

    // QZSS: GQ, PQ, QZ
    if ((data[1] == 'G' && data[2] == 'Q') || (data[1] == 'P' && data[2] == 'Q')
        || (data[1] == 'Q' && data[2] == 'Z')) {
        return QGeoSatelliteInfo::QZSS;
    }

    // Multiple: GN
    if (data[1] == 'G' && data[2] == 'N')
        return QGeoSatelliteInfo::Multiple;

    return QGeoSatelliteInfo::Undefined;
}

QGeoSatelliteInfo::SatelliteSystem QLocationUtils::getSatelliteSystemBySatelliteId(int satId)
{
    if (satId >= 1 && satId <= 32)
        return QGeoSatelliteInfo::GPS;

    if (satId >= 65 && satId <= 96) // including future extensions
        return QGeoSatelliteInfo::GLONASS;

    if (satId >= 193 && satId <= 200) // including future extensions
        return QGeoSatelliteInfo::QZSS;

    if ((satId >= 201 && satId <= 235) || (satId >= 401 && satId <= 437))
        return QGeoSatelliteInfo::BEIDOU;

    if (satId >= 301 && satId <= 336)
        return QGeoSatelliteInfo::GALILEO;

    return QGeoSatelliteInfo::Undefined;
}

bool QLocationUtils::getPosInfoFromNmea(const char *data, int size, QGeoPositionInfo *info,
                                        double uere, bool *hasFix)
{
    if (!info)
        return false;

    if (hasFix)
        *hasFix = false;

    NmeaSentence nmeaType = getNmeaSentenceType(data, size);
    if (nmeaType == NmeaSentenceInvalid)
        return false;

    // Adjust size so that * and following characters are not parsed by the following functions.
    for (int i = 0; i < size; ++i) {
        if (data[i] == '*') {
            size = i;
            break;
        }
    }

    switch (nmeaType) {
    case NmeaSentenceGGA:
        qlocationutils_readGga(data, size, info, uere, hasFix);
        return true;
    case NmeaSentenceGSA:
        qlocationutils_readGsa(data, size, info, uere, hasFix);
        return true;
    case NmeaSentenceGLL:
        qlocationutils_readGll(data, size, info, hasFix);
        return true;
    case NmeaSentenceRMC:
        qlocationutils_readRmc(data, size, info, hasFix);
        return true;
    case NmeaSentenceVTG:
        qlocationutils_readVtg(data, size, info, hasFix);
        return true;
    case NmeaSentenceZDA:
        qlocationutils_readZda(data, size, info, hasFix);
        return true;
    default:
        return false;
    }
}

QNmeaSatelliteInfoSource::SatelliteInfoParseStatus
QLocationUtils::getSatInfoFromNmea(const char *data, int size, QList<QGeoSatelliteInfo> &infos, QGeoSatelliteInfo::SatelliteSystem &system)
{
    if (!data || !size)
        return QNmeaSatelliteInfoSource::NotParsed;

    NmeaSentence nmeaType = getNmeaSentenceType(data, size);
    if (nmeaType != NmeaSentenceGSV)
        return QNmeaSatelliteInfoSource::NotParsed;

    // Standard forbids using $GN talker id for GSV messages, so the system
    // type here will be uniquely identified.
    system = getSatelliteSystem(data, size);

    // Adjust size so that * and following characters are not parsed by the
    // following code.
    for (int i = 0; i < size; ++i) {
        if (data[i] == '*') {
            size = i;
            break;
        }
    }

    QList<QByteArray> parts = QByteArray::fromRawData(data, size).split(',');

    if (parts.size() <= 3) {
        infos.clear();
        return QNmeaSatelliteInfoSource::FullyParsed; // Malformed sentence.
    }
    bool ok;
    const int totalSentences = parts.at(1).toInt(&ok);
    if (!ok) {
        infos.clear();
        return QNmeaSatelliteInfoSource::FullyParsed; // Malformed sentence.
    }

    const int sentence = parts.at(2).toInt(&ok);
    if (!ok) {
        infos.clear();
        return QNmeaSatelliteInfoSource::FullyParsed; // Malformed sentence.
    }

    const int totalSats = parts.at(3).toInt(&ok);
    if (!ok) {
        infos.clear();
        return QNmeaSatelliteInfoSource::FullyParsed; // Malformed sentence.
    }

    if (sentence == 1)
        infos.clear();

    const int numSatInSentence = qMin(sentence * 4, totalSats) - (sentence - 1) * 4;
    if (parts.size() < (4 + numSatInSentence * 4)) {
        infos.clear();
        return QNmeaSatelliteInfoSource::FullyParsed; // Malformed sentence.
    }

    int field = 4;
    for (int i = 0; i < numSatInSentence; ++i) {
        QGeoSatelliteInfo info;
        info.setSatelliteSystem(system);
        int prn = parts.at(field++).toInt(&ok);
        // Quote from: https://gpsd.gitlab.io/gpsd/NMEA.html#_satellite_ids
        // GLONASS satellite numbers come in two flavors. If a sentence has a GL
        // talker ID, expect the skyviews to be GLONASS-only and in the range
        // 1-32; you must add 64 to get a globally-unique NMEA ID. If the
        // sentence has a GN talker ID, the device emits a multi-constellation
        // skyview with GLONASS IDs already in the 65-96 range.
        //
        // However I don't observe such behavior with my device. So implementing
        // a safe scenario.
        if (ok && (system == QGeoSatelliteInfo::GLONASS)) {
            if (prn <= 64)
                prn += 64;
        }
        info.setSatelliteIdentifier((ok) ? prn : 0);
        const int elevation = parts.at(field++).toInt(&ok);
        info.setAttribute(QGeoSatelliteInfo::Elevation, (ok) ? elevation : 0);
        const int azimuth = parts.at(field++).toInt(&ok);
        info.setAttribute(QGeoSatelliteInfo::Azimuth, (ok) ? azimuth : 0);
        const int snr = parts.at(field++).toInt(&ok);
        info.setSignalStrength((ok) ? snr : -1);
        infos.append(info);
    }

    if (sentence == totalSentences)
        return QNmeaSatelliteInfoSource::FullyParsed;

    return QNmeaSatelliteInfoSource::PartiallyParsed;
}

QGeoSatelliteInfo::SatelliteSystem QLocationUtils::getSatInUseFromNmea(const char *data, int size,
                                                                       QList<int> &pnrsInUse)
{
    if (!data || !size)
        return QGeoSatelliteInfo::Undefined;

    NmeaSentence nmeaType = getNmeaSentenceType(data, size);
    if (nmeaType != NmeaSentenceGSA)
        return QGeoSatelliteInfo::Undefined;

    auto systemType = getSatelliteSystem(data, size);
    if (systemType == QGeoSatelliteInfo::Undefined)
        return systemType;

    // The documentation states that we do not modify pnrsInUse if we could not
    // parse the data
    pnrsInUse.clear();

    // Adjust size so that * and following characters are not parsed by the following functions.
    for (int i = 0; i < size; ++i) {
        if (data[i] == '*') {
            size = i;
            break;
        }
    }
    qlocationutils_readGsa(data, size, pnrsInUse);

    // Quote from: https://gpsd.gitlab.io/gpsd/NMEA.html#_satellite_ids
    // GLONASS satellite numbers come in two flavors. If a sentence has a GL
    // talker ID, expect the skyviews to be GLONASS-only and in the range 1-32;
    // you must add 64 to get a globally-unique NMEA ID. If the sentence has a
    // GN talker ID, the device emits a multi-constellation skyview with
    // GLONASS IDs already in the 65-96 range.
    //
    // However I don't observe such behavior with my device. So implementing a
    // safe scenario.
    if (systemType == QGeoSatelliteInfo::GLONASS) {
        std::for_each(pnrsInUse.begin(), pnrsInUse.end(), [](int &id) {
            if (id <= 64)
                id += 64;
        });
    }

    if ((systemType == QGeoSatelliteInfo::Multiple) && !pnrsInUse.isEmpty()) {
        // Standard claims that in case of multiple system types we will receive
        // several GSA messages, each containing data from only one satellite
        // system, so we can pick the first id to determine the system type.
        auto tempSystemType = getSatelliteSystemBySatelliteId(pnrsInUse.front());
        if (tempSystemType != QGeoSatelliteInfo::Undefined)
            systemType = tempSystemType;
    }

    return systemType;
}

bool QLocationUtils::hasValidNmeaChecksum(const char *data, int size)
{
    int asteriskIndex = -1;
    for (int i = 0; i < size; ++i) {
        if (data[i] == '*') {
            asteriskIndex = i;
            break;
        }
    }

    const int CSUM_LEN = 2;
    if (asteriskIndex < 0 || asteriskIndex + CSUM_LEN >= size)
        return false;

    // XOR byte value of all characters between '$' and '*'
    int result = 0;
    for (int i = 1; i < asteriskIndex; ++i)
        result ^= data[i];
    /*
        char calc[CSUM_LEN + 1];
        ::snprintf(calc, CSUM_LEN + 1, "%02x", result);
        return ::strncmp(calc, &data[asteriskIndex+1], 2) == 0;
        */

    QByteArray checkSumBytes(&data[asteriskIndex + 1], 2);
    bool ok = false;
    int checksum = checkSumBytes.toInt(&ok,16);
    return ok && checksum == result;
}

bool QLocationUtils::getNmeaTime(const QByteArray &bytes, QTime *time)
{
    int dotIndex = bytes.indexOf('.');
    QTime tempTime;

    if (dotIndex < 0) {
        tempTime = QTime::fromString(QString::fromLatin1(bytes.constData()),
                                     QStringLiteral("hhmmss"));
    } else {
        tempTime = QTime::fromString(QString::fromLatin1(bytes.mid(0, dotIndex)),
                                     QStringLiteral("hhmmss"));
        bool hasMsecs = false;
        int midLen = qMin(3, bytes.size() - dotIndex - 1);
        int msecs = bytes.mid(dotIndex + 1, midLen).toUInt(&hasMsecs);
        if (hasMsecs)
            tempTime = tempTime.addMSecs(msecs*(midLen == 3 ? 1 : midLen == 2 ? 10 : 100));
    }

    if (tempTime.isValid()) {
        *time = tempTime;
        return true;
    }
    return false;
}

bool QLocationUtils::getNmeaLatLong(const QByteArray &latString, char latDirection, const QByteArray &lngString, char lngDirection, double *lat, double *lng)
{
    if ((latDirection != 'N' && latDirection != 'S')
            || (lngDirection != 'E' && lngDirection != 'W')) {
        return false;
    }

    bool hasLat = false;
    bool hasLong = false;
    double tempLat = latString.toDouble(&hasLat);
    double tempLng = lngString.toDouble(&hasLong);
    if (hasLat && hasLong) {
        tempLat = qlocationutils_nmeaDegreesToDecimal(tempLat);
        if (latDirection == 'S')
            tempLat *= -1;
        tempLng = qlocationutils_nmeaDegreesToDecimal(tempLng);
        if (lngDirection == 'W')
            tempLng *= -1;

        if (isValidLat(tempLat) && isValidLong(tempLng)) {
            *lat = tempLat;
            *lng = tempLng;
            return true;
        }
    }
    return false;
}

QT_END_NAMESPACE

