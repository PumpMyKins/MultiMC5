#include "PmpPackFetchTask.h"
#include <QDomDocument>
#include "PmpPrivatePackManager.h"

#include "net/URLConstants.h"

void PmpPackFetchTask::fetch()
{
    publicPacks.clear();
    thirdPartyPacks.clear();

    NetJob *netJob = new NetJob("PmpModpackFetch");

    QUrl publicPacksUrl = QUrl(URLConstants::FTB_CDN_BASE_URL + "public/modpacks.xml");
    qDebug() << "Downloading public version info from" << publicPacksUrl.toString();
    netJob->addNetAction(Net::Download::makeByteArray(publicPacksUrl, &publicModpacksXmlFileData));

    QUrl thirdPartyUrl = QUrl(URLConstants::FTB_CDN_BASE_URL + "beta/modpacks.xml");
    qDebug() << "Downloading thirdparty version info from" << thirdPartyUrl.toString();
    netJob->addNetAction(Net::Download::makeByteArray(thirdPartyUrl, &thirdPartyModpacksXmlFileData));

    QObject::connect(netJob, &NetJob::succeeded, this, &PmpPackFetchTask::fileDownloadFinished);
    QObject::connect(netJob, &NetJob::failed, this, &PmpPackFetchTask::fileDownloadFailed);

    jobPtr.reset(netJob);
    netJob->start();
}

/*void PmpPackFetchTask::fetchPrivate(const QStringList & toFetch)
{
    QString privatePackBaseUrl = URLConstants::FTB_CDN_BASE_URL + "static/%1.xml";

    for (auto &packCode: toFetch)
    {
        QByteArray *data = new QByteArray();
        NetJob *job = new NetJob("Fetching private pack");
        job->addNetAction(Net::Download::makeByteArray(privatePackBaseUrl.arg(packCode), data));

        QObject::connect(job, &NetJob::succeeded, this, [this, job, data, packCode]
        {
            PmpModpackList packs;
            parseAndAddPacks(*data, PmpPackType::Private, packs);
            foreach(PmpModpack currentPack, packs)
            {
                currentPack.packCode = packCode;
                emit privateFileDownloadFinished(currentPack);
            }

            job->deleteLater();

            data->clear();
            delete data;
        });

        QObject::connect(job, &NetJob::failed, this, [this, job, packCode, data](QString reason)
        {
            emit privateFileDownloadFailed(reason, packCode);
            job->deleteLater();

            data->clear();
            delete data;
        });

        job->start();
    }
}
*/
void PmpPackFetchTask::fileDownloadFinished()
{
    jobPtr.reset();

    QStringList failedLists;

    if(!parseAndAddPacks(publicModpacksXmlFileData, PmpPackType::Public, publicPacks))
    {
        failedLists.append(tr("Public Packs"));
    }

    if(!parseAndAddPacks(thirdPartyModpacksXmlFileData, PmpPackType::Beta, thirdPartyPacks))
    {
        failedLists.append(tr("Beta Packs"));
    }

    if(failedLists.size() > 0)
    {
        emit failed(tr("Failed to download some pack lists: %1").arg(failedLists.join("\n- ")));
    }
    else
    {
        emit finished(publicPacks, thirdPartyPacks);
    }
}

bool PmpPackFetchTask::parseAndAddPacks(QByteArray &data, PmpPackType packType, PmpModpackList &list)
{
    QDomDocument doc;

    QString errorMsg = "Unknown error.";
    int errorLine = -1;
    int errorCol = -1;

    if(!doc.setContent(data, false, &errorMsg, &errorLine, &errorCol))
    {
        auto fullErrMsg = QString("Failed to fetch modpack data: %1 %2:3d!").arg(errorMsg, errorLine, errorCol);
        qWarning() << fullErrMsg;
        data.clear();
        return false;
    }

    QDomNodeList nodes = doc.elementsByTagName("modpack");

    for(int i = 0; i < nodes.length(); i++)
    {
        QDomElement element = nodes.at(i).toElement();

        PmpModpack modpack;
        modpack.name = element.attribute("name");
        modpack.currentVersion = element.attribute("version");
        modpack.mcVersion = element.attribute("mcVersion");
        modpack.description = element.attribute("description");
        modpack.mods = "";
        modpack.logo = element.attribute("logo");
        modpack.broken = false;
        modpack.bugged = false;

        if(!modpack.currentVersion.isNull() && !modpack.currentVersion.isEmpty())
        {
             qWarning() << "Added current version to oldVersions because oldVersions was empty! (" + modpack.name + ")";
         }
         else
         {
             modpack.broken = true;
             qWarning() << "Broken pack:" << modpack.name << " => No valid version!";
         }


        modpack.author = element.attribute("author");

        modpack.dir = element.attribute("dir");
        modpack.file = element.attribute("url");

        modpack.type = packType;

        list.append(modpack);
    }

    return true;
}

void PmpPackFetchTask::fileDownloadFailed(QString reason)
{
    qWarning() << "Fetching PmpPacks failed:" << reason;
    emit failed(reason);
}
