/* -*- mode: c++; c-basic-offset:4 -*-
    utils/scrollarea.h

    This file is part of Kleopatra, the KDE keymanager
    SPDX-FileCopyrightText: 2007 Klarälvdalens Datakonsult AB
    SPDX-FileCopyrightText: 2022 g10 Code GmbH
    SPDX-FileContributor: Ingo Klöcker <dev@ingo-kloecker.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include <QScrollArea>

namespace Kleo
{

/**
 * This class improves a few aspects of QScrollArea for usage by us, in
 * particular, for vertically scrollable widgets.
 *
 * If sizeAdjustPolicy is set to QAbstractScrollArea::AdjustToContents,
 * then the scroll area will (try to) adjust its size to the widget to avoid
 * scroll bars as much as possible.
 */
class ScrollArea : public QScrollArea
{
    Q_OBJECT

public:
    /**
     * Creates a scroll area with a QWidget with QVBoxLayout that is flagged
     * as resizable.
     */
    explicit ScrollArea(QWidget *parent = nullptr);
    ~ScrollArea() override;

    /**
     * Sets the maximum height that the scroll area should automatically resize
     * to to \p maxHeight. By default, or if \p maxHeight is negative, the
     * scroll area will resize to at most 2/3 of the desktop's height.
     */
    void setMaximumAutoAdjustHeight(int maxHeight);

    /**
     * Returns the maximum height that the scroll area will automatically resize
     * to.
     */
    int maximumAutoAdjustHeight() const;

    /**
     * Reimplemented to add the minimum size hint of the widget.
     */
    QSize minimumSizeHint() const override;

    /**
     * Reimplemented to remove the caching of the size/size hint of the
     * widget and to add the horizontal size hint of the vertical scroll bar
     * unless it is explicitly turned off.
     */
    QSize sizeHint() const override;

private:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    int mMaximumAutoAdjustHeight = -1;
};

}


