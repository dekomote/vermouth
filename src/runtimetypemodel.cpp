#include "runtimetypemodel.h"
#include "appentry.h"

RuntimeTypeModel::RuntimeTypeModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int RuntimeTypeModel::rowCount(const QModelIndex &) const
{
    return AppEntry::runtimeTypeCount;
}

QVariant RuntimeTypeModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (row < 0 || row >= AppEntry::runtimeTypeCount)
        return {};

    const auto &e = AppEntry::runtimeTypeTable[row];
    switch (role) {
    case KeyRole:
        return QString::fromLatin1(e.key);
    case LabelRole:
        return QString::fromLatin1(e.label);
    }
    return {};
}

QHash<int, QByteArray> RuntimeTypeModel::roleNames() const
{
    return {
        {KeyRole, "key"},
        {LabelRole, "label"},
    };
}

QVariantMap RuntimeTypeModel::get(int index) const
{
    QModelIndex idx = this->index(index, 0);
    if (!idx.isValid())
        return {};
    return {
        {QStringLiteral("key"), data(idx, KeyRole)},
        {QStringLiteral("label"), data(idx, LabelRole)},
    };
}
