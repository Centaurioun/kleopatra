/*  commands/importperkeycommand.cpp

    This file is part of Kleopatra, the KDE keymanager
    Copyright (c) 2017 Intevation GmbH

    Kleopatra is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Kleopatra is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
    MA  02110-1301  USA

    In addition, as a special exception, the copyright holders give
    permission to link the code of this program with any edition of
    the Qt library by Trolltech AS, Norway (or with modified versions
    of Qt that use the same license as Qt), and distribute linked
    combinations including the two.  You must obey the GNU General
    Public License in all respects for all of the code used other than
    Qt.  If you modify this file, you may extend this exception to
    your version of the file, but you are not obligated to do so.  If
    you do not wish to do so, delete this exception statement from
    your version.
*/

#include <config-kleopatra.h>

#include "importpaperkeycommand.h"

#include <utils/gnupg-helper.h>

#include <gpgme++/key.h>
#include <gpgme++/importresult.h>
#include <QGpgME/Protocol>
#include <QGpgME/ImportJob>
#include <QGpgME/ExportJob>

#include <Libkleo/KeyCache>

#include <KLocalizedString>
#include <KMessageBox>

#include <QProcess>
#include <QFileDialog>
#include <QTextStream>

#include "kleopatra_debug.h"
#include "command_p.h"

using namespace Kleo;
using namespace Kleo::Commands;
using namespace GpgME;

ImportPaperKeyCommand::ImportPaperKeyCommand(const GpgME::Key &k) :
    GnuPGProcessCommand(k)
{
}

QStringList ImportPaperKeyCommand::arguments() const
{
    const Key key = d->key();
    QStringList result;

    result << paperKeyInstallPath() << QStringLiteral("--pubring")
           << mTmpDir.path() + QStringLiteral("/pubkey.gpg")
           << QStringLiteral("--secrets")
           << mTmpDir.path() + QStringLiteral("/secrets.txt");

    return result;
}

void ImportPaperKeyCommand::exportResult(const GpgME::Error &err, const QByteArray &data)
{
    if (err) {
        d->error(err.asString(), errorCaption());
        d->finished();
        return;
    }
    if (!mTmpDir.isValid()) {
        // Should not happen so no i18n
        d->error(QStringLiteral("Failed to get temporary directory"), errorCaption());
        qCWarning(KLEOPATRA_LOG) << "Failed to get temporary dir";
        d->finished();
        return;
    }
    const QString fileName = mTmpDir.path() + QStringLiteral("/pubkey.gpg");
    QFile f(fileName);
    if (!f.open(QIODevice::WriteOnly)) {
        d->error(QStringLiteral("Failed to create temporary file"), errorCaption());
        qCWarning(KLEOPATRA_LOG) << "Failed to open tmp file";
        d->finished();
        return;
    }
    f.write(data);
    f.close();

    // Copy and sanitize input a bit
    QFile input(mFileName);

    if (!input.open(QIODevice::ReadOnly)) {
        d->error(xi18n("Cannot open <filename>%1</filename> for reading.", mFileName), errorCaption());
        d->finished();
        return;
    }
    const QString outName = mTmpDir.path() + QStringLiteral("/secrets.txt");
    QFile out(outName);
    if (!out.open(QIODevice::WriteOnly)) {
        // Should not happen
        d->error(QStringLiteral("Failed to create temporary file"), errorCaption());
        qCWarning(KLEOPATRA_LOG) << "Failed to open tmp file for writing";
        d->finished();
        return;
    }

    QTextStream in(&input);
    while (!in.atEnd()) {
        // Paperkey is picky, tabs may not be part. Neither may be empty lines.
        const QString line = in.readLine().trimmed().replace(QLatin1Char('\t'), QStringLiteral("  ")) +
            QLatin1Char('\n');
        out.write(line.toUtf8());
    }
    input.close();
    out.close();

    GnuPGProcessCommand::doStart();
}

void ImportPaperKeyCommand::postSuccessHook(QWidget *)
{
    qCDebug(KLEOPATRA_LOG) << "Paperkey secrets restore finished successfully.";

    auto importjob = QGpgME::openpgp()->importJob();

    auto data = process()->readAllStandardOutput();

    connect(importjob, &QGpgME::ImportJob::result, this, [this](ImportResult result, QString, Error) {
        if (result.error()) {
            d->error(result.error().asString(), errorCaption());
            finished();
            return;
        }
        if (!result.numSecretKeysImported() ||
            (result.numSecretKeysUnchanged() == result.numSecretKeysImported())) {
            d->error(i18n("Failed to restore any secret keys."), errorCaption());
            finished();
            return;
        }

        // Refresh the key after success
        auto key = d->key();
        key.update();
        KeyCache::mutableInstance()->insert(key);
        finished();
        return;
    });
    importjob->start(data);
}

void ImportPaperKeyCommand::doStart()
{
    if (paperKeyInstallPath().isNull()) {
        KMessageBox::sorry(d->parentWidgetOrView(),
                           xi18nc("@info", "<para><application>Kleopatra</application> uses "
                                           "<application>PaperKey</application> to import your "
                                           "text backup.</para>"
                                           "<para>Please make sure it is installed.</para>"),
                           i18nc("@title", "Failed to find PaperKey executable."));
        return;
    }


    mFileName = QFileDialog::getOpenFileName(d->parentWidgetOrView(), i18n("Select input file"),
                                             QString(),
                                             QStringLiteral("Paper backup(*.txt)"));

    if (mFileName.isEmpty()) {
        d->finished();
        return;
    }

    auto exportJob = QGpgME::openpgp()->publicKeyExportJob();
    // Do not change to new style connect without testing on
    // Windows / mingw first for compatibility please.
    connect(exportJob, SIGNAL(result(GpgME::Error, QByteArray)),
            this, SLOT(exportResult(GpgME::Error, QByteArray)));
    exportJob->start(QStringList() << QLatin1String(d->key().primaryFingerprint()));
}

QString ImportPaperKeyCommand::errorCaption() const
{
    return i18nc("@title:window", "Error importing secret key");
}

QString ImportPaperKeyCommand::crashExitMessage(const QStringList &args) const
{
    return xi18nc("@info",
                  "<para>The GPG process that tried to restore the secret key "
                  "ended prematurely because of an unexpected error.</para>"
                  "<para>Please check the output of <icode>%1</icode> for details.</para>",
                  args.join(QLatin1Char(' ')));
}

QString ImportPaperKeyCommand::errorExitMessage(const QStringList &args) const
{
    return xi18nc("@info",
                  "<para>An error occurred while trying to restore the secret key.</para> "
                  "<para>The output from <command>%1</command> was:</para>"
                  "<para><message>%2</message></para>",
                  args[0], errorString());
}

QString ImportPaperKeyCommand::successMessage(const QStringList &) const
{
    return xi18nc("@info", "Successfully restored the secret key parts from <filename>%1</filename>",
                  mFileName);
}
