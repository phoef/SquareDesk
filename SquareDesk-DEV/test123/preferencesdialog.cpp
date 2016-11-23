#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"
#include "QColorDialog"

static void  SetTimerPulldownValuesToFirstDigit(QComboBox *comboBox)
{
    for (int i = 0; i < comboBox->count(); ++i)
    {
        QString theText = comboBox->itemText(i);
        QStringList sl1 = theText.split(" ");
        QString mText = sl1[0];
        int length = mText.toInt();
        comboBox->setItemData(i, QVariant(length));
    }
}

static void SetPulldownValuesToItemNumberPlusOne(QComboBox *comboBox)
{
    for (int i = 0; i < comboBox->count(); ++i)
    {
        comboBox->setItemData(i, QVariant(i + 1));
    }
}



PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);
#if 0
//    qDebug() << "PreferencesDialog::PreferencesDialog()";
    // musicPath preference -------
    QSettings MySettings;
    musicPath = MySettings.value("musicPath").toString();
    ui->musicPath->setText(musicPath);

    // Timers tab (experimental) preference -------
    experimentalTimersTabEnabled = MySettings.value("experimentalTimersTabEnabled").toString();
//    qDebug() << "preferencesDialog, expTimers = " << experimentalTimersTabEnabled;  // FIX
    if (experimentalTimersTabEnabled == "true") {
        ui->EnableTimersTabCheckbox->setChecked(true);
    }
    else {
        ui->EnableTimersTabCheckbox->setChecked(false);
    }

    // Pitch/Tempo View (experimental) preference -------
    experimentalPitchTempoViewEnabled = MySettings.value("experimentalPitchTempoViewEnabled").toString();
//    qDebug() << "preferencesDialog, expTimers = " << experimentalPitchTempoViewEnabled;  // FIX
    if (experimentalPitchTempoViewEnabled == "true") {
        ui->EnablePitchTempoViewCheckbox->setChecked(true);
    }
    else {
        ui->EnablePitchTempoViewCheckbox->setChecked(false);
    }

    // Clock coloring (experimental) preference -------
    experimentalClockColoringEnabled = MySettings.value("experimentalClockColoringEnabled").toString();
//    qDebug() << "preferencesDialog, expTimers = " << experimentalPitchTempoViewEnabled;  // FIX
    if (experimentalClockColoringEnabled == "true") {
        ui->EnableClockColoring->setChecked(true);
    }
    else {
        ui->EnableClockColoring->setChecked(false);
    }

    QString value;

    value = MySettings.value("SongPreferencesInConfig").toString();
    if (value == "true") {
        ui->checkBoxSaveSongPreferencesInConfig->setChecked(true);
    }
    else {
        ui->checkBoxSaveSongPreferencesInConfig->setChecked(false);
    }


    value = MySettings.value("MusicTypeSinging").toString();
    if (!value.isNull()) {
        ui->lineEditMusicTypeSinging->setText(value);
    }

    value = MySettings.value("MusicTypePatter").toString();
    if (!value.isNull()) {
        ui->lineEditMusicTypePatter->setText(value);
    }

    value = MySettings.value("MusicTypeExtras").toString();
    if (!value.isNull()) {
        ui->lineEditMusicTypeExtras->setText(value);
    }

    value = MySettings.value("MusicTypeCalled").toString();
    if (!value.isNull()) {
        ui->lineEditMusicTypeCalled->setText(value);
    }


    enum SongFilenameMatchingType songFilenameFormat = SongFilenameLabelDashName;
    if (!MySettings.value("SongFilenameFormat").isNull()) {
        songFilenameFormat = (SongFilenameMatchingType)(MySettings.value("SongFilenameFormat").toInt());
    }
#endif
    
#if 0
    for (int i = 0; i < ui->comboBoxMusicFormat->maxCount(); ++i) {
        if (songFilenameFormat == ui->comboBoxMusicFormat->itemData(i).toInt()) {
            ui->comboBoxMusicFormat->setCurrentIndex(i);
            break;
        }
    }
#endif
    setFontSizes();
//    setColorSwatches("#ff0000","#00ff00","#0000ff","#ffffff");  // DO NOT DO THIS HERE. PARENT MUST DO THIS.

    // settings for experimental break/tip timers are:
    // tipLengthTimerEnabled
    // tipLengthTimerLength
    // tipLengthAlarmAction
    // breakLengthTimerEnabled
    // breakLengthTimerLength
    // breakLengthAlarmAction
