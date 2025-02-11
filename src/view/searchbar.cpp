/* -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
    view/searchbar.cpp

    This file is part of Kleopatra, the KDE keymanager
    SPDX-FileCopyrightText: 2007 Klarälvdalens Datakonsult AB
    SPDX-FileCopyrightText: 2022 g10 Code GmbH
    SPDX-FileContributor: Ingo Klöcker <dev@ingo-kloecker.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include <config-kleopatra.h>

#include "searchbar.h"

#include <Libkleo/Algorithm>
#include <Libkleo/KeyCache>
#include <Libkleo/KeyFilter>
#include <Libkleo/KeyFilterManager>

#include <KLocalizedString>

#include <QLineEdit>
#include <QComboBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSortFilterProxyModel>

#include <gpgme++/key.h>

#include "kleopatra_debug.h"

using namespace Kleo;

namespace
{

class ProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        const QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
        const auto matchContexts = qvariant_cast<KeyFilter::MatchContexts>(sourceModel()->data(index, KeyFilterManager::FilterMatchContextsRole));
        return matchContexts & KeyFilter::Filtering;
    }
};

}

class SearchBar::Private
{
    friend class ::Kleo::SearchBar;
    SearchBar *const q;
public:
    explicit Private(SearchBar *qq);
    ~Private();

private:
    void slotKeyFilterChanged(int idx)
    {
        Q_EMIT q->keyFilterChanged(keyFilter(idx));
    }

    std::shared_ptr<KeyFilter> keyFilter(int idx) const
    {
        const QModelIndex mi = proxyModel->mapToSource(proxyModel->index(idx, 0));
        return KeyFilterManager::instance()->fromModelIndex(mi);
    }

    std::shared_ptr<KeyFilter> currentKeyFilter() const
    {
        return keyFilter(combo->currentIndex());
    }

    QString currentKeyFilterID() const
    {
        if (const std::shared_ptr<KeyFilter> f = currentKeyFilter()) {
            return f->id();
        } else {
            return QString();
        }
    }

    static auto notCertifiedKeysFilterId()
    {
        static const QString filterId = QStringLiteral("not-certified-certificates");
        return filterId;
    }

    void listNotCertifiedKeys() const
    {
        lineEdit->clear();
        combo->setCurrentIndex(combo->findData(notCertifiedKeysFilterId()));
        Q_EMIT q->keyFilterChanged(keyFilter(combo->currentIndex()));
    }

    void showOrHideCertifyButton() const
    {
        if (!KeyCache::instance()->initialized()) {
            return;
        }
        const auto filter = KeyFilterManager::instance()->keyFilterByID(notCertifiedKeysFilterId());
        if (filter) {
            if (Kleo::any_of(KeyCache::instance()->keys(),
                             [filter](const auto &key) {
                                 return filter->matches(key, KeyFilter::Filtering);
                             })) {
                certifyButton->show();
                return;
            }
        } else {
            qCDebug(KLEOPATRA_LOG) << __func__ << "Key filter with id" << notCertifiedKeysFilterId() << "not found";
        }
        certifyButton->hide();
    }

private:
    ProxyModel *proxyModel;
    QLineEdit *lineEdit;
    QComboBox *combo;
    QPushButton *certifyButton;
};

SearchBar::Private::Private(SearchBar *qq)
    : q(qq)
{
    auto layout = new QHBoxLayout(q);
    layout->setContentsMargins(0, 0, 0, 0);
    lineEdit = new QLineEdit(q);
    lineEdit->setClearButtonEnabled(true);
    lineEdit->setPlaceholderText(i18n("Search..."));
    lineEdit->setAccessibleName(i18n("Filter certificates by text"));
    lineEdit->setToolTip(i18n("Show only certificates that match the entered search term."));
    layout->addWidget(lineEdit, /*stretch=*/1);
    combo = new QComboBox(q);
    combo->setAccessibleName(i18n("Filter certificates by category"));
    combo->setToolTip(i18n("Show only certificates that belong to the selected category."));
    layout->addWidget(combo);
    certifyButton = new QPushButton(q);
    certifyButton->setIcon(QIcon::fromTheme(QStringLiteral("security-medium")));
    certifyButton->setAccessibleName(i18n("Show not certified certificates"));
    certifyButton->setToolTip(i18n("Some certificates are not yet certified. "
                                   "Click here to see a list of these certificates."
                                   "<br/><br/>"
                                   "Certification is required to make sure that the certificates "
                                   "actually belong to the identity they claim to belong to."));
    certifyButton->hide();
    layout->addWidget(certifyButton);

    proxyModel = new ProxyModel{q};
    proxyModel->setSourceModel(KeyFilterManager::instance()->model());
    proxyModel->sort(0, Qt::AscendingOrder);
    combo->setModel(proxyModel);

    KDAB_SET_OBJECT_NAME(layout);
    KDAB_SET_OBJECT_NAME(lineEdit);
    KDAB_SET_OBJECT_NAME(combo);
    KDAB_SET_OBJECT_NAME(certifyButton);

    connect(lineEdit, &QLineEdit::textChanged, q, &SearchBar::stringFilterChanged);
    connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), q, [this](int index) { slotKeyFilterChanged(index); });
    connect(certifyButton, &QPushButton::clicked, q, [this]() { listNotCertifiedKeys(); });

    connect(KeyCache::instance().get(), &KeyCache::keyListingDone,
            q, [this]() { showOrHideCertifyButton(); });
    showOrHideCertifyButton();
}

SearchBar::Private::~Private() {}

SearchBar::SearchBar(QWidget *parent, Qt::WindowFlags f)
    : QWidget(parent, f), d(new Private(this))
{

}

SearchBar::~SearchBar() {}

void SearchBar::updateClickMessage(const QString &shortcutStr)
{
    d->lineEdit->setPlaceholderText(i18n("Search...<%1>", shortcutStr));
}

// slot
void SearchBar::setStringFilter(const QString &filter)
{
    d->lineEdit->setText(filter);
}

// slot
void SearchBar::setKeyFilter(const std::shared_ptr<KeyFilter> &kf)
{
    const QModelIndex sourceIndex = KeyFilterManager::instance()->toModelIndex(kf);
    const QModelIndex proxyIndex = d->proxyModel->mapFromSource(sourceIndex);
    if (proxyIndex.isValid()) {
        d->combo->setCurrentIndex(proxyIndex.row());
    } else {
        d->combo->setCurrentIndex(0);
    }
}

// slot
void SearchBar::setChangeStringFilterEnabled(bool on)
{
    d->lineEdit->setEnabled(on);
}

// slot
void SearchBar::setChangeKeyFilterEnabled(bool on)
{
    d->combo->setEnabled(on);
}

QLineEdit *SearchBar::lineEdit() const
{
    return d->lineEdit;
}

#include "moc_searchbar.cpp"
#include "searchbar.moc"
