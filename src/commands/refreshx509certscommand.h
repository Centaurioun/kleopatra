/* -*- mode: c++; c-basic-offset:4 -*-
    commands/refreshx509certscommand.h

    This file is part of Kleopatra, the KDE keymanager
    SPDX-FileCopyrightText: 2008 Klarälvdalens Datakonsult AB

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef __KLEOPATRA_COMMMANDS_REFRESHX509CERTSCOMMAND_H__
#define __KLEOPATRA_COMMMANDS_REFRESHX509CERTSCOMMAND_H__

#include <commands/gnupgprocesscommand.h>

namespace Kleo
{
namespace Commands
{

class RefreshX509CertsCommand : public GnuPGProcessCommand
{
    Q_OBJECT
public:
    explicit RefreshX509CertsCommand(QAbstractItemView *view, KeyListController *parent);
    explicit RefreshX509CertsCommand(KeyListController *parent);
    ~RefreshX509CertsCommand() override;

private:
    bool preStartHook(QWidget *) const override;

    QStringList arguments() const override;

    QString errorCaption() const override;
    QString successCaption() const override;

    QString crashExitMessage(const QStringList &) const override;
    QString errorExitMessage(const QStringList &) const override;
    QString successMessage(const QStringList &) const override;
};

}
}

#endif // __KLEOPATRA_COMMMANDS_REFRESHX509CERTSCOMMAND_H__