#if 0
    ui->longTipCheckbox->setChecked(MySettings.value("tipLengthTimerEnabled") == "true");
    tipLengthTimerEnabledString = MySettings.value("tipLengthTimerEnabled").toString();

    unsigned int tt = MySettings.value("tipLengthTimerLength").toInt();
    ui->longTipLength->setCurrentIndex(tt-5);
    tipLength = tt;

    unsigned int tt2 = MySettings.value("tipLengthAlarmAction").toInt();
    ui->afterLongTipAction->setCurrentIndex(tt2);
    tipAlarmAction = tt2;

    ui->breakTimerCheckbox->setChecked(MySettings.value("breakLengthTimerEnabled") == "true");
    breakLengthTimerEnabledString = MySettings.value("breakLengthTimerEnabled").toString();

    unsigned int bb = MySettings.value("breakLengthTimerLength").toInt();
    ui->breakLength->setCurrentIndex(bb/5 - 1);
    breakLength = bb;

    unsigned int bb2 = MySettings.value("breakLengthAlarmAction").toInt();
    ui->afterBreakAction->setCurrentIndex(bb2);
    breakAlarmAction = bb2;
#endif
    
    SetTimerPulldownValuesToFirstDigit(ui->breakLength);
    SetTimerPulldownValuesToFirstDigit(ui->longTipLength);
    SetPulldownValuesToItemNumberPlusOne(ui->comboBoxMusicFormat);    
}


PreferencesDialog::~PreferencesDialog()
{
//    qDebug() << "    PreferencesDialog::~PreferencesDialog()";
    delete ui;
}

void PreferencesDialog::setColorSwatches(QString patter, QString singing, QString called, QString extras) {
    const QString COLOR_STYLE("QPushButton { background-color : %1; color : %2; }");

    QColor patterColor1(patter);
    patterColor = patterColor1;
    ui->patterColorButton->setStyleSheet(COLOR_STYLE.arg(patterColor1.name()).arg(patterColor1.name()));
    ui->patterColorButton->setAutoFillBackground(true);
    ui->patterColorButton->setFlat(true);

    QColor singingColor1(singing);
    singingColor = singingColor1;
    ui->singingColorButton->setStyleSheet(COLOR_STYLE.arg(singingColor1.name()).arg(singingColor1.name()));
    ui->singingColorButton->setAutoFillBackground(true);
    ui->singingColorButton->setFlat(true);

    QColor calledColor1(called);
    calledColor = calledColor1;
    ui->calledColorButton->setStyleSheet(COLOR_STYLE.arg(calledColor1.name()).arg(calledColor1.name()));
    ui->calledColorButton->setAutoFillBackground(true);
    ui->calledColorButton->setFlat(true);

    QColor extrasColor1(extras);
    extrasColor = extrasColor1;
    ui->extrasColorButton->setStyleSheet(COLOR_STYLE.arg(extrasColor1.name()).arg(extrasColor1.name()));
    ui->extrasColorButton->setAutoFillBackground(true);
    ui->extrasColorButton->setFlat(true);
}

void PreferencesDialog::setDefaultColors(QString patter, QString singing, QString called, QString extras) {
    defaultPatterColor=patter;
    defaultSingingColor=singing;
    defaultCalledColor=called;
    defaultExtrasColor=extras;
}


void PreferencesDialog::setFontSizes()
{

    int preferredSmallFontSize;
#if defined(Q_OS_MAC)
    preferredSmallFontSize = 11;
#elif defined(Q_OS_WIN32)
    preferredSmallFontSize = 8;
#elif defined(Q_OS_LINUX)
    preferredSmallFontSize = 9;
#endif

    QFont font = ui->musicDirHelpLabel->font();
    font.setPointSize(preferredSmallFontSize);

    ui->musicDirHelpLabel->setFont(font);
    ui->timersHelpLabel->setFont(font);
    ui->pitchTempoHelpLabel->setFont(font);
    ui->clockColoringHelpLabel->setFont(font);
    ui->musicTypesHelpLabel->setFont(font);
    ui->musicFormatHelpLabel->setFont(font);
    ui->saveSongPrefsHelpLabel->setFont(font);

}

void PreferencesDialog::on_chooseMusicPathButton_clicked()
{
    QString dir =
        QFileDialog::getExistingDirectory(this, tr("Select Base Directory for Music"),
                                          QDir::homePath(),
                                          QFileDialog::ShowDirsOnly
                                          | QFileDialog::DontResolveSymlinks);

    if (dir.isNull()) {
        return;  // user cancelled the "Select Base Directory for Music" dialog...so don't do anything, just return
    }

    ui->musicPath->setText(dir);
    musicPath = dir;
    // NOTE: saving of Preferences is done at the dialog caller site, not here.
}

