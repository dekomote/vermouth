#pragma once

#include <QAbstractListModel>
#include <QVector>
#include "appentry.h"

class AppModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        ExePathRole,
        RuntimeTypeRole,
        ProtonPathRole,
        ProtonPrefixRole,
        WineBinaryRole,
        WinePrefixRole,
        IconPathRole,
        LaunchOptionsRole,
        EnableLoggingRole,
    };

    explicit AppModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return m_filtered.size(); }

    Q_INVOKABLE void addApp(const QString &name, const QString &exePath,
                            const QString &runtimeType,
                            const QString &protonPath, const QString &protonPrefix,
                            const QString &wineBinary, const QString &winePrefix,
                            const QString &iconPath,
                            const QString &launchOptions = QString(),
                            bool enableLogging = false);
    Q_INVOKABLE void removeApp(int index);
    Q_INVOKABLE void removeAndCleanApp(int index);
    Q_INVOKABLE void editApp(int index,
                             const QString &name, const QString &exePath,
                             const QString &runtimeType,
                             const QString &protonPath, const QString &protonPrefix,
                             const QString &wineBinary, const QString &winePrefix,
                             const QString &iconPath,
                             const QString &launchOptions = QString(),
                             bool enableLogging = false);
    Q_INVOKABLE QVariantMap getApp(int index) const;
    Q_INVOKABLE void setFilterString(const QString &filter);

    void load();
    void save() const;

Q_SIGNALS:
    void countChanged();

private:
    int sourceIndex(int filteredIndex) const;
    void rebuildFilter();
    QString configPath() const;

    QVector<AppEntry> m_entries;
    QVector<int> m_filtered; // indices into m_entries
    QString m_filter;
};
