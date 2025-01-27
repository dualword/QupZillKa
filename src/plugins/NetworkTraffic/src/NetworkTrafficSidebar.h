/* QupZillKa (2023) http://github.com/dualword/QupZillKa License:GNU GPL version 3*/

#ifndef SRC_PLUGINS_NETWORKTRAFFIC_SRC_NETWORKTRAFFICSIDEBAR_H_
#define SRC_PLUGINS_NETWORKTRAFFIC_SRC_NETWORKTRAFFICSIDEBAR_H_

#include "sidebarinterface.h"

class NetworkInterceptor;
class QWidget;
class QTableWidget;
class QWebEngineUrlRequestInfo;

class NetworkTrafficSidebar : public SideBarInterface {
    Q_OBJECT

public:
	explicit NetworkTrafficSidebar(QObject* p = 0);
	virtual ~NetworkTrafficSidebar();
    QString title() const;
    QAction* createMenuAction();
    QWidget* createSideBarWidget(BrowserWindow* mainWindow);

public slots:
	void clear();
	void newRequest(const QString&, const QString&, const QString&);

private:
    QWidget* w;
    QTableWidget* tbl;
    NetworkInterceptor* net;
};

#endif /* SRC_PLUGINS_NETWORKTRAFFIC_SRC_NETWORKTRAFFICSIDEBAR_H_ */
