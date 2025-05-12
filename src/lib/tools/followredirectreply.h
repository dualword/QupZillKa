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

#ifndef FOLLOWREDIRECTREPLY_H
#define FOLLOWREDIRECTREPLY_H

#include <QObject>
#include <QNetworkReply>

#include "qzcommon.h"

class QNetworkAccessManager;
class QNetworkReply;
class QUrl;

class QUPZILLA_EXPORT FollowRedirectReply : public QObject
{
    Q_OBJECT
public:
    explicit FollowRedirectReply(const QUrl &url, QNetworkAccessManager* manager,
                                 QList<QPair<QString, QString>> = QList<QPair<QString, QString>>());
    ~FollowRedirectReply();

    QNetworkReply* reply() const;
    QUrl originalUrl() const;
    QUrl url() const;

    QNetworkReply::NetworkError error() const;
    QString errorString() const;
    int status() {return m_status;};
    QString lm() {return m_lm;};
    QByteArray readAll();

signals:
    void finished();

private slots:
    void replyFinished();

private:
    QNetworkAccessManager* m_manager;
    QNetworkReply* m_reply;
    int m_redirectCount, m_status;
    QString m_lm;

};

#endif // FOLLOWREDIRECTREPLY_H
