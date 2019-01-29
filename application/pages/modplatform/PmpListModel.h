#pragma once

#include <modplatform/pmp/PackHelpers.h>
#include <RWStorage.h>

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QThreadPool>
#include <QIcon>
#include <QStyledItemDelegate>

#include <functional>

typedef QMap<QString, QIcon> PmpLogoMap;
typedef std::function<void(QString)> LogoCallback;

class PmpFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    PmpFilterModel(QObject* parent = Q_NULLPTR);
    enum Sorting {
        ByName,
        ByGameVersion
    };
    const QMap<QString, Sorting> getAvailableSortings();
    QString translateCurrentSorting();
    void setSorting(Sorting sorting);
    Sorting getCurrentSorting();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    QMap<QString, Sorting> sortings;
    Sorting currentSorting;

};

class PmpListModel : public QAbstractListModel
{
    Q_OBJECT
private:
    PmpModpackList modpacks;
    QStringList m_failedLogos;
    QStringList m_loadingLogos;
    PmpLogoMap m_logoMap;
    QMap<QString, LogoCallback> waitingCallbacks;

    void requestLogo(QString pack);
    QString translatePackType(PmpPackType type) const;


private slots:
    void logoFailed(QString logo);
    void logoLoaded(QString logo, QIcon out);

public:
    PmpListModel(QObject *parent);
    ~PmpListModel();
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void fill(PmpModpackList modpacks);
    void addPack(PmpModpack modpack);
    void clear();
    void remove(int row);

    PmpModpack at(int row);
    void getLogo(const QString &logo, LogoCallback callback);
};
