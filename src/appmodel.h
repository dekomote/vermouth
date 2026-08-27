#pragma once

#include "appentry.h"
#include <QAbstractListModel>
#include <QVector>

class AppModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool showHidden READ showHidden WRITE setShowHidden NOTIFY showHiddenChanged)
    Q_PROPERTY(QString sortField READ sortField WRITE setSortField NOTIFY sortFieldChanged)
    Q_PROPERTY(bool sortAscending READ sortAscending WRITE setSortAscending NOTIFY sortAscendingChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        ExePathRole,
        RuntimeTypeRole,
        ProtonPathRole,
        ProtonPrefixRole,
        WineBinaryRole,
        WinePrefixRole,
        IconPathRole,
        GridPathRole,
        HeroPathRole,
        LogoPathRole,
        SteamGridDbIdRole,
        SteamAppIdRole,
        PlatformSlugRole,
        CustomCorePathRole,
        UzdoomPathRole,
        UzdoomModsRole,
        LaunchOptionsRole,
        EnableLoggingRole,
        HiddenRole,
        PlayTimeRole,
        ProtonGameIdRole,
        EnableMangohudRole,
        EnableGamemodeRole,
        EnablePreferSdlRole,
        EnableLsfgRole,
        LsfgMultiplierRole,
        LsfgFlowScaleRole,
        LsfgPerformanceModeRole,
        LsfgPresentModeRole,
    };

    explicit AppModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const
    {
        return m_filtered.size();
    }

    Q_INVOKABLE void addApp(const QVariantMap &app);
    Q_INVOKABLE void removeApp(int index);
    Q_INVOKABLE void removeAndCleanApp(int index);
    Q_INVOKABLE void editApp(int index, const QVariantMap &app);
    Q_INVOKABLE QVariantMap getApp(int index) const;
    Q_INVOKABLE QVariantMap getAppById(const QString &id) const;
    Q_INVOKABLE QVariantMap getAppByExePath(const QString &exePath) const;
    Q_INVOKABLE bool hasSteamApp(int appId) const;
    Q_INVOKABLE void setFilterString(const QString &filter);
    Q_INVOKABLE QString generateUUID() const;
    Q_INVOKABLE void
    updateAppArt(const QString &id, const QString &iconPath, const QString &gridPath, const QString &heroPath, const QString &logoPath, int steamGridDbId = 0);
    Q_INVOKABLE void addPlayTime(const QString &exePath, qint64 seconds);

    bool showHidden() const
    {
        return m_showHidden;
    }
    void setShowHidden(bool showHidden);

    QString sortField() const
    {
        return m_sortField;
    }
    void setSortField(const QString &field);

    bool sortAscending() const
    {
        return m_sortAscending;
    }
    void setSortAscending(bool ascending);

    void load();
    void save() const;

Q_SIGNALS:
    void countChanged();
    void showHiddenChanged();
    void sortFieldChanged();
    void sortAscendingChanged();

private:
    int sourceIndex(int filteredIndex) const;
    void rebuildFilter();
    QString configPath() const;

    QVector<AppEntry> m_entries;
    QVector<int> m_filtered; // indices into m_entries
    QString m_filter;
    bool m_showHidden = false;
    QString m_sortField = QStringLiteral("name");
    bool m_sortAscending = true;
    int m_saveCounter = 0;
};
