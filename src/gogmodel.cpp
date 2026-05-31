#include "gogmodel.h"
#include "goglibrary.h"
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrent>

GogModel::GogModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int GogModel::rowCount(const QModelIndex &) const
{
    return m_entries.size();
}

QVariant GogModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_entries.size())
        return {};
    const auto &e = m_entries[index.row()];
    switch (role) {
    case GameIdRole:
        return e.gameId;
    case NameRole:
        return e.name;
    case ExePathRole:
        return e.exePath;
    case IconPathRole:
        return e.iconPath;
    case IsWindowsRole:
        return e.isWindows;
    }
    return {};
}

QHash<int, QByteArray> GogModel::roleNames() const
{
    return {
        {GameIdRole, "gameId"},
        {NameRole, "name"},
        {ExePathRole, "exePath"},
        {IconPathRole, "iconPath"},
        {IsWindowsRole, "isWindows"},
    };
}

bool GogModel::busy() const
{
    return m_busy;
}

void GogModel::scanFolder(const QString &path)
{
    if (m_busy)
        return;
    m_busy = true;
    Q_EMIT busyChanged();

    auto *watcher = new QFutureWatcher<QVector<GogEntry>>(this);
    connect(watcher, &QFutureWatcher<QVector<GogEntry>>::finished, this, [this, watcher] {
        beginResetModel();
        m_entries = watcher->result();
        endResetModel();
        watcher->deleteLater();
        m_busy = false;
        Q_EMIT busyChanged();
        Q_EMIT countChanged();
        Q_EMIT gamesFetched();
    });
    watcher->setFuture(QtConcurrent::run([path] {
        return GogLibrary::scan(path);
    }));
}

QVariantMap GogModel::getGame(int index) const
{
    if (index < 0 || index >= m_entries.size())
        return {};
    const auto &e = m_entries[index];
    return {
        {QStringLiteral("gameId"), e.gameId},
        {QStringLiteral("name"), e.name},
        {QStringLiteral("exePath"), e.exePath},
        {QStringLiteral("iconPath"), e.iconPath},
        {QStringLiteral("isWindows"), e.isWindows},
    };
}
