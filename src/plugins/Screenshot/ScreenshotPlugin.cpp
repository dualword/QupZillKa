/* QupZillKa (2021-2025) https://github.com/dualword/QupZillKa License:GNU GPL v3*/

#include "ScreenshotPlugin.h"
#include "pluginproxy.h"
#include "mainapplication.h"
#include "browserwindow.h"
#include "webview.h"
#include "webpage.h"

#include <QMenu>
#include <QDir>
#include <QMessageBox>
#include <QPainter>

ScreenshotPlugin::ScreenshotPlugin() : QObject() {

}

PluginSpec ScreenshotPlugin::pluginSpec() {
    PluginSpec spec;
    spec.name = "Screenshot Plugin";
    spec.info = "Screenshot plugin";
    spec.description = "Take a screenshot";
    spec.version = "0.1.0";
    spec.author = "";
    spec.icon = QPixmap(":qupzilla.png");
    spec.hasSettings = false;
    return spec;
}

void ScreenshotPlugin::init(InitState state, const QString &settingsPath) {
    Q_UNUSED(state); Q_UNUSED(settingsPath);
}

bool ScreenshotPlugin::testPlugin() {
    return (Qz::VERSION == QLatin1String(QUPZILLA_VERSION));
}

void ScreenshotPlugin::populateWebViewMenu(QMenu* menu, WebView* view, const WebHitTestResult &r) {
    m_view = view;
    if (r.linkUrl().isEmpty() && r.imageUrl().isEmpty()
    		&& !r.isContentSelected() && !r.isContentEditable() && r.mediaUrl().isEmpty()) {
        menu->addAction(tr("Screenshot"), this, &ScreenshotPlugin::takeScreenshot);
    }
}

void ScreenshotPlugin::takeScreenshot() {
	QSize size = m_view->size();
	QImage image = QImage(size.width(), size.height(), QImage::Format_ARGB32);
	QScopedPointer<QPainter> p(new QPainter(&image));
	m_view->render(p.get());

	QString name (QDir::homePath() + QDir::separator() + "QupZillKa_Screenshot_"
			+ QDate::currentDate().toString("ddMMMyyyy") + "_" + QTime::currentTime().toString() + ".png");
	QMessageBox msgBox;
	msgBox.setWindowTitle("Do you want to save the Screenshot?");
	msgBox.setText(name);
	msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
	msgBox.setDefaultButton(QMessageBox::Cancel);
	if (msgBox.exec() == QMessageBox::Save) image.save(name);

}
