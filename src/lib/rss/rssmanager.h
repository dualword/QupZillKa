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
#ifndef RSSMANAGER_H
#define RSSMANAGER_H

#include "mainapplication.h"
#include "qzcommon.h"
#include "tabwidget.h"
#include "qzsettings.h"
#include "followredirectreply.h"
#include "datapaths.h"
#include "tabbedwebview.h"
#include "webpage.h"
#include "networkmanager.h"

#include <QWidget>
#include <QTreeWidget>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QFormLayout>
#include <QPointer>
#include <QDialogButtonBox>
#include <QToolButton>
#include <QInputDialog>
#include <QtSql>
#include <QStyledItemDelegate>
#include <QWebEnginePage>
#include <QNetworkReply>
#include <QTableWidgetItem>

class TItem : public QTableWidgetItem {
public:
    TItem(const QString& t) : QTableWidgetItem(t) {}
    bool operator<(const QTableWidgetItem &other) const {
        if (data(Qt::UserRole +1).isValid() & other.data(Qt::UserRole +1).isValid()) {
            return data(Qt::UserRole +1).toDateTime() < other.data(Qt::UserRole +1).toDateTime();
        }
        return QTableWidgetItem::operator<(other);
    }
};

class DtItem : public QStyledItemDelegate {
    Q_OBJECT
public:
    DtItem(QObject *p = nullptr) : QStyledItemDelegate(p) {};
    QString displayText(const QVariant &val, const QLocale &locale) const {
        return QDateTime::fromString(val.toString(), Qt::ISODate).toLocalTime().toString("dd MMM yyyy HH:mm:ss");
    }
};

class WPage : public QWebEnginePage {
    Q_OBJECT
public:
    WPage(QWebEngineProfile *profile, QObject* p = nullptr) : QWebEnginePage(profile, p) {};

protected:
    bool acceptNavigationRequest(const QUrl &url, QWebEnginePage::NavigationType type, bool isMainFrame) override{
        if (type == QWebEnginePage::NavigationTypeLinkClicked) {
            mApp->getWindow()->tabWidget()->addView(url, qzSettings->newTabPosition);
            return false;
        }
        return true;
    }
    QWebEnginePage* createWindow(QWebEnginePage::WebWindowType type) {
        int index =mApp->getWindow()->tabWidget()->addView(QUrl(), Qz::NT_CleanNotSelectedTab);
        TabbedWebView* view = mApp->getWindow()->weView(index);
        view->setPage(new WebPage());
        return view->page();
    }
};

class WorkerThread;

namespace Ui
{
class RSSManager;

}

class BrowserWindow;
class FollowRedirectReply;
class NetworkManager;
class QUPZILLA_EXPORT RSSManager : public QWidget
{
    Q_OBJECT

    friend class BrowsingLibrary;
public:
    explicit RSSManager(BrowserWindow* window, QWidget* p = 0);
    ~RSSManager();

    bool addRssFeed(const QUrl &url, const QString &title, const QIcon &icon);
    void setMainWindow(BrowserWindow* window);

public slots:
    void update();
    void update1(const QString&);
    void refreshTable();
    void refreshTable(const QString&, int);
    void refreshTree();
    void countP();
    void countM();
    void optimizeDb() {QSqlQuery q(db);q.exec(QSL("VACUUM"));q.finish();};
    void thStart(){
        m_reloadButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaStop));
        m_reloadButton->setEnabled(false);
    };
    void thStop(){
        m_reloadButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
        m_reloadButton->setEnabled(true);
    };
    QVariant selectValue(QSqlDatabase& db, const QString&, const QString&, const QString& = QString(), const QVariant& = QVariant());
    QVariant updateValue(const QString&, const QString&, const QVariant&, const QString& = QString(), const QVariant& = QVariant());

private slots:
    void beginToLoadSlot(const QUrl &url);
    void finished();
    void loadFeed(QTreeWidgetItem* item);
    void controlLoadFeed(QTreeWidgetItem* item);
    void addFeed();
    void addFolder();
    void reloadFeeds();
    void reloadFeeds(int);
    void loadFeedInNewTab();

private:
    BrowserWindow* getQupZilla();
    QList<QPair<FollowRedirectReply*, QUrl> > m_replies;
    NetworkManager* m_networkManager;
    Ui::RSSManager* ui;
    QToolButton *m_reloadButton, *btnImage;
    QPointer<BrowserWindow> m_window;
    QLineEdit* txt;
    QSqlDatabase db;
    int rId = Qt::UserRole +1;
    QAtomicInteger<qint8> aCount = 0;
};

