/* QupZillKa (2021-2026) https://github.com/dualword/QupZillKa License:GNU GPL v3*/

#ifndef SRC_PLUGINS_USERAGENTMANAGER_UAMANAGERPLUGIN_H_
#define SRC_PLUGINS_USERAGENTMANAGER_UAMANAGERPLUGIN_H_

#include "plugininterface.h"

class AbstractButtonInterface;
class BrowserWindow;
class Preferences;

class UAManagerPlugin : public QObject, public PluginInterface {
    Q_OBJECT
    Q_INTERFACES(PluginInterface)
    Q_PLUGIN_METADATA(IID "QupZilla.Browser.plugin.UAManagerPlugin")

public:
	explicit UAManagerPlugin();
    PluginSpec pluginSpec();
    void init(InitState state, const QString &settingsPath);
    void unload();
    bool testPlugin();
    QTranslator* getTranslator(const QString &locale);

public slots:
    void mainWindowCreated(BrowserWindow* window);
    void mainWindowDeleted(BrowserWindow* window);
    void updateSettings();

private slots:
	void showSettings();

private:
    AbstractButtonInterface* createStatusBarIcon(BrowserWindow* mainWindow);
    QHash<BrowserWindow*, AbstractButtonInterface*> m_icons;
    QPointer<Preferences> m_preferences;
};

#endif /* SRC_PLUGINS_USERAGENTMANAGER_UAMANAGERPLUGIN_H_ */
