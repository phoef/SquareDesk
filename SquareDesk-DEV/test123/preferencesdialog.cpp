#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);

    // musicPath preference -------
    QSettings MySettings;
    musicPath = MySettings.value("musicPath").toString();
    ui->musicPath->setText(musicPath);

    // Timers tab (experimental) preference -------
    experimentalTimersTabEnabled = MySettings.value("experimentalTimersTabEnabled").toString();
//    qDebug() << "preferencesDialog, expTimers = " << experimentalTimersTabEnabled;  // FIX
    if (experimentalTimersTabEnabled == "true") {
        ui->EnableTimersTabCheckbox->setChecked(true);
    } else {
        ui->EnableTimersTabCheckbox->setChecked(false);
    }

    // Cuesheet tab (experimental) preference -------
    experimentalCuesheetTabEnabled = MySettings.value("experimentalCuesheetTabEnabled").toString();
//    qDebug() << "preferencesDialog, expTimers = " << experimentalCuesheetTabEnabled;  // FIX
    if (experimentalCuesheetTabEnabled == "true") {
        ui->EnableCuesheetTabCheckbox->setChecked(true);
    } else {
        ui->EnableCuesheetTabCheckbox->setChecked(false);
    }

    // Pitch/Tempo View (experimental) preference -------
    experimentalPitchTempoViewEnabled = MySettings.value("experimentalPitchTempoViewEnabled").toString();
//    qDebug() << "preferencesDialog, expTimers = " << experimentalPitchTempoViewEnabled;  // FIX
    if (experimentalPitchTempoViewEnabled == "true") {
        ui->EnablePitchTempoViewCheckbox->setChecked(true);
    } else {
        ui->EnablePitchTempoViewCheckbox->setChecked(false);
    }

}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::on_chooseMusicPathButton_clicked()
{
    QString dir =
        QFileDialog::getExistingDirectory(this, tr("Select Base Directory for Music"),
                                                 QDir::homePath(),
                                                 QFileDialog::ShowDirsOnly
                                                 | QFileDialog::DontResolveSymlinks);

    if (dir.isNull()) {
//        qDebug() << "User cancelled.";
        return;  // user cancelled the "Select Base Directory for Music" dialog...so don't do anything, just return
    }

//    qDebug() << "User selected directory: " << dir;
    ui->musicPath->setText(dir);
    musicPath = dir;
    // NOTE: saving of Preferences is done at the dialog caller site, not here.
}

void PreferencesDialog::on_EnableTimersTabCheckbox_toggled(bool checked)
{
    if (checked) {
        experimentalTimersTabEnabled = "true";
    } else {
        experimentalTimersTabEnabled = "false";
    }
//    qDebug() << "User selected timers tab: " << experimentalTimersTabEnabled;
    // NOTE: saving of Preferences is done at the dialog caller site, not here.
}

void PreferencesDialog::on_EnableCuesheetTabCheckbox_toggled(bool checked)
{
    if (checked) {
        experimentalCuesheetTabEnabled = "true";
    } else {
        experimentalCuesheetTabEnabled = "false";
    }
//    qDebug() << "User selected timers tab: " << experimentalCuesheetTabEnabled;
    // NOTE: saving of Preferences is done at the dialog caller site, not here.
}

void PreferencesDialog::on_EnablePitchTempoViewCheckbox_toggled(bool checked)
{
    if (checked) {
        experimentalPitchTempoViewEnabled = "true";
    } else {
        experimentalPitchTempoViewEnabled = "false";
    }
//    qDebug() << "User selected Pitch Tempo View Enabled: " << experimentalPitchTempoViewEnabled;
    // NOTE: saving of Preferences is done at the dialog caller site, not here.
}

void PreferencesDialog::on_checkBoxReverseLabelTitle_toggled(bool checked)
{
    if (checked) {
        reverseLabelTitle = "true";
    } else {
        reverseLabelTitle = "false";
    }
//    qDebug() << "User selected Pitch Tempo View Enabled: " << experimentalPitchTempoViewEnabled;
    // NOTE: saving of Preferences is done at the dialog caller site, not here.
}
