#include "PMPacksPage.h"
#include "ui_PMPacksPage.h"

#include <QInputDialog>

#include "MultiMC.h"
#include "dialogs/CustomMessageBox.h"
#include "dialogs/NewInstanceDialog.h"
#include "modplatform/pmp/PmpPackFetchTask.h"
#include "modplatform/pmp/PmpPackInstallTask.h"
#include "PmpListModel.h"

PMPacksPage::PMPacksPage(NewInstanceDialog* dialog, QWidget *parent)
    : QWidget(parent), dialog(dialog), ui(new Ui::PMPacksPage)
{
    pmpFetchTask.reset(new PmpPackFetchTask());

    ui->setupUi(this);

    {
        publicFilterModel = new PmpFilterModel(this);
        publicListModel = new PmpListModel(this);
        publicFilterModel->setSourceModel(publicListModel);

        ui->publicPackList->setModel(publicFilterModel);
        ui->publicPackList->setSortingEnabled(true);
        ui->publicPackList->header()->hide();
        ui->publicPackList->setIndentation(0);
        ui->publicPackList->setIconSize(QSize(42, 42));

        for(int i = 0; i < publicFilterModel->getAvailableSortings().size(); i++)
        {
            ui->sortByBox->addItem(publicFilterModel->getAvailableSortings().keys().at(i));
        }

        ui->sortByBox->setCurrentText(publicFilterModel->translateCurrentSorting());
    }

    {
        betaFilterModel = new PmpFilterModel(this);
        betaListModel = new PmpListModel(this);
        betaFilterModel->setSourceModel(betaListModel);

        ui->thirdPartyPackList->setModel(betaListModel);
        ui->thirdPartyPackList->setSortingEnabled(true);
        ui->thirdPartyPackList->header()->hide();
        ui->thirdPartyPackList->setIndentation(0);
        ui->thirdPartyPackList->setIconSize(QSize(42, 42));

        betaFilterModel->setSorting(publicFilterModel->getCurrentSorting());
    }

    ui->versionSelectionBox->view()->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->versionSelectionBox->view()->parentWidget()->setMaximumHeight(300);

    connect(ui->sortByBox, &QComboBox::currentTextChanged, this, &PMPacksPage::onSortingSelectionChanged);
    connect(ui->versionSelectionBox, &QComboBox::currentTextChanged, this, &PMPacksPage::onVersionSelectionItemChanged);

    connect(ui->publicPackList->selectionModel(), &QItemSelectionModel::currentChanged, this, &PMPacksPage::onPublicPackSelectionChanged);
    connect(ui->thirdPartyPackList->selectionModel(), &QItemSelectionModel::currentChanged, this, &PMPacksPage::onBetaPackSelectionChanged);

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &PMPacksPage::onTabChanged);

    // ui->modpackInfo->setOpenExternalLinks(true);

    ui->publicPackList->selectionModel()->reset();
    ui->thirdPartyPackList->selectionModel()->reset();

    onTabChanged(ui->tabWidget->currentIndex());
}

PMPacksPage::~PMPacksPage()
{
    delete ui;
}

bool PMPacksPage::shouldDisplay() const
{
    return true;
}

void PMPacksPage::openedImpl()
{
    if(!initialized)
    {
        connect(pmpFetchTask.get(), &PmpPackFetchTask::finished, this, &PMPacksPage::pmpPackDataDownloadSuccessfully);
        connect(pmpFetchTask.get(), &PmpPackFetchTask::failed, this, &PMPacksPage::pmpPackDataDownloadFailed);

        pmpFetchTask->fetch();
        initialized = true;
    }
    suggestCurrent();
}

