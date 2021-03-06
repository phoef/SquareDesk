/****************************************************************************
**
** Copyright (C) 2016, 2017 Mike Pogue, Dan Lyke
** Contact: mpogue @ zenstarstudio.com
**
** This file is part of the SquareDesk/SquareDeskPlayer application.
**
** $SQUAREDESK_BEGIN_LICENSE$
**
** Commercial License Usage
** For commercial licensing terms and conditions, contact the authors via the
** email address above.
**
** GNU General Public License Usage
** This file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appear in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.
**
** $SQUAREDESK_END_LICENSE$
**
****************************************************************************/

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QFileDialog>
#include <QDir>
#include <QDebug>
#include <QSettings>
#include <QValidator>

#include "common_enums.h"
#include "default_colors.h"

namespace Ui
{
class PreferencesDialog;
}

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = 0);
    ~PreferencesDialog();

    QString musicPath;
    void setFontSizes();

    QValidator *validator;

//    void setColorSwatches(QString patter, QString singing, QString called, QString extras);
//    QColor patterColor;
//    QColor singingColor;
//    QColor calledColor;
//    QString calledColor;
//    QColor extrasColor;

//    void setDefaultColors(QString patter, QString singing, QString called, QString extras);
//    QString defaultPatterColor, defaultSingingColor, defaultCalledColor, defaultExtrasColor;

/* See the large comment at the top of prefs_options.h */

#define CONFIG_ATTRIBUTE_STRING_NO_PREFS(name,default)
#define CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(name,default)
#define CONFIG_ATTRIBUTE_STRING(control, name, default) QString Get##name(); void Set##name(QString value);
#define CONFIG_ATTRIBUTE_BOOLEAN(control, name, default) bool Get##name(); void Set##name(bool value);
#define CONFIG_ATTRIBUTE_COMBO(control, name, default) int Get##name(); void Set##name(int value);
#define CONFIG_ATTRIBUTE_COLOR(control, name, default) QString Get##name(); void Set##name(QString value);
#define CONFIG_ATTRIBUTE_INT(control, name, default) int Get##name(); void Set##name(int value);
    #include "prefs_options.h"
#undef CONFIG_ATTRIBUTE_STRING
#undef CONFIG_ATTRIBUTE_BOOLEAN
#undef CONFIG_ATTRIBUTE_COMBO
#undef CONFIG_ATTRIBUTE_COLOR
#undef CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS
#undef CONFIG_ATTRIBUTE_STRING_NO_PREFS
#undef CONFIG_ATTRIBUTE_INT

private slots:
    void on_chooseMusicPathButton_clicked();

    void on_calledColorButton_clicked();
    void on_extrasColorButton_clicked();
    void on_patterColorButton_clicked();
    void on_singingColorButton_clicked();

    void on_initialBPMLineEdit_textChanged(const QString &arg1);

private:
    Ui::PreferencesDialog *ui;
};

#endif // PREFERENCESDIALOG_H