// ------------

void PreferencesDialog::on_calledColorButton_clicked()
{
    QColor chosenColor = QColorDialog::getColor(calledColor, this); //return the color chosen by user
    if (chosenColor.isValid()) {
        calledColor = chosenColor;

        if (calledColor.name() == "#ffffff") {
            calledColor = defaultCalledColor;  // a way to reset the colors individually
        }

        const QString COLOR_STYLE("QPushButton { background-color : %1; color : %2; }");
        ui->calledColorButton->setStyleSheet(COLOR_STYLE.arg(calledColor.name()).arg(calledColor.name()));
        ui->calledColorButton->setAutoFillBackground(true);
        ui->calledColorButton->setFlat(true);
    }
}

void PreferencesDialog::on_extrasColorButton_clicked()
{
    QColor chosenColor = QColorDialog::getColor(extrasColor, this); //return the color chosen by user
    if (chosenColor.isValid()) {
        extrasColor = chosenColor;

        if (extrasColor.name() == "#ffffff") {
            extrasColor = defaultExtrasColor;  // a way to reset the colors individually
        }

        const QString COLOR_STYLE("QPushButton { background-color : %1; color : %2; }");
        ui->extrasColorButton->setStyleSheet(COLOR_STYLE.arg(extrasColor.name()).arg(extrasColor.name()));
        ui->extrasColorButton->setAutoFillBackground(true);
        ui->extrasColorButton->setFlat(true);
    }
}

// TODO: I wonder if this was my problem...I was creating the preferences dialog with new() and exec()...FIX THIS?
//  http://stackoverflow.com/questions/25315408/how-to-cancel-out-of-qcolordialoggetcolor

void PreferencesDialog::on_patterColorButton_clicked()
{
    QColor chosenColor = QColorDialog::getColor(patterColor, this); //return the color chosen by user
    if (chosenColor.isValid()) {
        patterColor = chosenColor;

        if (patterColor.name() == "#ffffff") {
            patterColor = defaultPatterColor;  // a way to reset the colors individually
        }

        const QString COLOR_STYLE("QPushButton { background-color : %1; color : %2; }");
        ui->patterColorButton->setStyleSheet(COLOR_STYLE.arg(patterColor.name()).arg(patterColor.name()));
        ui->patterColorButton->setAutoFillBackground(true);
        ui->patterColorButton->setFlat(true);
    }
}

void PreferencesDialog::on_singingColorButton_clicked()
{
    QColor chosenColor = QColorDialog::getColor(singingColor, this); //return the color chosen by user
    if (chosenColor.isValid()) {
        singingColor = chosenColor;

        if (singingColor.name() == "#ffffff") {
            singingColor = defaultSingingColor;  // a way to reset the colors individually
        }

        const QString COLOR_STYLE("QPushButton { background-color : %1; color : %2; }");
        ui->singingColorButton->setStyleSheet(COLOR_STYLE.arg(singingColor.name()).arg(singingColor.name()));
        ui->singingColorButton->setAutoFillBackground(true);
        ui->singingColorButton->setFlat(true);
    }
}



/* See the large comment at the top of prefs_options.h */

#define CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(name,default)
#define CONFIG_ATTRIBUTE_STRING_NO_PREFS(name,default)
#define CONFIG_ATTRIBUTE_STRING(control, name, default)                 \
    QString PreferencesDialog::Get##name() { return ui->control->text(); } \
    void PreferencesDialog::Set##name(QString value) { ui->control->setText(value); }

#define CONFIG_ATTRIBUTE_BOOLEAN(control, name, default) \
    bool PreferencesDialog::Get##name() { return ui->control->isChecked(); } \
    void PreferencesDialog::Set##name(bool value) { ui->control->setChecked(value); }

#define CONFIG_ATTRIBUTE_COMBO(control, name, default) \
    int PreferencesDialog::Get##name() { return ui->control->itemData(ui->control->currentIndex()).toInt(); } \
    void PreferencesDialog::Set##name(int value) \
    { for (int i = 0; i < ui->control->count(); ++i) { \
            if (ui->control->itemData(i).toInt() == value) { ui->control->setCurrentIndex(value); break; } \
        } }
#include "prefs_options.h"
#undef CONFIG_ATTRIBUTE_STRING
#undef CONFIG_ATTRIBUTE_BOOLEAN
#undef CONFIG_ATTRIBUTE_COMBO
#undef CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS
#undef CONFIG_ATTRIBUTE_STRING_NO_PREFS
