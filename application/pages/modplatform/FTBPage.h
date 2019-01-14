/* Copyright 2013-2018 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <QWidget>
#include <QTreeView>
#include <QTextBrowser>

#include "pages/BasePage.h"
#include <MultiMC.h>
#include "tasks/Task.h"
#include "modplatform/ftb/PackHelpers.h"
#include "modplatform/ftb/FtbPackFetchTask.h"
#include "QObjectPtr.h"

namespace Ui
{
class FTBPage;
}

class PmpListModel;
class PmpFilterModel;
class NewInstanceDialog;
class PmpPrivatePackListModel;
class PmpPrivatePackFilterModel;
class PmpPrivatePackManager;

class FTBPage : public QWidget, public BasePage
{
    Q_OBJECT

public:
    explicit FTBPage(NewInstanceDialog * dialog, QWidget *parent = 0);
    virtual ~FTBPage();
    QString displayName() const override
    {
        return tr("FTB Legacy");
    }
    QIcon icon() const override
    {
        return MMC->getThemedIcon("ftb_logo");
    }
    QString id() const override
    {
        return "ftb";
    }
    QString helpPage() const override
    {
        return "FTB-platform";
    }
    bool shouldDisplay() const override;
    void openedImpl() override;

private:
    void suggestCurrent();
    void onPackSelectionChanged(PmpModpack *pack = nullptr);

private slots:
    void ftbPackDataDownloadSuccessfully(PmpModpackList publicPacks, PmpModpackList thirdPartyPacks);
    void ftbPackDataDownloadFailed(QString reason);

    void ftbPrivatePackDataDownloadSuccessfully(PmpModpack pack);
    void ftbPrivatePackDataDownloadFailed(QString reason, QString packCode);

    void onSortingSelectionChanged(QString data);
    void onVersionSelectionItemChanged(QString data);

    void onPublicPackSelectionChanged(QModelIndex first, QModelIndex second);
    void onThirdPartyPackSelectionChanged(QModelIndex first, QModelIndex second);
    void onPrivatePackSelectionChanged(QModelIndex first, QModelIndex second);

    void onTabChanged(int tab);

    void onAddPackClicked();
    void onRemovePackClicked();

private:
    PmpFilterModel* currentModel = nullptr;
    QTreeView* currentList = nullptr;
    QTextBrowser* currentModpackInfo = nullptr;

    bool initialized = false;
    PmpModpack selected;
    QString selectedVersion;

    PmpListModel* publicListModel = nullptr;
    PmpFilterModel* publicFilterModel = nullptr;

    PmpListModel *thirdPartyModel = nullptr;
    PmpFilterModel *thirdPartyFilterModel = nullptr;

    PmpListModel *privateListModel = nullptr;
    PmpFilterModel *privateFilterModel = nullptr;

    unique_qobject_ptr<PmpPackFetchTask> ftbFetchTask;
    std::unique_ptr<PmpPrivatePackManager> ftbPrivatePacks;

    NewInstanceDialog* dialog = nullptr;

    Ui::FTBPage *ui = nullptr;
};
