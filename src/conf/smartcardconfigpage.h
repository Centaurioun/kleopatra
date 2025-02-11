/* -*- mode: c++; c-basic-offset:4 -*-
    conf/smartcardconfigpage.h

    This file is part of Kleopatra, the KDE keymanager
    SPDX-FileCopyrightText: 2022 g10 Code GmbH
    SPDX-FileContributor: Ingo Klöcker <dev@ingo-kloecker.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <KCModule>

#include <memory>

namespace Kleo
{
namespace Config
{

class SmartCardConfigurationPage : public KCModule
{
    Q_OBJECT
public:
    explicit SmartCardConfigurationPage(QWidget *parent = nullptr, const QVariantList &args = {});
    ~SmartCardConfigurationPage() override;

    void load() override;
    void save() override;
    void defaults() override;

private:
    class Private;
    std::unique_ptr<Private> d;
};

}
}
