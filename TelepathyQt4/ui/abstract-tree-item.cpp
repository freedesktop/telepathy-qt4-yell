/*
 * This file is part of TelepathyQt4
 *
 * Copyright (C) 2010 Collabora Ltd. <info@collabora.co.uk>
 *   @Author George Goldberg <george.goldberg@collabora.co.uk>
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

#include "abstract-tree-item.h"

using namespace Tp;

AbstractTreeItem::AbstractTreeItem()
 : m_parent(0)
{

}

AbstractTreeItem::~AbstractTreeItem()
{

}

QList<AbstractTreeItem*> AbstractTreeItem::childItems() const
{
    return m_children;
}

AbstractTreeItem *AbstractTreeItem::parentItem() const
{
    return m_parent;
}

void AbstractTreeItem::appendChildItem(AbstractTreeItem *child)
{
    m_children.append(child);
}

void AbstractTreeItem::removeChildItem(AbstractTreeItem *child)
{
    m_children.removeOne(child);
}

void AbstractTreeItem::setParentItem(AbstractTreeItem *parent)
{
    m_parent = parent;
}

//#include "abstract-tree-item.moc"
