/* QupZillKa (2021-2026) https://github.com/dualword/QupZillKa License:GNU GPL v3*/
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

#include "rssmanager.h"
#include "ui_rssmanager.h"
#include "browserwindow.h"
#include "statusbar.h"
#include "treewidget.h"
#include "iconprovider.h"
#include "browsinglibrary.h"
#include "qztools.h"

#include <QMenu>
#include <QLabel>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QMessageBox>
#include <QBuffer>
#include <QSqlQuery>
#include <QToolBar>
#include <QWebEngineSettings>
#include <QWebEngineHistory>
#include <QClipboard>

RSSManager::RSSManager(BrowserWindow* window, QWidget* p) : QWidget(p)
    , ui(new Ui::RSSManager)
    , m_window(window)
{
    ui->setupUi(this);

#ifdef Q_OS_MAC
    ui->tabWidget->setDocumentMode(false);
#endif

    m_networkManager = mApp->networkManager();
    ui->view1->setPage(new WPage(mApp->webProfile(), ui->view1));
    ui->view1->settings()->setAttribute(QWebEngineSettings::AutoLoadImages, false);
    ui->view1->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, false);
    ui->view1->page()->action(QWebEnginePage::QWebEnginePage::Back)->setVisible(false);
    ui->view1->page()->action(QWebEnginePage::QWebEnginePage::Back)->disconnect();
    ui->view1->page()->action(QWebEnginePage::QWebEnginePage::Forward)->setVisible(false);
    ui->view1->page()->action(QWebEnginePage::QWebEnginePage::Forward)->disconnect();
    ui->view1->page()->action(QWebEnginePage::QWebEnginePage::Reload)->setVisible(false);
    ui->view1->page()->action(QWebEnginePage::QWebEnginePage::Reload)->disconnect();
    ui->view1->page()->action(QWebEnginePage::QWebEnginePage::ViewSource)->setVisible(false);
    ui->view1->page()->action(QWebEnginePage::QWebEnginePage::ViewSource)->disconnect();
    ui->view1->page()->action(QWebEnginePage::QWebEnginePage::OpenLinkInNewTab)->setVisible(false);
    ui->view1->page()->action(QWebEnginePage::QWebEnginePage::OpenLinkInNewTab)->disconnect();
    ui->view1->page()->action(QWebEnginePage::QWebEnginePage::OpenLinkInNewWindow)->setVisible(false);
    ui->view1->page()->action(QWebEnginePage::QWebEnginePage::OpenLinkInNewWindow)->disconnect();

    auto tb = new QToolBar(this);
    QToolButton* btnAddFolder = new QToolButton(tb);
    btnAddFolder->setAutoRaise(true);
    btnAddFolder->setToolTip(tr("Add New Folder"));
    btnAddFolder->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirIcon));
    connect(btnAddFolder, SIGNAL(clicked()), this, SLOT(addFolder()));

    m_reloadButton = new QToolButton(tb);
    m_reloadButton->setAutoRaise(true);
    m_reloadButton->setToolTip(tr("Update All Feeds"));
    m_reloadButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
    connect(m_reloadButton, SIGNAL(clicked()), this, SLOT(reloadFeeds()));

    auto btn = new QToolButton(tb);
    btn->setAutoRaise(true);
    btn->setToolTip(tr("Delete All News"));
    btn->setIcon(QApplication::style()->standardIcon(QStyle::SP_TrashIcon));
    connect(btn, &QToolButton::clicked, [this] {
        QMessageBox::StandardButton btn = QMessageBox::warning(this, tr("Confirmation"),
            tr("Are you sure you want to delete all news?"), QMessageBox::Yes | QMessageBox::No);
        if (btn != QMessageBox::Yes) return;
        QSqlQuery query(db);
        query.prepare("DELETE FROM item");
        query.exec();
        ui->tree1->itemClicked(ui->tree1->topLevelItem(0),0);

    });

    btnImage = new QToolButton(tb);
    btnImage->setAutoRaise(true);
    btnImage->setToolTip(tr("Autoload images"));
    btnImage->setCheckable(true);
    btnImage->setIcon(QApplication::style()->standardIcon(QStyle::SP_DesktopIcon));
    connect(btnImage, &QToolButton::clicked, [this] {
        ui->view1->settings()->setAttribute(QWebEngineSettings::AutoLoadImages, !ui->view1->settings()->testAttribute(QWebEngineSettings::AutoLoadImages));
        btnImage->setChecked(ui->view1->settings()->testAttribute(QWebEngineSettings::AutoLoadImages));
        ui->view1->history()->clear();
        ui->view1->reload();
    });

    txt = new QLineEdit(this);
    txt->setReadOnly(true);
    txt->setAlignment(Qt::AlignHCenter);
    txt->setFixedWidth(100);

    tb->addWidget(btnAddFolder);
    tb->addSeparator();
    tb->addWidget(m_reloadButton);
    tb->addSeparator();
    tb->addWidget(btn);
    tb->addSeparator();
    tb->addWidget(btnImage);
    tb->addSeparator();
    tb->addWidget(txt);
    layout()->setMenuBar(tb);

    ui->tree1->setHeaderLabels({"Folders"});
    ui->tree1->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tree1->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    connect(ui->tree1, QOverload<const QPoint&>::of(&QTreeWidget::customContextMenuRequested),
            [this](const QPoint& pos){
        QMenu menu(this);
        QTreeWidgetItem *item = ui->tree1->itemAt( pos );
        QAction *newAct;

        if(item == ui->tree1->topLevelItem(0)){
            newAct = new QAction(tr("Add new folder"), this);
            connect(newAct, SIGNAL(triggered()), this, SLOT(addFolder()));
            menu.addAction(newAct);
            newAct = new QAction(tr("Update All Feeds"), this);
            connect(newAct, SIGNAL(triggered()), this, SLOT(reloadFeeds()));
            menu.addAction(newAct);
            menu.addSeparator();
            newAct = new QAction(tr("Delete All News"), this);
            connect(newAct, QOverload<bool>::of(&QAction::triggered), [=](bool b){
                Q_UNUSED(b)
                QMessageBox::StandardButton btn = QMessageBox::warning(this, tr("Confirmation"),
                    tr("Are you sure you want to delete all news?"), QMessageBox::Yes | QMessageBox::No);
                if (btn != QMessageBox::Yes) return;
                QSqlQuery query(db);
                query.prepare("DELETE FROM item");
                query.exec();
                ui->tree1->itemClicked(ui->tree1->topLevelItem(0),0);

            });
            menu.addAction(newAct);
        } else if(item->parent() == ui->tree1->topLevelItem(0)){
            newAct = new QAction(tr("Add New RSS Feed"), this);
            connect(newAct, QOverload<bool>::of(&QAction::triggered), [=](bool b){
                Q_UNUSED(b)
                QUrl url = QUrl(QInputDialog::getText(this, tr("Add new feed"), tr("Please enter URL of new feed:")));
                if (url.isEmpty() || !url.isValid()) return;

                QSqlQuery query(db);
                query.prepare("INSERT INTO feed (fid, url, active) VALUES(?,?,?)");
                query.bindValue(0, item->data(0, rId));
                query.bindValue(1, url);
                query.bindValue(2, 1);
                query.exec();

                QTreeWidgetItem* tmp = new QTreeWidgetItem();
                tmp->setText(0, url.toString());
                tmp->setData(0, rId, query.lastInsertId().toInt());
                tmp->setToolTip(0, url.toString());
                tmp->setIcon(0,QIcon(QPixmap(":/icons/other/feed.png")));
                item->addChild(tmp);
                ui->tree1->itemClicked(tmp,0);

            });

            menu.addAction(newAct);
            newAct = new QAction(tr("Update Feeds"), this);
            connect(newAct, QOverload<bool>::of(&QAction::triggered), [=]{
                reloadFeeds(item->data(0, rId).toInt());
            });
            menu.addAction(newAct);

            newAct = new QAction(tr("Delete News in this Folder"), this);
            connect(newAct, QOverload<bool>::of(&QAction::triggered), [=](bool b){
                Q_UNUSED(b)
                QMessageBox::StandardButton btn = QMessageBox::warning(this, tr("Confirmation"),
                    tr("Are you sure you want to delete news?"), QMessageBox::Yes | QMessageBox::No);
                if (btn != QMessageBox::Yes) return;

                QSqlQuery query(db);
                query.prepare("DELETE FROM item WHERE fid in (select id from feed where fid=?)");
                query.addBindValue(item->data(0, rId));
                query.exec();
                ui->tree1->itemClicked(item,0);
            });
            menu.addAction(newAct);

            newAct = new QAction(tr("Toggle Enable/Disable Updates"), this);
            connect(newAct, QOverload<bool>::of(&QAction::triggered), [&](bool b){
                Q_UNUSED(b)
                bool tmp = !selectValue(db, "active", "feed", "fid", item->data(0, rId)).toBool();
                updateValue("feed", "active", tmp, "fid", item->data(0, rId));
                for (int i =0; i < item->childCount(); i++){
                    tmp ? item->child(i)->setForeground(0, QBrush(Qt::black)) :item->child(i)->setForeground(0, QBrush(Qt::gray));
                }
            });
            menu.addAction(newAct);
            menu.addSeparator();

            newAct = new QAction(tr("Delete Folder"), this);
            connect(newAct, QOverload<bool>::of(&QAction::triggered), [=](bool b){
                Q_UNUSED(b)
                QMessageBox::StandardButton btn = QMessageBox::warning(this, tr("Confirmation"),
                    tr("Are you sure you want to delete folder?"), QMessageBox::Yes | QMessageBox::No);
                if (btn != QMessageBox::Yes) return;

                QSqlQuery query(db);
                query.prepare("DELETE FROM folder WHERE id=?");
                query.addBindValue(item->data(0, rId));
                query.exec();
                ui->tree1->itemClicked(item->parent(),0);
                item->parent()->removeChild(item);
                delete item;

            });
            menu.addAction(newAct);
        } else {
            newAct = new QAction(tr("Update Feed"), this);
            connect(newAct, QOverload<bool>::of(&QAction::triggered), [=](bool b){
                Q_UNUSED(b)
                beginToLoadSlot(QUrl(item->toolTip(0)));
            });
            menu.addAction(newAct);

            newAct = new QAction(tr("Delete News"), this);
            connect(newAct, QOverload<bool>::of(&QAction::triggered), [=](bool b){
                Q_UNUSED(b)
                QMessageBox::StandardButton btn = QMessageBox::warning(this, tr("Confirmation"),
                    tr("Are you sure you want to delete news?"), QMessageBox::Yes | QMessageBox::No);
                if (btn != QMessageBox::Yes) return;

                QSqlQuery query(db);
                query.prepare("DELETE FROM item WHERE fid=?");
                query.addBindValue(item->data(0, rId));
                query.exec();
                ui->tree1->itemClicked(item,0);
            });
            menu.addAction(newAct);

            newAct = new QAction(tr("Toggle Enable/Disable Updates"), this);
            connect(newAct, QOverload<bool>::of(&QAction::triggered), [&](bool b){
                Q_UNUSED(b)
                bool tmp = !selectValue(db, "active", "feed", "id", item->data(0, rId)).toBool();
                updateValue("feed", "active", tmp, "id", item->data(0, rId));
                tmp ? item->setForeground(0, QBrush(Qt::black)) :item->setForeground(0, QBrush(Qt::gray));
            });
            menu.addAction(newAct);

            newAct = new QAction(tr("Copy link to clipboard"), this);
            connect(newAct, QOverload<bool>::of(&QAction::triggered), [=](bool b){
                Q_UNUSED(b)
                QClipboard *cb = QGuiApplication::clipboard();
                cb->setText(item->toolTip(0));
            });
            menu.addAction(newAct);

            newAct = new QAction(tr("Reload All"), this);
            connect(newAct, QOverload<bool>::of(&QAction::triggered), [=](bool b){
                Q_UNUSED(b)
                updateValue("feed", "updated", QVariant(), "id", item->data(0, rId));
                beginToLoadSlot(QUrl(item->toolTip(0)));
            });
            menu.addAction(newAct);

            menu.addSeparator();

            newAct = new QAction(tr("Delete Feed"), this);
            connect(newAct, QOverload<bool>::of(&QAction::triggered), [=](bool b){
                Q_UNUSED(b)
                QMessageBox::StandardButton btn = QMessageBox::warning(this, tr("Confirmation"),
                    tr("Are you sure you want to delete feed?"), QMessageBox::Yes | QMessageBox::No);
                if (btn != QMessageBox::Yes) return;

                QSqlQuery query(db);
                query.prepare("DELETE FROM feed WHERE id=?");
                query.addBindValue(item->data(0, rId));
                query.exec();
                ui->tree1->itemClicked(item->parent(),0);
                item->parent()->removeChild(item);
                delete item;
            });
            menu.addAction(newAct);
        }
        menu.exec(QCursor::pos());

    });
    connect(ui->tree1, QOverload<QTreeWidgetItem*, int>::of(&QTreeWidget::itemClicked),
            [this](QTreeWidgetItem *item, int col){
        Q_UNUSED(col)
        if(item == ui->tree1->topLevelItem(0)){
            refreshTable();
        } else if(item->parent() == ui->tree1->topLevelItem(0)){
            refreshTable("SELECT id, title, url, pubdate, unread FROM item where fid in (select id from feed where fid=?)", item->data(0, rId).toInt());
        } else {
            refreshTable("SELECT id, title, url, pubdate, unread FROM item where fid=?", item->data(0, rId).toInt());
        }
        ui->tree1->setCurrentItem(item);
    });

    ui->table1->setHorizontalHeaderLabels({"Title", "Date"});
    ui->table1->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->table1->setItemDelegateForColumn(1, new DtItem(ui->table1));
    ui->table1->horizontalHeader()->setDefaultSectionSize(150);
    ui->table1->verticalHeader()->hide();
    connect(ui->table1, QOverload<const QPoint&>::of(&QTableWidget::customContextMenuRequested),
            [this](const QPoint& pos){
        Q_UNUSED(pos)
        QMenu menu(this);
        QAction* newAct;
        QUrl link = selectValue(db, "url","item","id",ui->table1->item(ui->table1->currentRow(), 0)->data(rId)).toUrl();
        if (!link.isEmpty()){
            menu.addAction(tr("Open link in new tab"), this, SLOT(loadFeedInNewTab()))->setData(link);
            newAct = new QAction(tr("Copy link to clipboard"), this);
            connect(newAct, QOverload<bool>::of(&QAction::triggered), [=](bool b){
                Q_UNUSED(b)
                QClipboard *cb = QGuiApplication::clipboard();
                cb->setText(link.toString());
            });
            menu.addAction(newAct);
        }
        newAct = new QAction(tr("Delete"), this);
        connect(newAct, QOverload<bool>::of(&QAction::triggered), [this](bool b){
            Q_UNUSED(b)
            int row = 0;
            const QList<QTableWidgetItem *> list = ui->table1->selectedItems();
            row = list[0]->row();
            for (auto it : list){
                QSqlQuery query(db);
                query.prepare("DELETE FROM item WHERE id=?");
                query.addBindValue(ui->table1->item(it->row(), 0)->data(rId));
                query.exec();
                query.finish();
            }
            ui->tree1->itemClicked(ui->tree1->selectedItems()[0], 0);
            row >= ui->table1->rowCount() ? ui->table1->selectRow(ui->table1->rowCount()-1) : ui->table1->selectRow(row);
        });
        menu.addSeparator();
        menu.addAction(newAct);
        menu.exec(QCursor::pos());
    });
    connect(ui->table1, QOverload<QTableWidgetItem*, QTableWidgetItem*>::of(&QTableWidget::currentItemChanged),
            [this](QTableWidgetItem *item, QTableWidgetItem* item2){
        Q_UNUSED(item2)
        ui->view1->setHtml("<html><body></body></html>", QUrl("http://_blank"));
        ui->view1->history()->clear();
        if (item == nullptr) return;
        QUrl link = selectValue(db, "url","item","id",ui->table1->item(ui->table1->currentRow(), 0)->data(rId)).toUrl();
        QSqlQuery query(db);
        query.prepare("SELECT text, enc_url, enc_type, enc_length, title, m_url, m_type, m_thurl  FROM item WHERE id=?");
        query.addBindValue(ui->table1->item(ui->table1->currentRow(), 0)->data(rId));
        query.exec();
        QString txt;
        if (query.next()){
            if (query.value(1).toString().length() > 0){
                txt.append("<a href='").append(query.value(1).toString()).append("'>Enclosure ");
                if (query.value(2).isValid() || query.value(3).isValid()){
                    txt.append("(").append(query.value(2).toString()).append(" ");
                    txt.append(QLocale().formattedDataSize(query.value(3).toInt())).append(")");
                }
                txt.append("</a>");
            }
            if (query.value(5).toString().length() > 0){
                txt.append("&nbsp;<a href='").append(query.value(5).toString()).append("'>Media ");
                if (query.value(6).toString().length() > 0){
                    txt.append("(").append(query.value(6).toString()).append(")");
                }
                txt.append("</a>");
            }
            if (query.value(7).toString().length() > 0){
                txt.append("&nbsp;<a href='").append(query.value(7).toString()).append("'>Thumbnail");
                txt.append("</a>");
            }
            txt.append("<br/><hr></br/>");
        }
        txt.append(query.value(4).toString());
        txt.append("<br/><hr></br/>");
        txt.append(query.value(0).toString());
        query.finish();
        ui->view1->setEnabled(false);
        ui->view1->setHtml(txt, QUrl("http://_blank"));
        ui->view1->setEnabled(true);
        updateValue("item", "unread", 0, "id", ui->table1->item(ui->table1->currentRow(), 0)->data(rId));
        QFont font;
        font.setBold(false);
        ui->table1->item(ui->table1->currentRow(), 0)->setFont(font);
        m_window->statusBar()->showMessage(link.toString(), 2000);

    });
    connect(ui->table1, QOverload<QTableWidgetItem*>::of(&QTableWidget::itemDoubleClicked),
            [this](QTableWidgetItem *item){
        Q_UNUSED(item)
        QUrl link = selectValue(db, "url","item","id",ui->table1->item(ui->table1->currentRow(), 0)->data(rId)).toUrl();
        ui->view1->setHtml("<html><body></body></html>", QUrl("http://_blank"));
        ui->view1->history()->clear();
        QSqlQuery query(db);
        query.prepare("SELECT text, enc_url, enc_type, enc_length, title, m_url, m_type, m_thurl FROM item WHERE id=?");
        query.addBindValue(ui->table1->item(ui->table1->currentRow(), 0)->data(rId));
        query.exec();
        QString txt;
        if (query.next()){
            if (query.value(1).toString().length() > 0){
                txt.append("<a href='").append(query.value(1).toString()).append("'>Enclosure ");
                if (query.value(2).isValid() || query.value(3).isValid()){
                    txt.append("(").append(query.value(2).toString()).append(" ");
                    txt.append(QLocale().formattedDataSize(query.value(3).toInt())).append(")");
                }
                txt.append("</a>");
            }
            if (query.value(5).toString().length() > 0){
                txt.append("&nbsp;<a href='").append(query.value(5).toString()).append("'>Media ");
                if (query.value(6).toString().length() > 0){
                    txt.append("(").append(query.value(6).toString()).append(")");
                }
                txt.append("</a>");
            }
            if (query.value(7).toString().length() > 0){
                txt.append("&nbsp;<a href='").append(query.value(7).toString()).append("'>Thumbnail");
                txt.append("</a>");
            }
            txt.append("<br/><hr></br/>");
        }
        txt.append(query.value(4).toString());
        txt.append("<br/><hr></br/>");
        txt.append(query.value(0).toString());
        query.finish();
        ui->view1->setHtml(txt, QUrl("http://_blank"));
        updateValue("item", "unread", 0, "id", ui->table1->item(ui->table1->currentRow(), 0)->data(rId));
        QFont font;
        font.setBold(false);
        ui->table1->item(ui->table1->currentRow(), 0)->setFont(font);
        getQupZilla()->tabWidget()->addView(link, qzSettings->newTabPosition);
    });
    connect(ui->view1->page(), &QWebEnginePage::linkHovered, [this](const QString &url) {
        if (url.isEmpty()) return;
        m_window->statusBar()->showMessage(url, 3000);
    });
    db = QSqlDatabase::addDatabase("QSQLITE", "rss");
    db.setDatabaseName(DataPaths::currentProfilePath() + QLatin1String("/rss.db"));
    db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=10000");
    if(!db.open()){
        qCritical() << "Error:" << db.lastError().text();
    } else {
        if(db.tables().empty()) {
            auto list = QzTools::readAllFileContents(QSL(":/data/rss.sql")).split(";", Qt::SkipEmptyParts);
            for(const auto& sql : list){
               if(sql.trimmed().length() <= 0) continue;
               QSqlQuery query(db);
               if (!query.exec(sql))
                   qCritical() << "Error:" << query.lastError().text();
            }
        }
        QSqlQuery query(db);
        query.exec("PRAGMA foreign_keys = ON");
        query.exec("PRAGMA journal_mode=WAL");
        query.finish();
    }
}

