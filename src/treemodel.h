/*
 * Copyright (C) 2018 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 * 
**/
#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QAbstractItemModel>
#include "treeitem.h"
#include "customtype.h"

class TreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    TreeModel(int uid, BioType type, QObject *parent=nullptr);
    ~TreeModel();

    enum FeatureRoles{
        IndexRole = Qt::UserRole,
        UidRole,
        NameRole
    };

    void setModelData(const QList<FeatureInfo*> &featureInfoList);
    void appendData(const FeatureInfo* featureInfo);
    void insertData(const FeatureInfo *featureInfo);
    int findInsertPosition(const FeatureInfo* featureInfo, TreeItem *parentItem);
    bool removeRow(int row, const QModelIndex &parent=QModelIndex(), bool recursive=false);
    void removeAll();
    void updateSerialNum();
    int freeIndex();
    void setupTestData();
    bool hasFeature(int uid, const QString &featureName);
    TreeItem *createItem(int serialNum, const FeatureInfo *featureInfo, int type);

public:
    int columnCount(const QModelIndex &parent) const;
    int rowCount(const QModelIndex &parent=QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role) const;
    QModelIndex index(int row, int column, const QModelIndex &parent=QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QHash<int, QByteArray> roleNames() const;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

private:
    enum ItemType{NORMAL, ADMIN_PARENT, ADMIN_CHILD};
    TreeItem *rootItem;
    QMap<int, TreeItem*> parentItems;
    int uid_;   //当前用户id
    BioType type_;
};

#endif // TREEMODEL_H
