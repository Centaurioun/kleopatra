/* -*- mode: c++; c-basic-offset:4 -*-
    utils/keyusage.h

    This file is part of Kleopatra, the KDE keymanager
    SPDX-FileCopyrightText: 2022 g10 Code GmbH
    SPDX-FileContributor: Ingo Klöcker <dev@ingo-kloecker.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QFlags>

namespace Kleo
{

class KeyUsage
{
public:
    enum Flag {
        Certify = 1,
        Sign = 2,
        Encrypt = 4,
        Authenticate = 8,
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    KeyUsage()
    {
    }

    explicit KeyUsage(Flags flags)
        : mFlags{flags}
    {
    }

    void setValue(Flags flags)
    {
        mFlags = flags;
    }
    Flags value() const
    {
        return mFlags;
    }

    void setCanAuthenticate(bool set)
    {
        mFlags.setFlag(Authenticate, set);
    }
    bool canAuthenticate() const
    {
        return mFlags & Authenticate;
    }

    void setCanCertify(bool set)
    {
        mFlags.setFlag(Certify, set);
    }
    bool canCertify() const
    {
        return mFlags & Certify;
    }

    void setCanEncrypt(bool set)
    {
        mFlags.setFlag(Encrypt, set);
    }
    bool canEncrypt() const
    {
        return mFlags & Encrypt;
    }

    void setCanSign(bool set)
    {
        mFlags.setFlag(Sign, set);
    }
    bool canSign() const
    {
        return mFlags & Sign;
    }

private:
    Flags mFlags;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(KeyUsage::Flags)

}