BrowserWindow* RSSManager::getQupZilla()
{
    if (!m_window) {
        m_window = mApp->getWindow();
    }
    return m_window.data();
}

void RSSManager::setMainWindow(BrowserWindow* window)
{
    if (window) {
        m_window = window;
    }
}

void RSSManager::refreshTree() {
    ui->tree1->clear();    
    QTreeWidgetItem* root = new QTreeWidgetItem();
    root->setText(0, "All");
    ui->tree1->insertTopLevelItem(0, root);

    QSqlQuery query(db);
    query.exec("SELECT id, name FROM folder");
    while (query.next()) {
        int id = query.value(0).toUInt();
        QString name = query.value(1).toString();
        QTreeWidgetItem* item = new QTreeWidgetItem();
        item->setText(0, name);
        item->setData(0, rId, QVariant(id));
        root->addChild(item);

        QSqlQuery q(db);
        q.prepare("SELECT id, title, url, icon, active FROM feed where fid=?");
        q.bindValue(0, id);
        q.exec();
        while (q.next()) {
            QTreeWidgetItem* it = new QTreeWidgetItem();
            it->setData(0, rId, QVariant(q.value(0)));
            it->setToolTip(0,q.value(2).toString());
            q.value(4).toBool() ? it->setForeground(0, QBrush(Qt::black)) : it->setForeground(0, QBrush(Qt::gray));
            q.value(1).isNull() ? it->setText(0,q.value(2).toString()) : it->setText(0,q.value(1).toString());

            QPixmap pix = QPixmap();
            pix.loadFromData( q.value(3).toByteArray() );
            if(pix.isNull()) pix = QPixmap(":/icons/other/feed.png");
            it->setIcon(0,QIcon(pix));
            item->addChild(it);

        }
        q.finish();
    }
    query.finish();
    ui->tree1->expandAll();
    refreshTable();
    root->setSelected(true);
}

