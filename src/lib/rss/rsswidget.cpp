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
#include "rsswidget.h"
#include "ui_rsswidget.h"
#include "mainapplication.h"
#include "tabbedwebview.h"
#include "webpage.h"
#include "rssmanager.h"
#include "rssnotification.h"

#include <QToolTip>
#include <QPushButton>
#include <QSqlQuery>
#include <QTextEdit>

RSSWidget::RSSWidget(WebView* view, QWidget* parent)
    : LocationBarPopup(parent)
    , ui(new Ui::RSSWidget)
    , m_view(view)
{
    ui->setupUi(this);

    QString script =
        "(function() {"
        "  var arr = document.querySelectorAll(\"link[type='application/rss+xml'], link[type='application/atom+xml'] \");"
        "  var urls = [];"
        "  for (var i = 0; i < arr.length; i++) {"
        "    urls.push(arr[i].href);"
        "  }"
        "  return urls.join('\\n');"
        "})();";

    auto txt = new QTextEdit(this);
    txt->setReadOnly(true);
    m_view->page()->runJavaScript(script, [=](const QVariant& var){
        auto list = var.toString().split('\n', Qt::SkipEmptyParts);
        foreach (const QString &url, list) {
            txt->append(url);
        }
    });
    ui->gridLayout->addWidget(txt, 0, 0);
}

void RSSWidget::addRss()
{
    if (!m_view) {
        return;
    }
    if (QPushButton* button = qobject_cast<QPushButton*>(sender())) {
        QUrl url = button->property("rss-url").toUrl();

//        if (url.isRelative()) {
//            url = m_view->page()->mainFrame()->baseUrl().resolved(url);
//        }

        if (!url.isValid()) {
            return;
        }

        QString title = button->property("rss-title").toString();
        if (title.isEmpty()) {
            title = m_view->url().host();
        }

        RSSNotification* notif = new RSSNotification(title, url, m_view);
        m_view->addNotification(notif);
        close();
    }
}

bool RSSWidget::isRssFeedAlreadyStored(const QUrl &url)
{
    QUrl rurl = url;

//    if (url.isRelative()) {
//        rurl = m_view->page()->mainFrame()->baseUrl().resolved(url);
//    }

    if (rurl.isEmpty()) {
        return false;
    }
    QSqlQuery query;
    query.prepare("SELECT id FROM rss WHERE address=?");
    query.addBindValue(rurl);
    query.exec();

    return query.next();
}

RSSWidget::~RSSWidget()
{
    delete ui;
}
