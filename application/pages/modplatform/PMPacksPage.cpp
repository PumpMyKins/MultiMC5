#include "PMPacksPage.h"
#include "ui_PMPacksPage.h"

#include "MultiMC.h"

#include <meta/Index.h>
#include <meta/VersionList.h>
#include <dialogs/NewInstanceDialog.h>
#include <Filter.h>
#include <Env.h>
#include <InstanceCreationTask.h>
#include <QTabBar>

PMPacksPage::PMPacksPage(NewInstanceDialog *dialog, QWidget *parent)
    : QWidget(parent), dialog(dialog), ui(new Ui::PMPacksPage)
{
    ui->setupUi(this);
    ui->tabWidget->tabBar()->hide();
    connect(ui->versionList, &VersionSelectWidget::selectedVersionChanged, this, &PMPacksPage::setSelectedVersion);
    filterChanged();
    connect(ui->alphaFilter, &QCheckBox::stateChanged, this, &PMPacksPage::filterChanged);
    connect(ui->betaFilter, &QCheckBox::stateChanged, this, &PMPacksPage::filterChanged);
    connect(ui->snapshotFilter, &QCheckBox::stateChanged, this, &PMPacksPage::filterChanged);
    connect(ui->oldSnapshotFilter, &QCheckBox::stateChanged, this, &PMPacksPage::filterChanged);
    connect(ui->releaseFilter, &QCheckBox::stateChanged, this, &PMPacksPage::filterChanged);
    connect(ui->refreshBtn, &QPushButton::clicked, this, &PMPacksPage::refresh);
}

void PMPacksPage::openedImpl()
{
    if(!initialized)
    {
        auto vlist = ENV.metadataIndex()->get("net.minecraft");
        ui->versionList->initialize(vlist.get());
        initialized = true;
    }
    else
    {
        suggestCurrent();
    }
}

void PMPacksPage::refresh()
{
    ui->versionList->loadList();
}

void PMPacksPage::filterChanged()
{
    QStringList out;
    if(ui->alphaFilter->isChecked())
        out << "(old_alpha)";
    if(ui->betaFilter->isChecked())
        out << "(old_beta)";
    if(ui->snapshotFilter->isChecked())
        out << "(snapshot)";
    if(ui->oldSnapshotFilter->isChecked())
        out << "(old_snapshot)";
    if(ui->releaseFilter->isChecked())
        out << "(release)";
    auto regexp = out.join('|');
    ui->versionList->setFilter(BaseVersionList::TypeRole, new RegexpFilter(regexp, false));
}

PMPacksPage::~PMPacksPage()
{
    delete ui;
}

bool PMPacksPage::shouldDisplay() const
{
    return true;
}

BaseVersionPtr PMPacksPage::selectedVersion() const
{
    return m_selectedVersion;
}

void PMPacksPage::suggestCurrent()
{
    if(m_selectedVersion && isOpened)
    {
        dialog->setSuggestedPack(m_selectedVersion->descriptor(), new InstanceCreationTask(m_selectedVersion));
    }
}

void PMPacksPage::setSelectedVersion(BaseVersionPtr version)
{
    m_selectedVersion = version;
    suggestCurrent();
}