void RSSManager::refreshTable()
{
    QSqlQuery query(db);
    ui->table1->setRowCount(0);
    ui->table1->setSortingEnabled(false);
    query.exec("SELECT id, title, url, pubdate, unread FROM item");
    QFont font;
    font.setBold(true);
    while (query.next()) {
        int row = ui->table1->rowCount();
        ui->table1->insertRow(row);

        QTableWidgetItem *newItem = new QTableWidgetItem(query.value(1).toString());
        ui->table1->setItem(row, 0, newItem);
        newItem->setToolTip(query.value(1).toString());
        newItem->setData(rId, query.value(0));
        if(query.value(4).toBool()) newItem->setFont(font);

        newItem = new QTableWidgetItem();
        newItem->setData(Qt::DisplayRole, query.value(3));
        ui->table1->setItem(row, 1, newItem);
    }
    ui->table1->setSortingEnabled(true);
    ui->table1->sortItems(1, Qt::DescendingOrder);
    ui->table1->selectRow(0);
    txt->setText(QString::number(ui->table1->rowCount()));

}

void RSSManager::refreshTable(const QString& sql, int id)
{
    QSqlQuery query(db);
    ui->table1->setRowCount(0);
    ui->table1->setSortingEnabled(false);
    query.prepare(sql);
    query.bindValue(0, id);
    query.exec();
    QFont font;
    font.setBold(true);

    while (query.next()) {
        int row = ui->table1->rowCount();
        ui->table1->insertRow(row);
        QTableWidgetItem *newItem = new QTableWidgetItem(query.value(1).toString());
        ui->table1->setItem(row, 0, newItem);
        newItem->setToolTip(query.value(1).toString());
        newItem->setData(rId, query.value(0));
        if(query.value(4).toBool()) newItem->setFont(font);

        newItem = new QTableWidgetItem();
        newItem->setData(Qt::DisplayRole, query.value(3));
        ui->table1->setItem(row, 1, newItem);
    }
    ui->table1->setSortingEnabled(true);
    ui->table1->sortItems(1, Qt::DescendingOrder);
    ui->table1->selectRow(0);
    txt->setText(QString::number(ui->table1->rowCount()));
}

