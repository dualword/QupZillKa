/* ============================================================
* MailHandle plugin for QupZilla
* Copyright (C) 2012-2014  David Rosca <nowrep@gmail.com>
* Copyright (C) 2012-2014  Mladen Pejaković <pejakm@autistici.org>
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
#ifndef MAILHANDLE_SETTINGS_H
#define MAILHANDLE_SETTINGS_H

#include <QDialog>

namespace Ui
{
class MailHandle_Settings;
}

class MailHandle_SchemeHandler;

class MailHandle_Settings : public QDialog
{
    Q_OBJECT

public:
    explicit MailHandle_Settings(MailHandle_SchemeHandler* schemehandler, QWidget* parent = 0);
    ~MailHandle_Settings();

private slots:
    void dialogAccepted();
    void mhserviceChanged(int value);

private:
    Ui::MailHandle_Settings* ui;

    MailHandle_SchemeHandler* m_schemehandler;
    QString m_settingsFile;
};

#endif // MAILHANDLE_SETTINGS_H
