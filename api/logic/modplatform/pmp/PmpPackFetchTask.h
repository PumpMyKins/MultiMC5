#pragma once

#include "net/NetJob.h"
#include <QTemporaryDir>
#include <QByteArray>
#include <QObject>
#include "PackHelpers.h"

class MULTIMC_LOGIC_EXPORT PmpPackFetchTask : public QObject {

    Q_OBJECT

public:
    PmpPackFetchTask() = default;
    virtual ~PmpPackFetchTask() = default;

    void fetch();
    void fetchPrivate(const QStringList &toFetch);

private:
    NetJobPtr jobPtr;

    QByteArray publicModpacksXmlFileData;
    QByteArray thirdPartyModpacksXmlFileData;

    bool parseAndAddPacks(QByteArray &data, PmpPackType packType, PmpModpackList &list);
    PmpModpackList publicPacks;
    PmpModpackList thirdPartyPacks;

protected slots:
    void fileDownloadFinished();
    void fileDownloadFailed(QString reason);

signals:
    void finished(PmpModpackList publicPacks, PmpModpackList thirdPartyPacks);
    void failed(QString reason);

    void privateFileDownloadFinished(PmpModpack modpack);
    void privateFileDownloadFailed(QString reason, QString packCode);
};