void RSSManager::reloadFeeds()
{
    QSqlQuery q(db);
    q.prepare("SELECT id,url FROM feed WHERE active");
    q.exec();
    while (q.next()) {
        beginToLoadSlot(QUrl(q.value(1).toString()));
    }
    q.finish();
}

void RSSManager::reloadFeeds(int id)
{
    QSqlQuery q(db);
    q.prepare("SELECT id,url FROM feed WHERE active AND fid=?");
    q.addBindValue(id);
    q.exec();
    q.exec();
    while (q.next()) {
        beginToLoadSlot(QUrl(q.value(1).toString()));
    }
    q.finish();
}

void RSSManager::addFolder()
{
    QString name = QInputDialog::getText(this, tr("Add new folder"), tr("Please enter new name:"));

    if (name.isEmpty()) {
        return;
    }
    QSqlQuery query(db);
    query.prepare("SELECT id FROM folder WHERE name=?");
    query.addBindValue(name);
    query.exec();

    if (query.next()) {
        QMessageBox::warning(getQupZilla(), tr("RSS feed duplicated"), tr("You already have this folder."));
        return;
    }
    query.finish();

    query.prepare("INSERT INTO folder (name) VALUES(?)");
    query.bindValue(0, name);
    query.exec();

    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText(0, name);
    item->setData(0, rId, query.lastInsertId().toInt());
    ui->tree1->topLevelItem(0)->addChild(item);
    ui->tree1->itemClicked(item,0);

}

