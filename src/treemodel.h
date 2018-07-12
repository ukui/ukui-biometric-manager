#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QAbstractItemModel>
#include "treeitem.h"
#include "customtype.h"

#define ADMIN_UID 1000

class TreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    TreeModel(int uid, BioType type, QObject *parent=nullptr);
    ~TreeModel();

    enum FeatureRoles{
        IndexRole = Qt::UserRole,
        UidRole
    };

    void setModelData(const QList<FeatureInfo*> &featureInfoList);
    void appendData(const FeatureInfo* featureInfo); 
    bool removeRow(int row, const QModelIndex &parent=QModelIndex(), bool recursive=false);
    void removeAll();
    void updateSerialNum();
    int freeIndex();
    void setupTestData();

public:
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent=QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent=QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QHash<int, QByteArray> roleNames() const;

private:
    TreeItem *rootItem;
    QMap<int, TreeItem*> parentItems;
    int uid_;
    BioType type_;
};

#endif // TREEMODEL_H
