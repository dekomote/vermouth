#pragma once

#include <QAbstractListModel>
#include <QVariantList>
#include <QVector>

class EpicClient;
class GogCoverCache;

class EpicLibraryModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY countChanged)

public:
    enum Roles {
        AppNameRole = Qt::UserRole + 1,
        TitleRole,
        CoverUrlRole,
        LocalCoverRole,
        InstalledRole,
        InstallPathRole,
        ExePathRole,
        UpdateAvailableRole,
    };

    explicit EpicLibraryModel(QObject *parent = nullptr);

    void setClient(EpicClient *client);
    void setCoverCache(GogCoverCache *cache);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int totalCount() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void setFilter(const QString &text);
    Q_INVOKABLE void clear();
    Q_INVOKABLE QVariantMap getGame(int index) const;
    Q_INVOKABLE void markInstalled(const QString &appName, const QString &installPath);
    Q_INVOKABLE void markUninstalled(const QString &appName);
    void setOutdated(const QStringList &appNames);

    void notifyCoverCached(const QString &appName, const QString &localPath);

Q_SIGNALS:
    void countChanged();
    void libraryUpdated();

private:
    struct Game {
        QString appName;
        QString title;
        QString coverUrl;
        QString localCover;
        QString installPath;
        QString exePath;
        bool outdated = false;
    };

    void onLibraryFetched(const QVariantList &games, const QVariantList &installed);
    void rebuild();
    int viewRow(const QString &appName) const;

    EpicClient *m_client = nullptr;
    GogCoverCache *m_coverCache = nullptr;
    QVector<Game> m_all;
    QVector<int> m_view;
    QString m_filter;
};
