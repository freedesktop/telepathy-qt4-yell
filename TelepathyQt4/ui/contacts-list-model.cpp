/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2008-2010 Collabora Ltd. <http://www.collabora.co.uk/>
 * Copyright (C) 2008-2010 Nokia Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <TelepathyQt4/ui/ContactsListModel>
#include "TelepathyQt4/ui/_gen/contacts-list-model.moc.hpp"

#include <TelepathyQt4/Types>
#include <TelepathyQt4/Contact>
#include <TelepathyQt4/ContactManager>
#include <TelepathyQt4/PendingConnection>
#include <TelepathyQt4/PendingContacts>
#include <TelepathyQt4/PendingOperation>
#include <TelepathyQt4/PendingReady>

#include "contact-item.h"



namespace Tp
{
    ContactsListModel::ContactsListModel(const Tp::AccountManagerPtr &am, QObject *parent)
     : QAbstractItemModel(parent),
     mAM(am),
     mRootItem(new AbstractTreeItem())
    {
        connect(mAM->becomeReady(),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(onAccountManagerReady(Tp::PendingOperation *)));

        QHash<int, QByteArray> roles;
        roles[IdRole] = "id";
        roles[ContactRole] = "contact";
        roles[AliasRole] = "aliasName";
        roles[AvatarRole] = "avatar";
        roles[PresenceStatusRole] = "presenceStatus";
        roles[PresenceTypeRole] = "presenceType";
        roles[PresenceMessageRole] = "presenceMessage";
        roles[SubscriptionStateRole] = "subscriptionState";
        roles[PublishStateRole] = "publishState";
        roles[BlockedRole] = "blocked";
        roles[GroupsRole] = "groups";
        setRoleNames(roles);
    }

    ContactsListModel::~ContactsListModel()
    {
    }

    void ContactsListModel::onAccountManagerReady(Tp::PendingOperation *)
    {
        Features features;
        features << Account::FeatureCore
                << Account::FeatureAvatar
                << Account::FeatureProtocolInfo
                << Account::FeatureCapabilities
                << Account::FeatureProfile;

        QList<AccountPtr> accountsList = mAM->allAccounts();
        QList<AccountPtr>::ConstIterator ac_it;
        for(ac_it = accountsList.constBegin(); ac_it != accountsList.constEnd(); ++ac_it) {
            AccountPtr accountPtr = (*ac_it);

            if(accountPtr->connectionStatus() == Tp::ConnectionStatusConnected) {
                connect(accountPtr->becomeReady(features),
                        SIGNAL(finished(Tp::PendingOperation *)),
                        SLOT(onAccountReady(Tp::PendingOperation *)));
            }
        }
    }

    void ContactsListModel::onAccountReady(Tp::PendingOperation* op)
    {
        if (op->isError()) {
            qWarning() << "Account cannot become ready";
            return;
        }

        PendingReady *pr = qobject_cast<PendingReady *>(op);
        AccountPtr accountPtr = AccountPtr(qobject_cast<Account *>(pr->object()));

        if(accountPtr->haveConnection()) {
           addConnection(accountPtr->connection());
        }
    }

    void ContactsListModel::onPresencePublicationRequested(const Tp::Contacts &contacts)
    {
        AbstractTreeItem *item;
        bool exists;
        foreach (const ContactPtr &contact, contacts) {
            exists = false;
            item = createContactItem(contact, exists);
            }
    }

    void ContactsListModel::addConnection(const ConnectionPtr &conn)
    {
        Features features;
        features << Connection::FeatureCore
                << Connection::FeatureSelfContact
                << Connection::FeatureSimplePresence
                << Connection::FeatureRoster
                << Connection::FeatureRosterGroups
                << Connection::FeatureAccountBalance;
        connect(conn->becomeReady(features),
                SIGNAL(finished(Tp::PendingOperation *)),
                SLOT(onConnectionReady(Tp::PendingOperation *)));
    }

    void ContactsListModel::removeConnection(const ConnectionPtr &conn)
    {
        QList<AbstractTreeItem*>::const_iterator it;
        for(it = mRootItem->childItems().constBegin(); it != mRootItem->childItems().constEnd(); ++it ) {
            ContactItem* item = dynamic_cast<ContactItem*>(*it);
            if (item->contact()->manager()->connection() == conn) {
                mRootItem->removeChildItem(*it);
                continue;
            }
        }
        mConns.removeOne(conn);
    }

    void ContactsListModel::onConnectionReady(Tp::PendingOperation *op)
    {
        if (op->isError()) {
            qWarning() << "Connection cannot become ready";
            return;
        }

        PendingReady *pr = qobject_cast<PendingReady *>(op);
        ConnectionPtr conn = ConnectionPtr(qobject_cast<Connection *>(pr->object()));
        mConns.append(conn);
        connect(conn.data(),
                SIGNAL(invalidated(Tp::DBusProxy *, const QString &, const QString &)),
                SLOT(onConnectionInvalidated(Tp::DBusProxy *, const QString &, const QString &)));
        connect(conn->contactManager(),
                SIGNAL(presencePublicationRequested(const Tp::Contacts &)),
                SLOT(onPresencePublicationRequested(const Tp::Contacts &)));

        AbstractTreeItem *item;
        bool exists;
        foreach (const ContactPtr &contact, conn->contactManager()->allKnownContacts()) {
            exists = false;
            item = createContactItem(contact, exists);
            }
    }