void RSSManager::addFeed()
{
    QUrl url = QUrl(QInputDialog::getText(this, tr("Add new feed"), tr("Please enter URL of new feed:")));
    if (url.isEmpty() || !url.isValid()) {
        return;
    }
    addRssFeed(url, tr("New feed"), IconProvider::iconForUrl(url));
}

void RSSManager::loadFeed(QTreeWidgetItem* item)
{
    if (!item) {
        return;
    }
    if (item->toolTip(0).isEmpty()) {
        return;
    }
    getQupZilla()->loadAddress(QUrl(item->toolTip(0)));
}

void RSSManager::controlLoadFeed(QTreeWidgetItem* item)
{
    if (!item || item->toolTip(0).isEmpty()) {
        return;
    }
    getQupZilla()->tabWidget()->addView(QUrl(item->toolTip(0)), qzSettings->newTabPosition);
}

void RSSManager::loadFeedInNewTab()
{
    if (QAction* action = qobject_cast<QAction*>(sender())) {
        getQupZilla()->tabWidget()->addView(action->data().toUrl(), qzSettings->newTabPosition);
    }
}

void RSSManager::beginToLoadSlot(const QUrl &url)
{
    QDateTime tmp(QDateTime::fromString(selectValue(db, "lm","feed","url", QVariant(url)).toString()));
        QList<QPair<QString, QString>> list;
    if(!tmp.isNull() ) {
            list << QPair<QString, QString>("If-Modified-Since", tmp.toString("ddd, dd MMM yyyy HH:mm:ss").append(" GMT"));
    }

    FollowRedirectReply* reply = new FollowRedirectReply(url, m_networkManager, list);
    connect(reply, SIGNAL(finished()), this, SLOT(finished()));

    QPair<FollowRedirectReply*, QUrl> pair;
    pair.first = reply;
    pair.second = url;
    m_replies.append(pair);

}

