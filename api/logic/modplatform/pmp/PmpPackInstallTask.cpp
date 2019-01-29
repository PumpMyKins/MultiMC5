#include "PmpPackInstallTask.h"
#include "Env.h"
#include "MMCZip.h"
#include "QtConcurrent"
#include "BaseInstance.h"
#include "FileSystem.h"
#include "settings/INISettingsObject.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/ComponentList.h"
#include "minecraft/GradleSpecifier.h"

#include "net/URLConstants.h"

PmpPackInstallTask::PmpPackInstallTask(PmpModpack pack, QString version)
{
    m_pack = pack;
    m_version = version;
}

void PmpPackInstallTask::executeTask()
{
    downloadPack();
}

void PmpPackInstallTask::downloadPack()
{
    setStatus(tr("Downloading zip for %1").arg(m_pack.name));

    auto packoffset = QString("%1/%2/%3").arg(m_pack.dir, m_version.replace(".", "_"), m_pack.name);
    auto entry = ENV.metacache()->resolveEntry("FTBPacks", packoffset);
    NetJob *job = new NetJob("Download FTB Pack");

    entry->setStale(true);
    QString url = QString(m_pack.file);

    job->addNetAction(Net::Download::makeCached(url, entry));
    archivePath = entry->getFullPath();

    netJobContainer.reset(job);
    connect(job, &NetJob::succeeded, this, &PmpPackInstallTask::onDownloadSucceeded);
    connect(job, &NetJob::failed, this, &PmpPackInstallTask::onDownloadFailed);
    connect(job, &NetJob::progress, this, &PmpPackInstallTask::onDownloadProgress);
    job->start();

    progress(1, 4);
}

void PmpPackInstallTask::onDownloadSucceeded()
{
    abortable = false;
    unzip();
}

void PmpPackInstallTask::onDownloadFailed(QString reason)
{
    abortable = false;
    emitFailed(reason);
}

void PmpPackInstallTask::onDownloadProgress(qint64 current, qint64 total)
{
    abortable = true;
    progress(current, total * 4);
    setStatus(tr("Downloading zip for %1 (%2%)").arg(m_pack.name).arg(current / 10));
}

void PmpPackInstallTask::unzip()
{
    progress(2, 4);
    setStatus(tr("Extracting modpack"));
    QDir extractDir(m_stagingPath);

    m_packZip.reset(new QuaZip(archivePath));
    if(!m_packZip->open(QuaZip::mdUnzip))
    {
        emitFailed(tr("Failed to open modpack file %1!").arg(archivePath));
        return;
    }

    m_extractFuture = QtConcurrent::run(QThreadPool::globalInstance(), MMCZip::extractDir, archivePath, extractDir.absolutePath() + "/unzip");
    connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::finished, this, &PmpPackInstallTask::onUnzipFinished);
    connect(&m_extractFutureWatcher, &QFutureWatcher<QStringList>::canceled, this, &PmpPackInstallTask::onUnzipCanceled);
    m_extractFutureWatcher.setFuture(m_extractFuture);
}

void PmpPackInstallTask::onUnzipFinished()
{
    install();
}

void PmpPackInstallTask::onUnzipCanceled()
{
    emitAborted();
}

void PmpPackInstallTask::install()
{
    progress(3, 4);
    setStatus(tr("Installing modpack"));
    QDir unzipMcDir(m_stagingPath + "/unzip/minecraft");
    if(unzipMcDir.exists())
    {
        //ok, found minecraft dir, move contents to instance dir
        if(!QDir().rename(m_stagingPath + "/unzip/minecraft", m_stagingPath + "/.minecraft"))
        {
            emitFailed(tr("Failed to move unzipped minecraft!"));
            return;
        }
    }

    QString instanceConfigPath = FS::PathCombine(m_stagingPath, "instance.cfg");
    auto instanceSettings = std::make_shared<INISettingsObject>(instanceConfigPath);
    instanceSettings->registerSetting("InstanceType", "Legacy");
    instanceSettings->set("InstanceType", "OneSix");

    MinecraftInstance instance(m_globalSettings, instanceSettings, m_stagingPath);
    auto components = instance.getComponentList();
    components->buildingFromScratch();
    components->setComponentVersion("net.minecraft", m_pack.mcVersion, true);

    bool fallback = true;

    //handle different versions
    QFile packJson(m_stagingPath + "/.minecraft/pack.json");
    QDir jarmodDir = QDir(m_stagingPath + "/unzip/instMods");
    if(packJson.exists())
    {
        packJson.open(QIODevice::ReadOnly | QIODevice::Text);
        QJsonDocument doc = QJsonDocument::fromJson(packJson.readAll());
        packJson.close();

        //we only care about the libs
        QJsonArray libs = doc.object().value("libraries").toArray();

        foreach (const QJsonValue &value, libs)
        {
            QString nameValue = value.toObject().value("name").toString();
            if(!nameValue.startsWith("net.minecraftforge"))
            {
                continue;
            }

            GradleSpecifier forgeVersion(nameValue);

            components->setComponentVersion("net.minecraftforge", forgeVersion.version().replace(m_pack.mcVersion, "").replace("-", ""));
            packJson.remove();
            fallback = false;
            break;
        }

    }

    if(jarmodDir.exists())
    {
        qDebug() << "Found jarmods, installing...";

        QStringList jarmods;
        for (auto info: jarmodDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files))
        {
            qDebug() << "Jarmod:" << info.fileName();
            jarmods.push_back(info.absoluteFilePath());
        }

        components->installJarMods(jarmods);
        fallback = false;
    }

    //just nuke unzip directory, it s not needed anymore
    FS::deletePath(m_stagingPath + "/unzip");

    if(fallback)
    {
        //TODO: Some fallback mechanism... or just keep failing!
        emitFailed(tr("No installation method found!"));
        return;
    }

    components->saveNow();

    progress(4, 4);

    instance.setName(m_instName);
    if(m_instIcon == "default")
    {
        m_instIcon = "ftb_logo";
    }
    instance.setIconKey(m_instIcon);
    instanceSettings->resumeSave();

    emitSucceeded();
}

bool PmpPackInstallTask::abort()
{
    if(abortable)
    {
        return netJobContainer->abort();
    }
    return false;
}
