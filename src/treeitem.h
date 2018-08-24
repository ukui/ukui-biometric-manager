#ifndef TREEITEM_H
#define TREEITEM_H

#include <QList>
#include <QVariant>

class TreeItem
{
public:
    TreeItem(const QList<QVariant> &data, TreeItem *parent=nullptr,
             int uid = -1, int index = 0);
    ~TreeItem();

    void appendChild(TreeItem *child);
    void insertChild(int pos,TreeItem *child);
    TreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    int row() const;
    TreeItem *parent();
    void setParent(TreeItem *parent);
    QVariant data(int column) const;
    void setData(int column, const QVariant &data);
    int getIndex();
    void setIndex(int index);
    bool removeChild(int row, bool recursive = false);
    bool removeChildren(int row, int count, bool recursive = false);
    void removeChildrenNoDelete();
    void cleanChildren();
    void setUid(int uid);
    int getUid();

private:
    QList<QVariant> itemData;
    TreeItem *parentItem;
    QList<TreeItem*> childItems;
    int index;
    int uid;
};

#endif // TREEITEM_H
