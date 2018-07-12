#include "treeitem.h"

TreeItem::TreeItem(const QList<QVariant> &data, TreeItem *parent, int uid, int index)
    : itemData(data),
      parentItem(parent),
      index(index),
      uid(uid)
{
}

TreeItem::~TreeItem()
{
    qDeleteAll(childItems);
}

void TreeItem::appendChild(TreeItem *child)
{
    childItems.append(child);
}


TreeItem *TreeItem::child(int row)
{
    return childItems.value(row);
}

int TreeItem::childCount() const
{
    return childItems.count();
}

int TreeItem::columnCount() const
{
    return itemData.count();
}

QVariant TreeItem::data(int column) const
{
    return itemData.value(column);
}

int TreeItem::row() const
{
    if(parentItem)
        return parentItem->childItems.indexOf(const_cast<TreeItem*>(this));
    return 0;
}

TreeItem *TreeItem::parent()
{
    return parentItem;
}

void TreeItem::setIndex(int index)
{
    this->index = index;
}

int TreeItem::getIndex()
{
    return index;
}


void TreeItem::setData(int column, const QVariant &data)
{
    itemData[column] = data;
}

int TreeItem::getUid()
{
    return uid;
}


/*!
 * \brief TreeItem::removeChild
 * \param row       要删除的子节点的行数
 * \param recursive 是否递归删除其子节点
 * \return
 */
bool TreeItem::removeChild(int row, bool recursive)
{
    if(row < 0  || row >= childItems.size())
        return false;

    TreeItem *childItem = childItems[row];

    if(recursive) {
        childItem->cleanChildren();
        delete childItem;
        childItems.removeAt(row);
    } else {
        if(childItem->childCount() > 0) {
            childItem->setData(2, childItem->child(0)->data(2));
            childItem->setIndex(childItem->child(0)->getIndex());
            childItem->removeChild(0, false);
        } else {
            delete childItem;
            childItems.removeAt(row);
        }
    }

    return true;
}

/*!
 * \brief TreeItem::removeChildren
 * \param row       要删除的子节点的起始行数
 * \param count     要删除的子节点的数量
 * \param recursive 是否递归删除子节点
 * \return
 */
bool TreeItem::removeChildren(int row, int count, bool recursive)
{
    if(childItems.size() <= row || childItems.size() < row + count)
        return false;

    for(int i = row; i < row + count; i++) {
        removeChild(row, recursive);
    }
    return true;
}

/*!
 * \brief TreeItem::cleanChildren
 * 删除所有子节点
 */
void TreeItem::cleanChildren()
{
    for(auto item : childItems){
        item->cleanChildren();
        delete item;
    }
    childItems.clear();
}
