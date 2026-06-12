#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QSet>
#include <QStringList>
#include <QVariantList>
#include <QVector>

class GogClient;
class GogCoverCache;

class GogLibraryModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
    enum Roles {
        GameIdRole = Qt::UserRole + 1,
        TitleRole,
        CoverUrlRole,
        LocalCoverRole,
        WorksOnWindowsRole,
        WorksOnLinuxRole,
        InstalledRole,
        ExePathRole,
        SizeTextRole,
    };

    explicit GogLibraryModel(QObject *parent = nullptr);

    void setClient(GogClient *client);
    void setCoverCache(GogCoverCache *cache);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    bool busy() const;
    QString statusText() const;

    Q_INVOKABLE void fetchLibrary(const QString &search = {}, int page = 1);
    Q_INVOKABLE void fetchNextPage(const QString &search);
    Q_INVOKABLE void clear();
    Q_INVOKABLE QVariantMap getGame(int index) const;

    Q_INVOKABLE void setInstalledMap(const QVariantMap &map);
    Q_INVOKABLE void markInstalled(const QString &gameId, const QString &exePath);
    Q_INVOKABLE void revalidateInstalled();

    void notifyCoverCached(const QString &gameId, const QString &localPath);

Q_SIGNALS:
    void countChanged();
    void busyChanged();
    void statusTextChanged();
    void libraryUpdated(bool hasMore);
    void installedRemoved(const QString &gameId);
    void error(const QString &message);

private:
    struct GameEntry {
        QString id;
        QString title;
        QString coverUrl;
        QString localCover;
        bool worksOnWindows = false;
        bool worksOnLinux = false;
        double sizeBytes = -1;
    };

    bool isInstalled(const QString &id) const;
    int indexOfGame(const QString &id) const;
    void onLibraryFetched(const QVariantList &items, int totalPages, int page);
    void onSizeFetched(const QString &gameId, double bytes);
    void setBusy(bool b);
    void setStatusText(const QString &s);
    void requestCovers();
    void requestSizes();
    void pumpSizes();

    static constexpr int kMaxConcurrentSizes = 5;

    GogClient *m_client = nullptr;
    GogCoverCache *m_coverCache = nullptr;
    QVector<GameEntry> m_entries;
    QHash<QString, QString> m_installed;
    QStringList m_sizeQueue;
    QSet<QString> m_sizeInFlight;
    int m_currentPage = 1;
    int m_totalPages = 1;
    bool m_busy = false;
    QString m_statusText;
};
