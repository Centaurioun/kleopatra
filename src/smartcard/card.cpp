/*  smartcard/card.h

    This file is part of Kleopatra, the KDE keymanager
    SPDX-FileCopyrightText: 2017 Bundesamt für Sicherheit in der Informationstechnik
    SPDX-FileContributor: Intevation GmbH

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "card.h"

#include "readerstatus.h"

using namespace Kleo;
using namespace Kleo::SmartCard;

namespace {
static QString formatVersion(int value)
{
    if (value < 0) {
        return QString();
    }

    const unsigned int a = ((value >> 24) & 0xff);
    const unsigned int b = ((value >> 16) & 0xff);
    const unsigned int c = ((value >>  8) & 0xff);
    const unsigned int d = ((value      ) & 0xff);
    if (a) {
        return QStringLiteral("%1.%2.%3.%4").arg(QString::number(a), QString::number(b), QString::number(c), QString::number(d));
    } else if (b) {
        return QStringLiteral("%1.%2.%3").arg(QString::number(b), QString::number(c), QString::number(d));
    } else if (c) {
        return QStringLiteral("%1.%2").arg(QString::number(c), QString::number(d));
    }
    return QString::number(d);
}
}

Card::Card()
{
}

Card::~Card()
{
}

void Card::setStatus(Status s)
{
    mStatus = s;
}

Card::Status Card::status() const
{
    return mStatus;
}

void Card::setSerialNumber(const std::string &sn)
{
    mSerialNumber = sn;
}

std::string Card::serialNumber() const
{
    return mSerialNumber;
}

QString Card::displaySerialNumber() const
{
    return mDisplaySerialNumber;
}

void Card::setDisplaySerialNumber(const QString &serialNumber)
{
    mDisplaySerialNumber = serialNumber;
}

std::string Card::appName() const
{
    return mAppName;
}

void Card::setAppName(const std::string &name)
{
    mAppName = name;
}

void Card::setAppVersion(int version)
{
    mAppVersion = version;
}

int Card::appVersion() const
{
    return mAppVersion;
}

QString Card::displayAppVersion() const
{
    return formatVersion(mAppVersion);
}

std::string Card::cardType() const
{
    return mCardType;
}

int Card::cardVersion() const
{
    return mCardVersion;
}

QString Card::displayCardVersion() const
{
    return formatVersion(mCardVersion);
}

QString Card::cardHolder() const
{
    return mCardHolder;
}

std::vector<Card::PinState> Card::pinStates() const
{
    return mPinStates;
}

void Card::setPinStates(const std::vector<PinState> &pinStates)
{
    mPinStates = pinStates;
}

bool Card::hasNullPin() const
{
    return mHasNullPin;
}

void Card::setHasNullPin(bool value)
{
    mHasNullPin = value;
}

bool Card::canLearnKeys() const
{
    return mCanLearn;
}

void Card::setCanLearnKeys(bool value)
{
    mCanLearn = value;
}

bool Card::operator == (const Card &other) const
{
    return mStatus == other.status()
        && mSerialNumber == other.serialNumber()
        && mAppName == other.appName()
        && mAppVersion == other.appVersion()
        && mPinStates == other.pinStates()
        && mCanLearn == other.canLearnKeys()
        && mHasNullPin == other.hasNullPin();
}

bool Card::operator != (const Card &other) const
{
    return !operator==(other);
}

void Card::setErrorMsg(const QString &msg)
{
    mErrMsg = msg;
}

QString Card::errorMsg() const
{
    return mErrMsg;
}

namespace {
static int parseHexEncodedVersionTuple(const std::string &s) {
    // s is a hex-encoded, unsigned int-packed version tuple,
    // i.e. each byte represents one part of the version tuple
    bool ok;
    const auto version = QByteArray::fromStdString(s).toUInt(&ok, 16);
    return ok ? version : -1;
}
}

bool Card::parseCardInfo(const std::string &name, const std::string &value)
{
    if (name == "APPVERSION") {
        mAppVersion = parseHexEncodedVersionTuple(value);
        return true;
    } else if (name == "CARDTYPE") {
        mCardType = value;
        return true;
    } else if (name == "CARDVERSION") {
        mCardVersion = parseHexEncodedVersionTuple(value);
        return true;
    } else if (name == "DISP-NAME") {
        auto list = QString::fromUtf8(QByteArray::fromStdString(value)).
                    split(QStringLiteral("<<"), Qt::SkipEmptyParts);
        std::reverse(list.begin(), list.end());
        mCardHolder = list.join(QLatin1Char(' '));
        return true;
    }

    return false;
}
