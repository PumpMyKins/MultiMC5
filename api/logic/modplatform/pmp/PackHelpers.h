#pragma once

#include <QList>
#include <QString>
#include <QStringList>
#include <QMetaType>

//Header for structs etc...
enum class PmpPackType
{
    Public,
    Beta
};

struct PmpModpack
{
    QString name;
    QString description;
    QString author;
    QString currentVersion;
    QString mcVersion;
    QString mods;
    QString logo;

    //Technical data
    QString dir;
    QString file; //<- Url in the xml, but doesn't make much sense

    bool bugged = false;
    bool broken = false;

    PmpPackType type;
    QString packCode;
};

//We need it for the proxy model
Q_DECLARE_METATYPE(PmpModpack)

typedef QList<PmpModpack> PmpModpackList;