    void ContactsListModel::onConnectionInvalidated(DBusProxy *proxy,
            const QString &errorName, const QString &errorMessage)
    {
        qWarning() << "onConnectionInvalidated: connection became invalid:" <<
            errorName << "-" << errorMessage;
        foreach (const ConnectionPtr &conn, mConns) {
            if (conn.data() == proxy) {
                mConns.removeOne(conn);
            }
        }
    }

    AbstractTreeItem* ContactsListModel::createContactItem(const Tp::ContactPtr &contact, bool &exists)
    {
        QList<AbstractTreeItem*>::const_iterator it;
        for(it = mRootItem->childItems().constBegin(); it != mRootItem->childItems().constEnd(); ++it ) {
            ContactItem* item = dynamic_cast<ContactItem*>(*it);
            if(item) {
                if(item->contact() == contact) {
                    exists = true;
                    return item;
                }
            }
        }

        ContactItem* contactItem = new ContactItem();
        contactItem->setContact(contact);
        mRootItem->appendChildItem(contactItem);
        return contactItem;
    }

    QModelIndex ContactsListModel::index(int row, int column, const QModelIndex &parent) const
    {
        // 1 column list, so invalid index if the column is not 1.
        if (parent.isValid() && parent.column() != 0) {
            return QModelIndex();
        }

        // Get the parent item.
        AbstractTreeItem *parentItem = item(parent);

        // Get all the parent's children.
        QList<AbstractTreeItem*> children = parentItem->childItems();

        // Check the row doesn't go beyond the end of the list of children.
        if (row >= children.length()) {
            return QModelIndex();
        }

        // Return the index to the item.
        return createIndex(row, column, children.at(row));
    }

    QModelIndex ContactsListModel::parent(const QModelIndex &index) const
    {
        // If the index is invalid, return an invalid parent index.
        if (!index.isValid()) {
            return QModelIndex();
        }

        // Get the item we have been passed, and it's parent
        AbstractTreeItem *childItem = item(index);
        AbstractTreeItem *parentItem = childItem->parentItem();

        // If the parent is the root item, then the parent index of the index we were passed is
        // by definition an invalid index.
        if (parentItem == mRootItem) {
            return QModelIndex();
        }

        // The parent of the item is not the root item, meaning that the parent must have a parent too.
        AbstractTreeItem *parentOfParentItem = parentItem->parentItem();

        // As stated in the previous comment, something is really wrong if it doesn't have a parent.
        Q_ASSERT(parentOfParentItem);
        if (!parentOfParentItem) {
            return createIndex(0, 0, parentItem);
        }

        // Return the model index of the parent item.
        return createIndex(parentOfParentItem->childItems().lastIndexOf(parentItem), 0, parentItem);
    }

    int ContactsListModel::rowCount(const QModelIndex &parent) const
    {
        // If the parent is invalid, then this request is for the root item.
        if (!parent.isValid()) {
            return mRootItem->childItems().length();
        }

        // Get the item from the internal pointer of the ModelIndex.
        AbstractTreeItem *item = static_cast<AbstractTreeItem*>(parent.internalPointer());

        // If the item is valid, return the number of children it has.
        if (item) {
            return item->childItems().length();
        }

        // Otherwise, return 0
        return 0;
    }

    AbstractTreeItem* ContactsListModel::item(const QModelIndex &index) const
    {
        if (index.isValid()) {
            AbstractTreeItem *item = static_cast<AbstractTreeItem*>(index.internalPointer());
             if (item) {
                 return item;
             }
         }

         return mRootItem;
    }

    int ContactsListModel::columnCount(const QModelIndex &parent) const
    {
        Q_UNUSED(parent);

        // All items have the same number of columns
        return 1;
    }

    QVariant ContactsListModel::data(const QModelIndex &index, int role) const
    {
        // Only column 0 is valid.
        if (index.column() != 0) {
            return QVariant();
        }

        // Check what type of item we have here.
        AbstractTreeItem *abstractItem = static_cast<AbstractTreeItem*>(index.internalPointer());
        ContactItem *contactItem = dynamic_cast<ContactItem*>(abstractItem);

        if (contactItem) {

            QVariant data;

            switch(role)
            {
            case Qt::DisplayRole:
                data.setValue<QString>(contactItem->alias());
                break;
//            case Qt::DecorationRole:
//                data.setValue<QIcon>(contactItem->presenceIcon());
                break;
            case ContactsListModel::ContactRole:
                break;
            case ContactsListModel::IdRole:
                break;
            case ContactsListModel::PresenceStatusRole:
                break;
            case ContactsListModel::PresenceTypeRole:
                data.setValue<qint64>(contactItem->presenceType());
                break;
            case ContactsListModel::PresenceMessageRole:
                break;
            case ContactsListModel::SubscriptionStateRole:
                break;
            case ContactsListModel::PublishStateRole:
                break;
            case ContactsListModel::BlockedRole:
                break;
            case ContactsListModel::GroupsRole:
                data.setValue<QStringList>(contactItem->groups());
                break;
            case ContactsListModel::AvatarRole:
                data.setValue<QString>(contactItem->avatarData().fileName);
                break;
            default:
                break;
            }

            return data;
        }
        return QVariant();
    }

    Qt::ItemFlags ContactsListModel::flags(const QModelIndex &index) const
    {
        return QAbstractItemModel::flags(index);
    }


}

