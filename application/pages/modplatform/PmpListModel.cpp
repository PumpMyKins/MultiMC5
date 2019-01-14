#include "FtbListModel.h"
#include "MultiMC.h"

#include <MMCStrings.h>
#include <Version.h>

#include <QtMath>
#include <QLabel>

#include <RWStorage.h>
#include <Env.h>

#include "net/URLConstants.h"

PmpFilterModel::PmpFilterModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    currentSorting = Sorting::ByGameVersion;
    sortings.insert(tr("Sort by name"), Sorting::ByName);
    sortings.insert(tr("Sort by game version"), Sorting::ByGameVersion);
}

bool PmpFilterModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    PmpModpack leftPack = sourceModel()->data(left, Qt::UserRole).value<PmpModpack>();
    PmpModpack rightPack = sourceModel()->data(right, Qt::UserRole).value<PmpModpack>();

    if(currentSorting == Sorting::ByGameVersion) {
        Version lv(leftPack.mcVersion);
        Version rv(rightPack.mcVersion);
        return lv < rv;

    } else if(currentSorting == Sorting::ByName) {
        return Strings::naturalCompare(leftPack.name, rightPack.name, Qt::CaseSensitive) >= 0;
    }

    //UHM, some inavlid value set?!
    qWarning() << "Invalid sorting set!";
    return true;
}

bool PmpFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    return true;
}

const QMap<QString, PmpFilterModel::Sorting> PmpFilterModel::getAvailableSortings()
{
    return sortings;
}

QString PmpFilterModel::translateCurrentSorting()
{
    return sortings.key(currentSorting);
}

void PmpFilterModel::setSorting(Sorting s)
{
    currentSorting = s;
    invalidate();
}

PmpFilterModel::Sorting PmpFilterModel::getCurrentSorting()
{
    return currentSorting;
}

PmpListModel::PmpListModel(QObject *parent) : QAbstractListModel(parent)
{
}

PmpListModel::~PmpListModel()
{
}

QString PmpListModel::translatePackType(PmpPackType type) const
{
    switch(type)
    {
        case PmpPackType::Public:
            return tr("Public Modpack");
        case PmpPackType::ThirdParty:
            return tr("Third Party Modpack");
        case PmpPackType::Private:
            return tr("Private Modpack");
    }
    qWarning() << "Unknown FTB modpack type:" << int(type);
    return QString();
}

int PmpListModel::rowCount(const QModelIndex &parent) const
{
    return modpacks.size();
}

int PmpListModel::columnCount(const QModelIndex &parent) const
{
    return 1;
}

QVariant PmpListModel::data(const QModelIndex &index, int role) const
{
    int pos = index.row();
    if(pos >= modpacks.size() || pos < 0 || !index.isValid())
    {
        return QString("INVALID INDEX %1").arg(pos);
    }

    PmpModpack pack = modpacks.at(pos);
    if(role == Qt::DisplayRole)
    {
        return pack.name + "\n" + translatePackType(pack.type);
    }
    else if (role == Qt::ToolTipRole)
    {
        if(pack.description.length() > 100)
        {
            //some magic to prevent to long tooltips and replace html linebreaks
            QString edit = pack.description.left(97);
            edit = edit.left(edit.lastIndexOf("<br>")).left(edit.lastIndexOf(" ")).append("...");
            return edit;

        }
        return pack.description;
    }
    else if(role == Qt::DecorationRole)
    {
        if(m_logoMap.contains(pack.logo))
        {
            return (m_logoMap.value(pack.logo));
        }
        QIcon icon = MMC->getThemedIcon("screenshot-placeholder");
        ((PmpListModel *)this)->requestLogo(pack.logo);
        return icon;
    }
    else if(role == Qt::TextColorRole)
    {
        if(pack.broken)
        {
            //FIXME: Hardcoded color
            return QColor(255, 0, 50);
        }
        else if(pack.bugged)
        {
            //FIXME: Hardcoded color
            //bugged pack, currently only indicates bugged xml
            return QColor(244, 229, 66);
        }
    }
    else if(role == Qt::UserRole)
    {
        QVariant v;
        v.setValue(pack);
        return v;
    }

    return QVariant();
}

void PmpListModel::fill(PmpModpackList modpacks)
{
    beginResetModel();
    this->modpacks = modpacks;
    endResetModel();
}

void PmpListModel::addPack(PmpModpack modpack)
{
    beginResetModel();
    this->modpacks.append(modpack);
    endResetModel();
}

void PmpListModel::clear()
{
    beginResetModel();
    modpacks.clear();
    endResetModel();
}

PmpModpack PmpListModel::at(int row)
{
    return modpacks.at(row);
}

void PmpListModel::remove(int row)
{
    if(row < 0 || row >= modpacks.size())
    {
        qWarning() << "Attempt to remove FTB modpacks with invalid row" << row;
        return;
    }
    beginRemoveRows(QModelIndex(), row, row);
    modpacks.removeAt(row);
    endRemoveRows();
}

void PmpListModel::logoLoaded(QString logo, QIcon out)
{
    m_loadingLogos.removeAll(logo);
    m_logoMap.insert(logo, out);
    emit dataChanged(createIndex(0, 0), createIndex(1, 0));
}

void PmpListModel::logoFailed(QString logo)
{
    m_failedLogos.append(logo);
    m_loadingLogos.removeAll(logo);
}

void PmpListModel::requestLogo(QString file)
{
    if(m_loadingLogos.contains(file) || m_failedLogos.contains(file))
    {
        return;
    }

    MetaEntryPtr entry = ENV.metacache()->resolveEntry("FTBPacks", QString("logos/%1").arg(file.section(".", 0, 0)));
    NetJob *job = new NetJob(QString("FTB Icon Download for %1").arg(file));
    job->addNetAction(Net::Download::makeCached(QUrl(QString(URLConstants::FTB_CDN_BASE_URL + "static/%1").arg(file)), entry));

    auto fullPath = entry->getFullPath();
    QObject::connect(job, &NetJob::finished, this, [this, file, fullPath]
    {
        emit logoLoaded(file, QIcon(fullPath));
        if(waitingCallbacks.contains(file))
        {
            waitingCallbacks.value(file)(fullPath);
        }
    });

    QObject::connect(job, &NetJob::failed, this, [this, file]
    {
        emit logoFailed(file);
    });

    job->start();

    m_loadingLogos.append(file);
}

void PmpListModel::getLogo(const QString &logo, LogoCallback callback)
{
    if(m_logoMap.contains(logo))
    {
        callback(ENV.metacache()->resolveEntry("FTBPacks", QString("logos/%1").arg(logo.section(".", 0, 0)))->getFullPath());
    }
    else
    {
        requestLogo(logo);
    }
}

Qt::ItemFlags PmpListModel::flags(const QModelIndex &index) const
{
    return QAbstractListModel::flags(index);
}
