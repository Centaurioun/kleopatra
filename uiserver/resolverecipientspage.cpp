/* -*- mode: c++; c-basic-offset:4 -*-
    resolverecipientspage.cpp

    This file is part of Kleopatra, the KDE keymanager
    Copyright (c) 2007 Klarälvdalens Datakonsult AB

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

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

#include "resolverecipientspage.h"
#include "resolverecipientspage_p.h"
#include "certificateresolver.h"
#include "certificateselectiondialog.h"
#include <utils/formatting.h>
#include <models/keycache.h>
#include <kmime/kmime_header_parsing.h>

#include <gpgme++/key.h>

#include <KConfig>
#include <KGlobal>
#include <KLocalizedString>

#include <QButtonGroup>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPointer>
#include <QPushButton>
#include <QRadioButton>
#include <QSignalMapper>
#include <QStringList>
#include <QVBoxLayout>

#include <cassert>

using namespace GpgME;
using namespace Kleo;
using namespace KMime::Types;

ResolveRecipientsPage::ListWidget::ListWidget( QWidget* parent, Qt::WindowFlags flags ) : QWidget( parent, flags ), m_protocol( UnknownProtocol )
{
    m_listWidget = new QListWidget;
    m_listWidget->setSelectionMode( QAbstractItemView::MultiSelection );
    QVBoxLayout * const layout = new QVBoxLayout( this );
    layout->addWidget( m_listWidget );
    connect( m_listWidget, SIGNAL( itemSelectionChanged() ), this, SIGNAL( selectionChanged() ) );
}

ResolveRecipientsPage::ListWidget::~ListWidget()
{
}
void ResolveRecipientsPage::ListWidget::addEntry( const Mailbox& mbox )
{
    addEntry( mbox.prettyAddress(), mbox.prettyAddress(), mbox );
}

void ResolveRecipientsPage::ListWidget::addEntry( const QString& id, const QString& name )
{
    addEntry( id, name, Mailbox() );
}

void ResolveRecipientsPage::ListWidget::addEntry( const QString& id, const QString& name, const Mailbox& mbox )
{
    assert( !widgets.contains( id ) && !items.contains( id ) );
    QListWidgetItem* item = new QListWidgetItem;
    item->setData( IdRole, id );
    ItemWidget* wid = new ItemWidget( id, name, mbox, this );
    connect( wid, SIGNAL( changed() ), this, SIGNAL( completeChanged() ) );
    wid->setProtocol( m_protocol );
    item->setSizeHint( wid->sizeHint() );
    m_listWidget->addItem( item );
    m_listWidget->setItemWidget( item, wid );    
    widgets[id] = wid;
    items[id] = item;
}

Mailbox ResolveRecipientsPage::ListWidget::mailbox( const QString& id ) const
{
    return widgets.contains( id ) ? widgets[id]->mailbox() : Mailbox();
}

void ResolveRecipientsPage::ListWidget::setCertificates( const QString& id, const std::vector<Key>& pgp, const std::vector<Key>& cms )
{
    assert( widgets.contains( id ) );
    widgets[id]->setCertificates( pgp, cms );
}

Key ResolveRecipientsPage::ListWidget::selectedCertificate( const QString& id ) const
{
    return widgets.contains( id ) ? widgets[id]->selectedCertificate() : Key();
}


GpgME::Key ResolveRecipientsPage::ListWidget::selectedCertificate( const QString& id, GpgME::Protocol prot ) const
{
    return  widgets.contains( id ) ? widgets[id]->selectedCertificate( prot ) : Key();
}

QStringList ResolveRecipientsPage::ListWidget::identifiers() const
{
    return widgets.keys();
}

void ResolveRecipientsPage::ListWidget::setProtocol( GpgME::Protocol prot )
{
    if ( m_protocol == prot )
        return;
    m_protocol = prot;
    Q_FOREACH ( ItemWidget* i, widgets.values() )
        i->setProtocol( prot );
}

void ResolveRecipientsPage::ListWidget::removeEntry( const QString& id )
{
    if ( !widgets.contains( id ) )
        return;
    delete items[id];
    items.remove( id );
    delete widgets[id]; 
    widgets.remove( id );
}

void ResolveRecipientsPage::ListWidget::showSelectionDialog( const QString& id )
{    
    if ( !widgets.contains( id ) )
        return;
    widgets[id]->showSelectionDialog();
}

QStringList ResolveRecipientsPage::ListWidget::selectedEntries() const
{
    QStringList entries;
    const QList<QListWidgetItem*> items = m_listWidget->selectedItems();
    Q_FOREACH ( const QListWidgetItem* i, items )
    {
        entries.append( i->data( IdRole ).toString() );
    }
    return entries;
}

ResolveRecipientsPage::ItemWidget::ItemWidget( const QString& id, const QString& name, const Mailbox& mbox,
        QWidget* parent, Qt::WindowFlags flags ) : QWidget( parent, flags ), m_id( id ), m_mailbox( mbox ), m_protocol( UnknownProtocol )
{
    assert( !m_id.isEmpty() );
    QHBoxLayout* layout = new QHBoxLayout( this );
    layout->setMargin( 0 );
    layout->addSpacing( 15 );
    m_nameLabel = new QLabel;
    m_nameLabel->setText( name );
    layout->addWidget( m_nameLabel );
    layout->addStretch();
    m_certLabel = new QLabel;
    m_certLabel->setText( i18n( "<i>No certificate selected</i>" ) );
    layout->addWidget( m_certLabel );
    m_certCombo = new QComboBox;
    connect( m_certCombo, SIGNAL( currentIndexChanged( int ) ), 
             this, SIGNAL( changed() ) );
    layout->addWidget( m_certCombo );
    m_selectButton = new QPushButton;
    m_selectButton->setText( i18n( "..." ) );
    connect( m_selectButton, SIGNAL( clicked() ), 
             this, SLOT( showSelectionDialog() ) );
    layout->addWidget( m_selectButton );
    layout->addSpacing( 15 );
    setCertificates( std::vector<Key>(), std::vector<Key>() );
}

void ResolveRecipientsPage::ItemWidget::updateVisibility()
{
    m_certLabel->setVisible( m_certCombo->count() == 0 );
    m_certCombo->setVisible( m_certCombo->count() > 0 );    
}

ResolveRecipientsPage::ItemWidget::~ItemWidget()
{
}

QString ResolveRecipientsPage::ItemWidget::id() const
{
    return m_id;
}

void ResolveRecipientsPage::ItemWidget::showSelectionDialog()
{

    QPointer<CertificateSelectionDialog> dlg = new CertificateSelectionDialog( this );
    dlg->setSelectionMode( CertificateSelectionDialog::SingleSelection );
    dlg->setProtocol( m_protocol );
    dlg->setAllowedKeys( CertificateSelectionDialog::EncryptOnly );
    dlg->addKeys( PublicKeyCache::instance()->keys() );
    if ( dlg->exec() == QDialog::Accepted && dlg )
    {
        const std::vector<GpgME::Key> keys = dlg->selectedKeys();
        if ( !keys.empty() )
        {
            addCertificateToComboBox( keys[0] );
            selectCertificateInComboBox( keys[0] );
        }
    }

    delete dlg;
}

Mailbox ResolveRecipientsPage::ItemWidget::mailbox() const
{
    return m_mailbox;
}

void ResolveRecipientsPage::ItemWidget::selectCertificateInComboBox( const Key& key )
{
    m_certCombo->setCurrentIndex( m_certCombo->findData( key.keyID() ) );
}

void ResolveRecipientsPage::ItemWidget::addCertificateToComboBox( const GpgME::Key& key )
{
    m_certCombo->addItem( Formatting::formatForComboBox( key ), QByteArray( key.keyID() ) );    
    if ( m_certCombo->count() == 1 )
        m_certCombo->setCurrentIndex( 0 );
    updateVisibility();
}
    
void ResolveRecipientsPage::ItemWidget::resetCertificates()
{
    std::vector<Key> certs;
    Key selected;
    switch ( m_protocol )
    {
        case OpenPGP:
            certs = m_pgp;
            break;
        case CMS:
            certs = m_cms;
            break;
        case UnknownProtocol:
            certs = m_cms;
            certs.insert( certs.end(), m_pgp.begin(), m_pgp.end() ); 
    }
 
    m_certCombo->clear();
    Q_FOREACH ( const Key& i, certs )
        addCertificateToComboBox( i );
    if ( !m_selectedCertificates[m_protocol].isNull() )
        selectCertificateInComboBox( m_selectedCertificates[m_protocol] );
    else if ( m_certCombo->count() > 0 )
        m_certCombo->setCurrentIndex( 0 );
    updateVisibility();
    emit changed();
}

void ResolveRecipientsPage::ItemWidget::setProtocol( Protocol prot )
{
    if ( m_protocol == prot )
        return;
    m_selectedCertificates[m_protocol] = selectedCertificate();
    if ( m_protocol != UnknownProtocol )
        ( m_protocol == OpenPGP ? m_pgp : m_cms ) = certificates();
    m_protocol = prot;
    resetCertificates();
}

void ResolveRecipientsPage::ItemWidget::setCertificates( const std::vector<Key>& pgp, const std::vector<Key>& cms )
{
    m_pgp = pgp;
    m_cms = cms;
    resetCertificates();
}

Key ResolveRecipientsPage::ItemWidget::selectedCertificate() const
{
    return PublicKeyCache::instance()->findByKeyIDOrFingerprint( m_certCombo->itemData( m_certCombo->currentIndex(), ListWidget::IdRole ).toString().toStdString() );
}


GpgME::Key ResolveRecipientsPage::ItemWidget::selectedCertificate( GpgME::Protocol prot ) const
{
    return m_selectedCertificates.value( prot );
}

std::vector<Key> ResolveRecipientsPage::ItemWidget::certificates() const
{
    std::vector<Key> certs;
    for ( int i = 0; i < m_certCombo->count(); ++i )
        certs.push_back( PublicKeyCache::instance()->findByKeyIDOrFingerprint( m_certCombo->itemData( i, ListWidget::IdRole ).toString().toStdString() ) );
    return certs;
}

class ResolveRecipientsPage::Private {
    friend class ::ResolveRecipientsPage;
    ResolveRecipientsPage * const q;
public:
    explicit Private( ResolveRecipientsPage * qq );
    ~Private();
    
    void setSelectedProtocol( Protocol protocol );
    void selectionChanged();
    void removeSelectedEntries();
    void addRecipient();
    void addRecipient( const QString& id, const QString& name );
    void updateProtocolRBVisibility();
    void protocolSelected( int prot );
    void writeSelectedCertificatesToPreferences();
    
private:
    ListWidget* m_listWidget;
    QPushButton* m_addButton;
    QPushButton* m_removeButton;
    QRadioButton* m_pgpRB;
    QRadioButton* m_cmsRB;
    Protocol m_presetProtocol;
    Protocol m_selectedProtocol;
    bool m_multipleProtocolsAllowed;
    boost::shared_ptr<RecipientPreferences> m_recipientPreferences;
};

ResolveRecipientsPage::Private::Private( ResolveRecipientsPage * qq )
    : q( qq ), m_presetProtocol( UnknownProtocol ), m_selectedProtocol( m_presetProtocol ), m_multipleProtocolsAllowed( false ), m_recipientPreferences( new KConfigBasedRecipientPreferences( KGlobal::config() ) )
{
    q->setTitle( i18n( "<b>Recipients</b>" ) );
    QVBoxLayout* const layout = new QVBoxLayout( q );
    m_listWidget = new ListWidget;
    connect( m_listWidget, SIGNAL( selectionChanged() ), q, SLOT( selectionChanged() ) );
    connect( m_listWidget, SIGNAL( completeChanged() ), q, SIGNAL( completeChanged() ) );
    layout->addWidget( m_listWidget );
    QWidget* buttonWidget = new QWidget;
    QHBoxLayout* buttonLayout = new QHBoxLayout( buttonWidget );
    buttonLayout->setMargin( 0 );
    m_addButton = new QPushButton;
    connect( m_addButton, SIGNAL( clicked() ), q, SLOT( addRecipient() ) );
    m_addButton->setText( i18n( "Add Recipient..." ) );
    buttonLayout->addWidget( m_addButton );
    m_removeButton = new QPushButton;
    m_removeButton->setEnabled( false );
    m_removeButton->setText( i18n( "Remove Selected" ) );
    connect( m_removeButton, SIGNAL( clicked() ), 
             q, SLOT( removeSelectedEntries() ) );  
    buttonLayout->addWidget( m_removeButton );
    buttonLayout->addStretch();
    layout->addWidget( buttonWidget );
    QWidget* protocolWidget = new QWidget;
    QHBoxLayout* protocolLayout = new QHBoxLayout( protocolWidget );
    QButtonGroup* protocolGroup = new QButtonGroup( q );
    connect( protocolGroup, SIGNAL( buttonClicked( int ) ), q, SLOT( protocolSelected( int ) ) );
    m_pgpRB = new QRadioButton;
    m_pgpRB->setText( i18n( "OpenPGP" ) );
    protocolGroup->addButton( m_pgpRB, OpenPGP );
    protocolLayout->addWidget( m_pgpRB );
    m_cmsRB = new QRadioButton;
    m_cmsRB->setText( i18n( "S/MIME" ) );
    protocolGroup->addButton( m_cmsRB, CMS );
    protocolLayout->addWidget( m_cmsRB );
    protocolLayout->addStretch();
    layout->addWidget( protocolWidget );
}

ResolveRecipientsPage::Private::~Private() {}

void ResolveRecipientsPage::Private::updateProtocolRBVisibility()
{
    const bool visible = !m_multipleProtocolsAllowed && m_presetProtocol == UnknownProtocol;
    m_cmsRB->setVisible( visible );
    m_pgpRB->setVisible( visible );
    if ( visible )
    {
        if ( m_selectedProtocol == CMS )
            m_cmsRB->click();
        else
            m_pgpRB->click();
    }
}
    
bool ResolveRecipientsPage::isComplete() const
{
    const QStringList ids = d->m_listWidget->identifiers();
    if ( ids.isEmpty() )
        return false;
    
    Q_FOREACH ( const QString& i, ids )
    {
        if ( d->m_listWidget->selectedCertificate( i ).isNull() )
            return false;
    }
     
    return true;
}

ResolveRecipientsPage::ResolveRecipientsPage( QWidget * parent )
    : WizardPage( parent ), d( new Private( this ) )
{
}

ResolveRecipientsPage::~ResolveRecipientsPage() {}

Protocol ResolveRecipientsPage::selectedProtocol() const
{
    return d->m_selectedProtocol;
}

void ResolveRecipientsPage::Private::setSelectedProtocol( Protocol protocol )
{
    if ( m_selectedProtocol == protocol )
        return;
    m_selectedProtocol = protocol;
    m_listWidget->setProtocol( m_selectedProtocol );
    emit q->selectedProtocolChanged();
}

void ResolveRecipientsPage::Private::protocolSelected( int p )
{
    const Protocol protocol = static_cast<Protocol>( p );
    assert( protocol != UnknownProtocol );
    setSelectedProtocol( protocol );
}

void ResolveRecipientsPage::setPresetProtocol( Protocol prot )
{
    if ( d->m_presetProtocol == prot )
        return;
    d->m_presetProtocol = prot;
    d->setSelectedProtocol( prot );
    if ( prot != UnknownProtocol )
        d->m_multipleProtocolsAllowed = false;
    d->updateProtocolRBVisibility();
}

Protocol ResolveRecipientsPage::presetProtocol() const
{
    return d->m_presetProtocol;
}

bool ResolveRecipientsPage::multipleProtocolsAllowed() const
{
    return d->m_multipleProtocolsAllowed;
}

void ResolveRecipientsPage::setMultipleProtocolsAllowed( bool allowed )
{
    if ( d->m_multipleProtocolsAllowed == allowed )
        return;
    d->m_multipleProtocolsAllowed = allowed;
    if ( d->m_multipleProtocolsAllowed )
    {
        setPresetProtocol( UnknownProtocol );
        d->setSelectedProtocol( UnknownProtocol );
    }
    d->updateProtocolRBVisibility();
}


void ResolveRecipientsPage::Private::addRecipient( const QString& id, const QString& name )
{
    m_listWidget->addEntry( id, name );    
}

void ResolveRecipientsPage::Private::addRecipient()
{
    int i = 0;
    const QStringList existing = m_listWidget->identifiers();
    QString rec = i18n( "Additional Recipient" );
    while ( existing.contains( rec ) )
        rec = i18nc( "%1 == number", "Additional Recipient (%1)", ++i );
    addRecipient( rec, rec );
    m_listWidget->showSelectionDialog( rec );
}

namespace {

    std::vector<Key> makeSuggestions( const boost::shared_ptr<RecipientPreferences>& prefs, const Mailbox& mb, GpgME::Protocol prot )
    {
        std::vector<Key> suggestions;
        const Key remembered = prefs ? prefs->preferredCertificate( mb, prot ) : Key();
         if ( !remembered.isNull() )
             suggestions.push_back( remembered );
         else
             suggestions = CertificateResolver::resolveRecipient( mb, prot );
         return suggestions;
    }
}

void ResolveRecipientsPage::setRecipients( const std::vector<Mailbox>& recipients )
{
    uint cmsCount = 0;
    uint pgpCount = 0;
    Q_FOREACH( const Mailbox& i, recipients )
    {
        const QString address = i.prettyAddress();
        d->addRecipient( address, address );
        const std::vector<Key> pgp = makeSuggestions( d->m_recipientPreferences, i, OpenPGP );
        const std::vector<Key> cms = makeSuggestions( d->m_recipientPreferences, i, CMS );
        pgpCount += pgp.empty() ? 0 : 1;
        cmsCount += cms.empty() ? 0 : 1;
        d->m_listWidget->setCertificates( address, pgp, cms );
    }
    if ( d->m_presetProtocol == UnknownProtocol && !d->m_multipleProtocolsAllowed )
        ( cmsCount > pgpCount ? d->m_cmsRB : d->m_pgpRB )->click();
}

std::vector<Key> ResolveRecipientsPage::resolvedCertificates() const
{
    std::vector<Key> certs;
    Q_FOREACH( const QString& i, d->m_listWidget->identifiers() )
    {
        const GpgME::Key cert = d->m_listWidget->selectedCertificate( i );
        if ( !cert.isNull() )
            certs.push_back( cert );
    }
    return certs;
}

void ResolveRecipientsPage::Private::selectionChanged()
{
    m_removeButton->setEnabled( !m_listWidget->selectedEntries().isEmpty() );
}

void ResolveRecipientsPage::Private::removeSelectedEntries()
{
    Q_FOREACH ( const QString& i, m_listWidget->selectedEntries() )
        m_listWidget->removeEntry( i );
    emit q->completeChanged();
}

void ResolveRecipientsPage::setRecipientsUserMutable( bool isMutable )
{
    d->m_addButton->setVisible( isMutable );
    d->m_removeButton->setVisible( isMutable );
}

bool ResolveRecipientsPage::recipientsUserMutable() const
{
    return d->m_addButton->isVisible();
}


boost::shared_ptr<RecipientPreferences> ResolveRecipientsPage::recipientPreferences() const
{
    return d->m_recipientPreferences;
}

void ResolveRecipientsPage::setRecipientPreferences( const boost::shared_ptr<RecipientPreferences>& prefs )
{
    d->m_recipientPreferences = prefs;
}

void ResolveRecipientsPage::Private::writeSelectedCertificatesToPreferences()
{
    if ( !m_recipientPreferences )
        return;
    
    Q_FOREACH ( const QString& i, m_listWidget->identifiers() )
    {
        const Mailbox mbox = m_listWidget->mailbox( i );
        if ( !mbox.hasAddress() )
            continue;
        const Key pgp = m_listWidget->selectedCertificate( i, OpenPGP );
        if ( !pgp.isNull() )
            m_recipientPreferences->setPreferredCertificate( mbox, OpenPGP, pgp );
        const Key cms = m_listWidget->selectedCertificate( i, CMS );
        if ( !cms.isNull() )
            m_recipientPreferences->setPreferredCertificate( mbox, CMS, cms );
    }
}
    
void ResolveRecipientsPage::onNext() {
    d->writeSelectedCertificatesToPreferences();
}

#include "moc_resolverecipientspage_p.cpp"
#include "moc_resolverecipientspage.cpp"
