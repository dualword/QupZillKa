/* QupZillKa (2021-2025) https://github.com/dualword/QupZillKa License:GNU GPL v3*/
/* ============================================================
* QupZilla - Qt web browser
* Copyright (C) 2015-2018 David Rosca <nowrep@gmail.com>
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
#include "networkurlinterceptor.h"
#include "urlinterceptor.h"
#include "settings.h"
#include "mainapplication.h"
#include "useragentmanager.h"

#include <QMutexLocker>

NetworkUrlInterceptor::NetworkUrlInterceptor(QObject *parent)
    : QWebEngineUrlRequestInterceptor(parent)
    , m_sendDNT(false), m_fparty(false), m_clearRef(false)
{
}

void NetworkUrlInterceptor::interceptRequest(QWebEngineUrlRequestInfo &info)
{
    QMutexLocker lock(&m_mutex);

    if (m_sendDNT) {
        info.setHttpHeader(QByteArrayLiteral("DNT"), QByteArrayLiteral("1"));
    }

    if (m_clearRef) {
        info.setHttpHeader(QByteArrayLiteral("Referer"),{});
    }
	
	if(m_fparty && (info.firstPartyUrl().host() != info.requestUrl().host()))
		info.block(true);

    const QString host = info.firstPartyUrl().host();
    info.setHttpHeader(QByteArrayLiteral("User-Agent"), mApp->userAgentManager()->globalUserAgent().toUtf8());
    if (m_usePerDomainUserAgent) {
        QString userAgent;
        if (m_userAgentsList.contains(host)) {
            userAgent = m_userAgentsList.value(host);
        } else {
            QHashIterator<QString, QString> i(m_userAgentsList);
            while (i.hasNext()) {
                i.next();
                if (host.endsWith(i.key())) {
                    userAgent = i.value();
                    break;
                }
            }
        }
        if (!userAgent.isEmpty()) {
            info.setHttpHeader(QByteArrayLiteral("User-Agent"), userAgent.toUtf8());
        }
    }

    foreach (UrlInterceptor *interceptor, m_interceptors) {
        interceptor->interceptRequest(info);
    }
}

void NetworkUrlInterceptor::installUrlInterceptor(UrlInterceptor *interceptor)
{
    QMutexLocker lock(&m_mutex);

    if (!m_interceptors.contains(interceptor)) {
        m_interceptors.append(interceptor);
    }
}

void NetworkUrlInterceptor::removeUrlInterceptor(UrlInterceptor *interceptor)
{
    QMutexLocker lock(&m_mutex);

    m_interceptors.removeOne(interceptor);
}

void NetworkUrlInterceptor::loadSettings()
{
    QMutexLocker lock(&m_mutex);

    Settings settings;
    settings.beginGroup("Web-Browser-Settings");
    m_sendDNT = settings.value("DoNotTrack", false).toBool();
    m_fparty = settings.value("FirstParty", false).toBool();
    m_clearRef = settings.value("ClearRef", false).toBool();
    settings.endGroup();

    m_usePerDomainUserAgent = mApp->userAgentManager()->usePerDomainUserAgents();
    m_userAgentsList = mApp->userAgentManager()->perDomainUserAgentsList();

}
