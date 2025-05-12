/* QupZillKa (2021-2025) https://github.com/dualword/QupZillKa License:GNU GPL v3*/
/* ============================================================
* QupZilla - WebKit based browser
* Copyright (C) 2010-2014  David Rosca <nowrep@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
#include "browsinglibrary.h"
#include "ui_browsinglibrary.h"
#include "bookmarksimport/bookmarksimportdialog.h"
#include "bookmarksexport/bookmarksexportdialog.h"
#include "historymanager.h"
#include "bookmarksmanager.h"
#include "rssmanager.h"
#include "mainapplication.h"
#include "qztools.h"
#include "settings.h"

#include <QMenu>
#include <QCloseEvent>

BrowsingLibrary::BrowsingLibrary(BrowserWindow* window, QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::BrowsingLibrary)
    , m_historyManager(new HistoryManager(window))
    , m_bookmarksManager(new BookmarksManager(window))
    , m_rssManager(mApp->rssManager())
    , m_rssLoaded(false)
{
    ui->setupUi(this);

    Settings settings;
    settings.beginGroup("BrowsingLibrary");
    resize(settings.value("size", QSize(760, 470)).toSize());
    m_historyManager->restoreState(settings.value("historyState", QByteArray()).toByteArray());
    settings.endGroup();

    QzTools::centerWidgetOnScreen(this);

    QIcon historyIcon;
    historyIcon.addFile(QSL(":/icons/other/bighistory.svg"), QSize(), QIcon::Normal);
    historyIcon.addFile(QSL(":/icons/other/bighistory-selected.svg"), QSize(), QIcon::Selected);

    QIcon bookmarksIcon;
    bookmarksIcon.addFile(QSL(":/icons/other/bigstar.svg"), QSize(), QIcon::Normal);
    bookmarksIcon.addFile(QSL(":/icons/other/bigstar-selected.svg"), QSize(), QIcon::Selected);

    ui->tabs->AddTab(m_historyManager, historyIcon, tr("History"));
    ui->tabs->AddTab(m_bookmarksManager, bookmarksIcon, tr("Bookmarks"));
    ui->tabs->AddTab(m_rssManager, QIcon(":/icons/other/feed.png"), tr("RSS"));
    ui->tabs->SetMode(FancyTabWidget::Mode_LargeSidebar);
    ui->tabs->setFocus();

    QMenu* m = new QMenu(this);
    m->addAction(tr("Import Bookmarks..."), this, SLOT(importBookmarks()));
    m->addAction(tr("Export Bookmarks..."), this, SLOT(exportBookmarks()));
    ui->importExport->setMenu(m);

    connect(ui->tabs, &FancyTabWidget::CurrentChanged, ui->searchLine, &QLineEdit::clear);
    connect(ui->searchLine, SIGNAL(textChanged(QString)), this, SLOT(search()));
    connect(ui->tabs, QOverload<int>::of(&FancyTabWidget::CurrentChanged), [=](int i){
        ui->searchLine->setVisible(true);
        ui->importExport->setVisible(false);
        if(i==1) ui->importExport->setVisible(true);
        if(i==2) ui->searchLine->setVisible(false);
    });

    QzTools::setWmClass("Browsing Library", this);
}

void BrowsingLibrary::search()
{
    if (ui->tabs->current_index() == 0) {
        m_historyManager->search(ui->searchLine->text());
    }
    else {
        m_bookmarksManager->search(ui->searchLine->text());
    }
}

void BrowsingLibrary::importBookmarks()
{
    BookmarksImportDialog* d = new BookmarksImportDialog(this);
    d->open();
}

void BrowsingLibrary::exportBookmarks()
{
    BookmarksExportDialog* d = new BookmarksExportDialog(this);
    d->open();
}

void BrowsingLibrary::showHistory(BrowserWindow* window)
{
    ui->tabs->SetCurrentIndex(0);
    show();
    m_historyManager->setMainWindow(window);

    raise();
    activateWindow();
}

void BrowsingLibrary::showBookmarks(BrowserWindow* window)
{
    ui->tabs->SetCurrentIndex(1);
    show();
    m_bookmarksManager->setMainWindow(window);

    raise();
    activateWindow();
}

void BrowsingLibrary::showRSS(BrowserWindow* window)
{
    ui->tabs->SetCurrentIndex(2);
    show();
    m_rssManager->setMainWindow(window);

    if (!m_rssLoaded) {
        m_rssManager->refreshTree();
        m_rssLoaded = true;
    }

    raise();
    activateWindow();
}

void BrowsingLibrary::closeEvent(QCloseEvent* e)
{
    Settings settings;
    settings.beginGroup("BrowsingLibrary");
    settings.setValue("size", size());
    settings.setValue("historyState", m_historyManager->saveState());
    settings.endGroup();
    e->accept();
}

void BrowsingLibrary::keyPressEvent(QKeyEvent* e)
{
    if (e->key() == Qt::Key_Escape
        || (e->key() == Qt::Key_W && e->modifiers() == Qt::ControlModifier)) {
        close();
    }

    QWidget::keyPressEvent(e);
}

BrowsingLibrary::~BrowsingLibrary()
{
    delete ui;
}
