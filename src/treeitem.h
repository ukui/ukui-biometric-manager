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
    TreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    TreeItem *parent();
    void setData(int column, const QVariant &data);
    void setIndex(int index);
    int getIndex();
    bool removeChild(int row, bool recursive = false);
    bool removeChildren(int row, int count, bool recursive = false);
    void cleanChildren();
    int getUid();

private:
    QList<QVariant> itemData;
    TreeItem *parentItem;
    QList<TreeItem*> childItems;
    int index;
    int uid;
};

#endif // TREEITEM_H