void RSSManager::finished()
{
    FollowRedirectReply* reply = qobject_cast<FollowRedirectReply*> (sender());
    if (!reply) return;
    if(reply->status() == 304) return;

    QString replyUrl;
    for (int i = 0; i < m_replies.count(); i++) {
        QPair<FollowRedirectReply*, QUrl> pair = m_replies.at(i);
        if (pair.first == reply) {
            replyUrl = pair.second.toString();
            break;
        }
    }
    if (replyUrl.isEmpty()) return;

    QByteArray arr(reply->readAll());
    QThread* thread = new QThread();
    WorkerThread* worker = new WorkerThread();
    worker->add(reply->originalUrl().toString(), arr, reply->lm(), reply->status());
    worker->moveToThread(thread);
    connect(thread, SIGNAL(started()), this, SLOT(countP()));
    connect(thread, SIGNAL(finished()), this, SLOT(countM()));
    connect( thread, &QThread::started, worker, &WorkerThread::run);
    connect( worker, &WorkerThread::finished, thread, &QThread::quit);
    connect( worker, &WorkerThread::finished, worker, &WorkerThread::deleteLater);
    connect( thread, &QThread::finished, thread, &QThread::deleteLater);
    connect(worker, &WorkerThread::update1,this,&RSSManager::update1);
    thread->start();
}