void PMPacksPage::suggestCurrent()
{
    if(isOpened)
    {
        if(!selected.broken)
        {
            dialog->setSuggestedPack(selected.name, new PmpPackInstallTask(selected, selectedVersion));
            QString editedLogoName;
            if(selected.logo.toLower().startsWith("ftb"))
            {
                editedLogoName = selected.logo;
            }
            else
            {
                editedLogoName = "ftb_" + selected.logo;
            }

            editedLogoName = editedLogoName.left(editedLogoName.lastIndexOf(".png"));

            if(selected.type == PmpPackType::Public)
            {
                publicListModel->getLogo(selected.logo, [this, editedLogoName](QString logo)
                {
                    dialog->setSuggestedIconFromFile(logo, editedLogoName);
                });
            }
            else if (selected.type == PmpPackType::Beta)
            {
                betaListModel->getLogo(selected.logo, [this, editedLogoName](QString logo)
                {
                    dialog->setSuggestedIconFromFile(logo, editedLogoName);
                });
            }
        }
        else
        {
            dialog->setSuggestedPack();
        }
    }
}

void PMPacksPage::pmpPackDataDownloadSuccessfully(PmpModpackList publicPacks, PmpModpackList betaPacks)
{
    publicListModel->fill(publicPacks);
    betaListModel->fill(betaPacks);
}

void PMPacksPage::pmpPackDataDownloadFailed(QString reason)
{
    //TODO: Display the error
}

void PMPacksPage::onPublicPackSelectionChanged(QModelIndex now, QModelIndex prev)
{
    if(!now.isValid())
    {
        onPackSelectionChanged();
        return;
    }
    PmpModpack selectedPack = publicFilterModel->data(now, Qt::UserRole).value<PmpModpack>();
    onPackSelectionChanged(&selectedPack);
}

void PMPacksPage::onBetaPackSelectionChanged(QModelIndex now, QModelIndex prev)
{
    if(!now.isValid())
    {
        onPackSelectionChanged();
        return;
    }
    PmpModpack selectedPack = betaFilterModel->data(now, Qt::UserRole).value<PmpModpack>();
    onPackSelectionChanged(&selectedPack);
}

void PMPacksPage::onPackSelectionChanged(PmpModpack* pack)
{
    ui->versionSelectionBox->clear();
    if(pack)
    {
        currentModpackInfo->setHtml("Pack by <b>" + pack->author + "</b>" +
                                    "<br>Minecraft " + pack->mcVersion + "<br>" + "<br>" + pack->description + "<ul><li>" + pack->mods.replace(";", "</li><li>")
                                    + "</li></ul>");

        ui->versionSelectionBox->addItem(pack->currentVersion);
        selected = *pack;
    }
    else
    {
        currentModpackInfo->setHtml("");
        ui->versionSelectionBox->clear();
        if(isOpened)
        {
            dialog->setSuggestedPack();
        }
        return;
    }
    suggestCurrent();
}

void PMPacksPage::onVersionSelectionItemChanged(QString data)
{
    if(data.isNull() || data.isEmpty())
    {
        selectedVersion = "";
        return;
    }

    selectedVersion = data;
    suggestCurrent();
}

void PMPacksPage::onSortingSelectionChanged(QString data)
{
    PmpFilterModel::Sorting toSet = publicFilterModel->getAvailableSortings().value(data);
    publicFilterModel->setSorting(toSet);
    betaFilterModel->setSorting(toSet);
}

void PMPacksPage::onTabChanged(int tab)
{
    if(tab == 1)
    {
        currentModel = betaFilterModel;
        currentList = ui->thirdPartyPackList;
        currentModpackInfo = ui->thirdPartyPackDescription;
    }
    else
    {
        currentModel = publicFilterModel;
        currentList = ui->publicPackList;
        currentModpackInfo = ui->publicPackDescription;
    }

    currentList->selectionModel()->reset();
    QModelIndex idx = currentList->currentIndex();
    if(idx.isValid())
    {
        auto pack = currentModel->data(idx, Qt::UserRole).value<PmpModpack>();
        onPackSelectionChanged(&pack);
    }
    else
    {
        onPackSelectionChanged();
    }
}
