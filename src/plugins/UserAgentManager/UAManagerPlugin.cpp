/* QupZillKa (2021-2026) https://github.com/dualword/QupZillKa License:GNU GPL v3*/

#include "UAManagerPlugin.h"
#include "abstractbuttoninterface.h"
#include "browserwindow.h"
#include "statusbar.h"
#include "mainapplication.h"
#include "preferences.h"
#include "pluginproxy.h"
#include "settings.h"

#include <QTranslator>

class UAM_Button : public AbstractButtonInterface {
public:
    explicit UAM_Button(QObject *p = nullptr) : AbstractButtonInterface(p) {
    }

    QString id() const override {
        return QSL("uam-icon");
    }

    QString name() const override {
        return tr("User Agent Manager button");
    }
};

UAManagerPlugin::UAManagerPlugin() : QObject() {

}

PluginSpec UAManagerPlugin::pluginSpec() {
    PluginSpec spec;
    spec.name = "User Agent manager Plugin";
    spec.info = "User Agent manager Plugin";
    spec.description = "Shows icon and current User Agent in status bar";
    spec.version = "0.1.0";
    spec.author = "";
    spec.icon = QPixmap(":/useragentmanager/data/UAplugin.png");
    spec.hasSettings = false;
    return spec;
}

void UAManagerPlugin::init(InitState state, const QString &settingsPath) {
    Q_UNUSED(state); Q_UNUSED(settingsPath);
    connect(mApp->plugins(), SIGNAL(mainWindowCreated(BrowserWindow*)), this, SLOT(mainWindowCreated(BrowserWindow*)));
    connect(mApp->plugins(), SIGNAL(mainWindowDeleted(BrowserWindow*)), this, SLOT(mainWindowDeleted(BrowserWindow*)));
    connect(mApp, SIGNAL(settingsReloaded()), SLOT(updateSettings()));

    if (state == LateInitState) {
        foreach (BrowserWindow* window, mApp->windows()) {
            mainWindowCreated(window);
        }
    }
}

void UAManagerPlugin::unload() {
    foreach (BrowserWindow* window, mApp->windows()) {
        mainWindowDeleted(window);
    }
}

bool UAManagerPlugin::testPlugin() {
    return (Qz::VERSION == QLatin1String(QUPZILLA_VERSION));
}

QTranslator* UAManagerPlugin::getTranslator(const QString &locale) {
    QTranslator* translator = new QTranslator(this);
    translator->load(locale, ":/useragentmanager/locale/");
    return translator;
}

void UAManagerPlugin::showSettings() {
	Settings settings;
    settings.beginGroup("Browser-View-Settings");
    settings.setValue("settingsDialogPage", 12);
    settings.endGroup();
    BrowserWindow* w = m_icons.keys()[0];
	if (!m_preferences) m_preferences = new Preferences(w);
	m_preferences->open();
}

AbstractButtonInterface* UAManagerPlugin::createStatusBarIcon(BrowserWindow* mainWindow) {
    if (m_icons.contains(mainWindow)) {
        return m_icons.value(mainWindow);
    }

    UAM_Button *icon = new UAM_Button(this);
    icon->setIcon(QIcon(QSL(":/useragentmanager/data/UAplugin.png")));
    connect(icon, &AbstractButtonInterface::clicked, this, &UAManagerPlugin::showSettings);
    m_icons.insert(mainWindow, icon);
    updateSettings();
    return icon;
}

void UAManagerPlugin::mainWindowCreated(BrowserWindow* window) {
	window->statusBar()->addButton(createStatusBarIcon(window));
}

void UAManagerPlugin::mainWindowDeleted(BrowserWindow* window) {
    window->statusBar()->removeButton(m_icons.value(window));
    delete m_icons.value(window);
    m_icons.remove(window);
}

void UAManagerPlugin::updateSettings() {
    foreach (AbstractButtonInterface* icon, m_icons.values()) {
//        Settings settings;
//        settings.beginGroup("Web-Browser-Settings");
//        icon->setToolTip(settings.value("UserAgent", QString()).toString());
//        settings.endGroup();
        icon->setToolTip("Open User Agent settings");
    }
}
