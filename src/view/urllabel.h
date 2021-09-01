/*
    view/urllabel.h

    This file is part of Kleopatra, the KDE keymanager
    SPDX-FileCopyrightText: 2021 g10 Code GmbH
    SPDX-FileContributor: Ingo Klöcker <dev@ingo-kloecker.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QLabel>

namespace Kleo
{

class UrlLabel : public QLabel
{
    Q_OBJECT
public:
    explicit UrlLabel(QWidget *parent = nullptr);

    void setUrl(const QUrl &url, const QString &text = {});

protected:
    void focusInEvent(QFocusEvent *event) override;
    bool focusNextPrevChild(bool next) override;

private:
    using QLabel::setText;
};

}