bool RSSManager::addRssFeed(const QUrl &url, const QString &title, const QIcon &icon)
{
    if (url.isEmpty()) {
        return false;
    }
    QSqlQuery query(db);
    query.prepare("SELECT id FROM feed WHERE url=?");
    query.addBindValue(url);
    query.exec();

    if (!query.next()) {
        QImage image = icon.pixmap(16, 16).toImage();

        if (image == IconProvider::emptyWebImage()) {
            image.load(":icons/menu/rss.png");
        }

        query.prepare("INSERT INTO feed (url, title, icon, fid) VALUES(?,?,?,?)");
        query.bindValue(0, url);
        query.bindValue(1, title);
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "PNG");
        query.bindValue(2, buffer.data());
        query.bindValue(3, 1);
        query.exec();
        return true;
    }

    QMessageBox::warning(getQupZilla(), tr("RSS feed duplicated"), tr("You already have this feed."));
    return false;
}

RSSManager::~RSSManager()
{
    delete ui;
    db.close();
    db = QSqlDatabase();
    QSqlDatabase::removeDatabase("rss");
}

QVariant RSSManager::selectValue(QSqlDatabase& db, const QString& col1, const QString& t, const QString& col2, const QVariant& id)
{
    QString sql;
    sql.append("select ").append(col1).append(" from ").append(t);
    if(id.isValid()) sql.append(" where ").append(col2).append("=:id");

    QSqlQuery query(db);
    query.prepare(sql);
    if(id.isValid()) query.addBindValue(id);
    query.setForwardOnly(true);
    query.exec();
    query.first();
    QVariant val = query.value(0);
    query.finish();
    return val;
}

