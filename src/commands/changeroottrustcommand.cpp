/* -*- mode: c++; c-basic-offset:4 -*-
    commands/changeroottrustcommand.cpp

    This file is part of Kleopatra, the KDE keymanager
    SPDX-FileCopyrightText: 2010 Klarälvdalens Datakonsult AB

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <config-kleopatra.h>

#include "changeroottrustcommand.h"
#include "command_p.h"

#include <Libkleo/KeyCache>
#include <Libkleo/Dn>
#include <Libkleo/GnuPG>

#include "kleopatra_debug.h"
#include <KLocalizedString>
#include <QSaveFile>

#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QFile>
#include <QDir>
#include <QProcess>

#include <gpgme++/key.h>


using namespace Kleo;
using namespace Kleo::Commands;
using namespace GpgME;

class ChangeRootTrustCommand::Private : public QThread, public Command::Private
{
    Q_OBJECT
private:
    friend class ::Kleo::Commands::ChangeRootTrustCommand;
    ChangeRootTrustCommand *q_func() const
    {
        return static_cast<ChangeRootTrustCommand *>(q);
    }
public:
    explicit Private(ChangeRootTrustCommand *qq, KeyListController *c)
        : QThread(), Command::Private(qq, c),
          mutex(),
          trust(Key::Ultimate),
          trustListFile(QDir(gnupgHomeDirectory()).absoluteFilePath(QStringLiteral("trustlist.txt"))),
          canceled(false)
    {

    }

private:
    void init()
    {
        q->setWarnWhenRunningAtShutdown(false);
        connect(this, &QThread::finished, this, &ChangeRootTrustCommand::Private::slotOperationFinished);
    }

    void run() override;

private:
    void slotOperationFinished()
    {
        KeyCache::mutableInstance()->enableFileSystemWatcher(true);
        if (error.isEmpty()) {
            KeyCache::mutableInstance()->reload(GpgME::CMS);
        } else
            Command::Private::error(i18n("Failed to update the trust database:\n"
                                         "%1", error),
                                    i18n("Root Trust Update Failed"));
        Command::Private::finished();
    }

private:
    mutable QMutex mutex;
    Key::OwnerTrust trust;
    QString trustListFile;
    QString gpgConfPath;
    QString error;
    volatile bool canceled;
};

ChangeRootTrustCommand::Private *ChangeRootTrustCommand::d_func()
{
    return static_cast<Private *>(d.get());
}
const ChangeRootTrustCommand::Private *ChangeRootTrustCommand::d_func() const
{
    return static_cast<const Private *>(d.get());
}

#define q q_func()
#define d d_func()

ChangeRootTrustCommand::ChangeRootTrustCommand(KeyListController *p)
    : Command(new Private(this, p))
{
    d->init();
}

ChangeRootTrustCommand::ChangeRootTrustCommand(QAbstractItemView *v, KeyListController *p)
    : Command(v, new Private(this, p))
{
    d->init();
}

ChangeRootTrustCommand::ChangeRootTrustCommand(const GpgME::Key &key, KeyListController *p)
    : Command(new Private(this, p))
{
    Q_ASSERT(!key.isNull());
    d->init();
    setKey(key);
}

ChangeRootTrustCommand::ChangeRootTrustCommand(const GpgME::Key &key, QAbstractItemView *v, KeyListController *p)
    : Command(v, new Private(this, p))
{
    Q_ASSERT(!key.isNull());
    d->init();
    setKey(key);
}

ChangeRootTrustCommand::~ChangeRootTrustCommand() {}

void ChangeRootTrustCommand::setTrust(Key::OwnerTrust trust)
{
    Q_ASSERT(!d->isRunning());
    const QMutexLocker locker(&d->mutex);
    d->trust = trust;
}

Key::OwnerTrust ChangeRootTrustCommand::trust() const
{
    const QMutexLocker locker(&d->mutex);
    return d->trust;
}

void ChangeRootTrustCommand::setTrustListFile(const QString &trustListFile)
{
    Q_ASSERT(!d->isRunning());
    const QMutexLocker locker(&d->mutex);
    d->trustListFile = trustListFile;
}

QString ChangeRootTrustCommand::trustListFile() const
{
    const QMutexLocker locker(&d->mutex);
    return d->trustListFile;
}

void ChangeRootTrustCommand::doStart()
{
    const std::vector<Key> keys = d->keys();
    Key key;
    if (keys.size() == 1) {
        key = keys.front();
    } else {
        qCWarning(KLEOPATRA_LOG) << "can only work with one certificate at a time";
    }

    if (key.isNull()) {
        d->Command::Private::finished();
        return;
    }

    d->gpgConfPath = gpgConfPath();
    KeyCache::mutableInstance()->enableFileSystemWatcher(false);
    d->start();
}

void ChangeRootTrustCommand::doCancel()
{
    const QMutexLocker locker(&d->mutex);
    d->canceled = true;
}

static QString change_trust_file(const QString &trustListFile, const QString &fingerprint, const DN &dn, Key::OwnerTrust trust);
static QString run_gpgconf_reload_gpg_agent(const QString &gpgConfPath);

void ChangeRootTrustCommand::Private::run()
{

    QMutexLocker locker(&mutex);

    const auto key = keys().front();
    const QString fpr = QString::fromLatin1(key.primaryFingerprint());
    const auto dn = DN(key.userID(0).id());
    const Key::OwnerTrust trust = this->trust;
    const QString trustListFile = this->trustListFile;
    const QString gpgConfPath   = this->gpgConfPath;

    locker.unlock();

    QString err = change_trust_file(trustListFile, fpr, dn, trust);
    if (err.isEmpty()) {
        err = run_gpgconf_reload_gpg_agent(gpgConfPath);
    }

    locker.relock();

    this->error = err;

}

static QString add_colons(const QString &fpr)
{
    QString result;
    result.reserve(fpr.size() / 2 * 3 + 1);
    bool needColon = false;
    for (QChar ch : fpr) {
        result += ch;
        if (needColon) {
            result += QLatin1Char(':');
        }
        needColon = !needColon;
    }
    if (result.endsWith(QLatin1Char(':'))) {
        result.chop(1);
    }
    return result;
}

namespace
{

// fix stupid default-finalize behaviour...
class KFixedSaveFile : public QSaveFile
{
public:
    explicit KFixedSaveFile(const QString &fileName)
        : QSaveFile(fileName) {}
    ~KFixedSaveFile() override
    {
        cancelWriting();
    }

};

}

// static
QString change_trust_file(const QString &trustListFile, const QString &key, const DN &dn, Key::OwnerTrust trust)
{
    QList<QByteArray> trustListFileContents;

    if (QFile::exists(trustListFile)) {  // non-existence is not fatal...
        if (QFile in(trustListFile); in.open(QIODevice::ReadOnly)) {
            trustListFileContents = in.readAll().split('\n');
            // remove last empty line to avoid adding more empty lines when we write the lines
            if (!trustListFileContents.empty() && trustListFileContents.back().isEmpty()) {
                trustListFileContents.pop_back();
            }
        } else { // ...but failure to open an existing file _is_
            return i18n("Cannot open existing file \"%1\" for reading: %2",
                        trustListFile, in.errorString());
        }
        // the file is now closed, so KSaveFile doesn't clobber the original
    } else {
        // the default contents of the trustlist.txt file (see the headerblurb variable in trustlist.c of gnupg);
        // we add an additional comment about the "include-default" statement
        trustListFileContents = {
            "# This is the list of trusted keys.  Comment lines, like this one, as",
            "# well as empty lines are ignored.  Lines have a length limit but this",
            "# is not a serious limitation as the format of the entries is fixed and",
            "# checked by gpg-agent.  A non-comment line starts with optional white",
            "# space, followed by the SHA-1 fingerpint in hex, followed by a flag",
            "# which may be one of 'P', 'S' or '*' and optionally followed by a list of",
            "# other flags.  The fingerprint may be prefixed with a '!' to mark the",
            "# key as not trusted.  You should give the gpg-agent a HUP or run the",
            "# command \"gpgconf --reload gpg-agent\" after changing this file.",
            "# Additionally to this file, gpg-agent will read the default trust list file",
            "# if the statement \"include-default\" is used below.",
            "",
            "",
            "# Include the default trust list",
            "include-default",
            "",
        };
    }

    KFixedSaveFile out(trustListFile);
    if (!out.open(QIODevice::WriteOnly))
        return i18n("Cannot open file \"%1\" for reading and writing: %2",
                    out.fileName() /*sic!*/, out.errorString());

    if (!out.setPermissions(QFile::ReadOwner | QFile::WriteOwner))
        return i18n("Cannot set restrictive permissions on file %1: %2",
                    out.fileName() /*sic!*/, out.errorString());

    const QString keyColon = add_colons(key);

    qCDebug(KLEOPATRA_LOG) << qPrintable(key) << " -> " << qPrintable(keyColon);

    //                                       ( 1)   (                         2                           )   (  3    )( 4)
    static const char16_t pattern[] = uR"(\s*(!?)\s*([a-fA-F0-9]{40}|(?:[a-fA-F0-9]{2}:){19}[a-fA-F0-9]{2})\s*([SsPp*])(.*))";
    static const QRegularExpression rx(QRegularExpression::anchoredPattern(pattern));
    bool found = false;

    for (const QByteArray &rawLine : std::as_const(trustListFileContents)) {

        const QString line = QString::fromLatin1(rawLine.data(), rawLine.size());
        const QRegularExpressionMatch match = rx.match(line);
        if (!match.hasMatch()) {
            qCDebug(KLEOPATRA_LOG) << "line \"" << rawLine.data() << "\" does not match";
            out.write(rawLine + '\n');
            continue;
        }
        const QString cap2 = match.captured(2);
        if (cap2 != key && cap2 != keyColon) {
            qCDebug(KLEOPATRA_LOG) << qPrintable(key) << " != "
                                   << qPrintable(cap2) << " != "
                                   << qPrintable(keyColon);
            out.write(rawLine + '\n');
            continue;
        }
        found = true;
        const bool disabled = match.capturedView(1) == QLatin1Char('!');
        const QByteArray flags = match.captured(3).toLatin1();
        const QByteArray rests = match.captured(4).toLatin1();
        if (trust == Key::Ultimate)
            if (!disabled) { // unchanged
                out.write(rawLine + '\n');
            } else {
                out.write(keyColon.toLatin1() + ' ' + flags + rests + '\n');
            }
        else if (trust == Key::Never) {
            if (disabled) { // unchanged
                out.write(rawLine + '\n');
            } else {
                out.write('!' + keyColon.toLatin1() + ' ' + flags + rests + '\n');
            }
        }
        // else: trust == Key::Unknown
        // -> don't write - ie.erase
    }

    if (!found) {  // add
        out.write("\n");
        // write comment lines with DN attributes
        std::for_each(dn.begin(), dn.end(), [&out](const auto &attr) {
            out.write("# " + attr.name().toUtf8() + "=" + attr.value().toUtf8() + '\n');
        });
        if (trust == Key::Ultimate) {
            out.write(keyColon.toLatin1() + " S relax\n");
        } else if (trust == Key::Never) {
            out.write('!' + keyColon.toLatin1() + " S relax\n");
        }
    }

    if (!out.commit())
        return i18n("Failed to move file %1 to its final destination, %2: %3",
                    out.fileName(), trustListFile, out.errorString());

    return QString();

}

// static
QString run_gpgconf_reload_gpg_agent(const QString &gpgConfPath)
{
    if (gpgConfPath.isEmpty()) {
        return i18n("Could not find gpgconf executable");
    }

    QProcess p;
    p.start(gpgConfPath, QStringList() << QStringLiteral("--reload") << QStringLiteral("gpg-agent"));
    qCDebug(KLEOPATRA_LOG) <<  "starting " << qPrintable(gpgConfPath)
                           << " --reload gpg-agent";
    p.waitForFinished(-1);
    qCDebug(KLEOPATRA_LOG) << "done";
    if (p.error() == QProcess::UnknownError) {
        return QString();
    } else {
        return i18n("\"gpgconf --reload gpg-agent\" failed: %1", p.errorString());
    }
}

#undef q_func
#undef d_func

#include "moc_changeroottrustcommand.cpp"
#include "changeroottrustcommand.moc"