class WorkerThread : public QObject {
    Q_OBJECT
public:
    WorkerThread(QObject *p = nullptr) : QObject(p){cname = QString::number( reinterpret_cast<int>(this));};
    void run() {        
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", cname);
        db.setDatabaseName(DataPaths::currentProfilePath() + QLatin1String("/rss.db"));
        db.setConnectOptions("QSQLITE_BUSY_TIMEOUT=10000");
        db.open();
        int count = 0;
        QString currentTag, strLink, titleString, desc, pubDate, guid, encUrl, encLenght, encType;
        QString mUrl, mType, mtUrl;
        QString fTitle, fDesc, fLink;
        QDateTime fPDate, fBDate;
        QXmlStreamReader xml;
        xml.addData(arr);
        bool b = true;
        {QSqlQuery query(db);
        query.exec("PRAGMA foreign_keys = ON");
        query.exec("PRAGMA journal_mode=WAL");
        query.finish();}
        int id  = mApp->rssManager()->selectValue(db, "id", "feed", "url", url).toInt();
        QDateTime dt = QDateTime::fromString(mApp->rssManager()->selectValue(db, "updated", "feed", "id", id).toString(), Qt::ISODate);

            if(xml.readNextStartElement()){
                if(xml.name() == "rss"){
                    if(xml.readNextStartElement()){
                        if(xml.name() == "channel"){
                            while(!xml.atEnd() & b){
                                while(xml.readNextStartElement()){
                                    if(xml.name() == "title"){
                                        if(fTitle.isNull()) fTitle =xml.readElementText();
                                    }
                                    if(xml.name() == "description"){
                                        fDesc =xml.readElementText();
                                    }
                                    if(xml.name() == "link"){
                                        fLink =xml.readElementText();
                                    }
                                    if(xml.name() == "lastBuild"){
                                        fBDate = QDateTime::fromString(xml.readElementText(), Qt::RFC2822Date);
                                    }
                                    if(xml.name() == "pubDate"){
                                        fPDate = QDateTime::fromString(xml.readElementText(), Qt::RFC2822Date);
                                    }
                                    if(xml.name() == "item"){
                                        while(!xml.atEnd() & b){
                                            xml.readNext();
                                            if (xml.isStartElement()) {
                                              if (xml.name() == QLatin1String("item")) {
                                                    strLink = xml.attributes().value("rss:about").toString();
                                              }else if (xml.name() == "enclosure")   {
                                                  encUrl = xml.attributes().value("url").toString();
                                                  encLenght = xml.attributes().value("length").toString();
                                                  encType = xml.attributes().value("type").toString();
                                              }else if (xml.name() == "content")   {
                                                  mUrl = xml.attributes().value("url").toString();
                                                  mType = xml.attributes().value("medium").toString();
                                              }else if (xml.name() == "thumbnail")   {
                                                  mtUrl = xml.attributes().value("url").toString();
                                              }
                                              currentTag = xml.qualifiedName().toString();
                                            } else if (xml.isEndElement()) {
                                                if (xml.qualifiedName() == QLatin1String("item")) {

                        QDateTime d = QDateTime::fromString(pubDate, Qt::RFC2822Date);
                        if(!d.isValid()) d = QDateTime::fromString(pubDate, Qt::ISODate);
                         if (d > dt) {
{QSqlQuery query(db);
 query.prepare("INSERT INTO item (fid, title, url, text, pubdate, guid, enc_url, enc_length, enc_type, unread,"
"m_url, m_type, m_thurl) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?)");
                                                                 query.bindValue(0, id);
                                                                 query.bindValue(1, titleString);
                                                                 query.bindValue(2, strLink);
                                                                 query.bindValue(3, desc);
                                                                 query.bindValue(4, d.toUTC().toString(Qt::ISODate));
                                                                 query.bindValue(5, guid);
                                                                 query.bindValue(6, encUrl);
                                                                 query.bindValue(7, encLenght);
                                                                 query.bindValue(8, encType);
                                                                 query.bindValue(9, 1);
                                                                 query.bindValue(10, mUrl);
                                                                 query.bindValue(11, mType);
                                                                 query.bindValue(12, mtUrl);
                                                                 query.exec();
                                                                 query.finish();}
                                                                 count++;
                         } else {b = false;}


                                                    titleString.clear();
                                                    strLink.clear();
                                                    desc.clear();
                                                    pubDate.clear();
                                                    guid.clear();
                                                    encUrl.clear(); encType.clear(); encLenght.clear();

                                                }
                                            } else if (xml.isCharacters() && !xml.isWhitespace()) {
                                                if (currentTag == QLatin1String("title")) {
                                                    titleString = xml.text().toString();
                                                } else if (currentTag == QLatin1String("link")) {
                                                    strLink += xml.text().toString();
                                                } else if (currentTag == QLatin1String("description")) {
                                                    desc += xml.text().toString();
                                                } else if (currentTag == QLatin1String("pubDate")) {
                                                    pubDate += xml.text().toString();

                                                } else if (currentTag == QLatin1String("guid")) {
                                                    guid += xml.text().toString();
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                } else if(xml.name() == "feed"){
                            while(!xml.atEnd() & b){
                                while(xml.readNextStartElement()){
                                    if(xml.name() == "title"){
                                        fTitle =xml.readElementText();
                                    }
                                    if(xml.name() == "subtitle"){
                                        fDesc =xml.readElementText();
                                    }
                                    if(xml.name() == "link"){
                                        fLink = xml.attributes().value("href").toString();
                                    }
                                    if(xml.name() == "lastBuild"){
                                        fBDate = QDateTime::fromString(xml.readElementText(), Qt::ISODate);
                                    }
                                    if(xml.name() == "updated"){
                                        fPDate = QDateTime::fromString(xml.readElementText(), Qt::ISODate);
                                    }
                                    if(xml.name() == "entry"){
                                        while(!xml.atEnd() & b){
                                            xml.readNext();

                                            if (xml.isStartElement()) {
                                              if (xml.name() == QLatin1String("entry")) {
                                                    strLink = xml.attributes().value("rss:about").toString();
                                                }else if ( (xml.name() == QLatin1String("link"))
                                                           & (xml.attributes().value("rel") == "enclosure")) {
                                                     encUrl = xml.attributes().value("href").toString();
                                                     encType = xml.attributes().value("type").toString();
                                                     encLenght = xml.attributes().value("length").toString();;
                                                 }else if ( (xml.name() == QLatin1String("link"))
                                                            & (xml.attributes().value("rel") == "alternate")) {
                                                      strLink = xml.attributes().value("href").toString();
                                                  }
                                                currentTag = xml.qualifiedName().toString();
                                            } else if (xml.isEndElement()) {
                                                if (xml.qualifiedName() == QLatin1String("entry")) {

                                        QDateTime d = QDateTime::fromString(pubDate, Qt::ISODate);
                                         if (d > dt) {
                                             {QSqlQuery query(db);
                                             query.prepare("INSERT INTO item (fid, title, url, text, pubdate, guid, enc_url, enc_length, enc_type, unread)"
                                             " VALUES(?,?,?,?,?,?,?,?,?,?)");
                                                    query.bindValue(0, id);
                                                    query.bindValue(1, titleString);
                                                    query.bindValue(2, strLink);
                                                    query.bindValue(3, desc);
                                                    query.bindValue(4, d.toUTC().toString(Qt::ISODate));
                                                    query.bindValue(5, guid);
                                                    query.bindValue(6, encUrl);
                                                    query.bindValue(7, encLenght);
                                                    query.bindValue(8, encType);
                                                    query.bindValue(9, 1);
                                                    query.exec();
                                                    query.finish();
                                                    titleString.clear();
                                                    strLink.clear();
                                                    desc.clear();
                                                    pubDate.clear();
                                                    guid.clear();
                                                    encUrl.clear(); encType.clear(); encLenght.clear();}
                                                    count++;

                                        } else {b = false;}
                                                }
                                            } else if (xml.isCharacters() && !xml.isWhitespace()) {
                                                if (currentTag == QLatin1String("title")) {
                                                    titleString = xml.text().toString();
                                                } else if (currentTag == QLatin1String("id")) {
                                                    guid = xml.text().toString();
                                                }
                                                else if (currentTag == QLatin1String("summary")) {
                                                    desc += xml.text().toString();
                                                }else if (currentTag == QLatin1String("published")) {
                                                    pubDate += xml.text().toString();
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
            }

            if(count > 0) {
                QSqlQuery query(db);
                query.prepare("UPDATE feed SET xmlurl=?, title=?, description=?, lastbuild=?, pubdate=?, updated=?, "
                                " lm=? WHERE id=?");
                query.bindValue(0, fLink);
                query.bindValue(1, fTitle);
                query.bindValue(2, fDesc);
                query.bindValue(3, fBDate.toUTC().toString(Qt::ISODate));
                query.bindValue(4, fPDate.toUTC().toString(Qt::ISODate));
                query.bindValue(5, QDateTime::currentDateTime().toUTC().toString(Qt::ISODate));
                query.bindValue(6, QDateTime::fromString(lm, Qt::RFC2822Date).toString());
                query.bindValue(7, id);
                query.exec();
                query.finish();
            }
        db.close();
        db = QSqlDatabase();
        QSqlDatabase::removeDatabase(cname);
        if(dt.isNull() & (fTitle.length() > 0)) {
            emit update1(url);
        }
        emit finished();
    }
signals:
    void update1(const QString&);
    void finished();

public slots:
    void add(const QString& url, const QByteArray& arr, const QString& lm, int status){
        this->url = QString(url);
        this->lm = QString(lm);
        this->status = status;
        this->arr = QByteArray(arr);
    };

private:
    QString cname, url, lm;
    int status;
    QByteArray arr;
};

#endif // RSSMANAGER_H