QVariant RSSManager::updateValue(const QString& t, const QString& col1, const QVariant& val,
                                 const QString& col2, const QVariant& id)
{
    QString sql;
    sql.append("UPDATE ").append(t).append(" set ").append(col1).append("=?");
    if(id.isValid()) sql.append(" where ").append(col2).append("=?");

    QSqlQuery query(db);
    query.prepare(sql);
    query.bindValue(0, val);
    if(id.isValid()) query.bindValue(1, id);
    query.exec();
    query.finish();
    return QVariant();
}

void RSSManager::update1(const QString& str){
    QList<QTreeWidgetItem *> list = ui->tree1->findItems(str, Qt::MatchFixedString|Qt::MatchRecursive, 0);
    if(list.size()>0) list[0]->setText(0, selectValue(db, "title","feed","url", str).toString());
    QPixmap pix = QPixmap();
    pix.loadFromData(selectValue(db, "icon", "feed", "url", str).toByteArray());
    if(pix.isNull()) pix = QPixmap(":/icons/other/feed.png");
    if(list.size() > 0) list[0]->setIcon(0, QIcon(pix));

    QString name = selectValue(db, "title", "feed", "url", str).toString();
    QUrl url(str);
    FollowRedirectReply* reply1 = new FollowRedirectReply(QUrl(QString("%1://%2/favicon.ico").arg(url.scheme()).arg(url.host())), m_networkManager);
    connect(reply1, &FollowRedirectReply::finished, [=]{
       QByteArray arr(reply1->readAll());
       QPixmap pix = QPixmap();
       pix.loadFromData( arr );
       if (pix.isNull()){
           if (url.host().isEmpty() || url.toString().count('.') < 2) return;
           QString h = url.host().mid(url.host().lastIndexOf(".", url.host().lastIndexOf(".")-1)+1);
           FollowRedirectReply* reply1 = new FollowRedirectReply(QUrl(QString("%1://%2/favicon.ico").arg(url.scheme()).arg(h)), m_networkManager);
           connect(reply1, &FollowRedirectReply::finished, [=]{
              QByteArray arr(reply1->readAll());
              QPixmap pix = QPixmap();
              pix.loadFromData( arr );
              if (pix.isNull()) return;
              updateValue("feed", "icon", arr, "url", url.toString());
              QList<QTreeWidgetItem *> list = ui->tree1->findItems(name, Qt::MatchFixedString|Qt::MatchRecursive, 0);
              if(list.size()>0) list[0]->setIcon(0, QIcon(pix));
           });
           return;
       }
       updateValue("feed", "icon", arr, "url", url.toString());
       QList<QTreeWidgetItem *> list = ui->tree1->findItems(name, Qt::MatchFixedString|Qt::MatchRecursive, 0);
       if(list.size()>0) list[0]->setIcon(0, QIcon(pix));
    });
    connect(reply1, &FollowRedirectReply::finished, reply1, &QObject::deleteLater);
}

void RSSManager::update(){
     if (ui->tree1->selectedItems().size() > 0) {
        ui->tree1->itemClicked(ui->tree1->selectedItems()[0], 0);
     } else {
        ui->tree1->itemClicked(ui->tree1->topLevelItem(0), 0);
     }
}

void RSSManager::countP(){
    if(aCount++ == 0) thStart();
}

void RSSManager::countM(){
    if(aCount-- == 1){
        thStop();
        QTimer::singleShot(100, this, &RSSManager::update);
    }
}
