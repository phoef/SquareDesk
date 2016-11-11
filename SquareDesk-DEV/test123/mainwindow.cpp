#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "utility.h"
#include "tablenumberitem.h"
#include "QMap"
#include "QMapIterator"
#include "QThread"
#include "QProcess"
#include "QDesktopWidget"

// BUG: Cmd-K highlights the next row, and hangs the app
// BUG: searching then clearing search will lose selection in songTable
// BUG: NL allowed in the search fields, makes the text disappear until DEL pressed

#include <iostream>
#include <sstream>
#include <iomanip>
using namespace std;

// =================================================================================================
// SquareDeskPlayer Keyboard Shortcuts:
//
// function                 MAC                  		PC                                SqView
// -------------------------------------------------------------------------------------------------
// FILE MENU
// Open Audio file          Cmd-O                		Ctrl-O, Alt-F-O
// Save                     Cmd-S                		Ctrl-S, Alt-F-S
// Save as                  Shft-Cmd-S           		Alt-F-A
// Quit                     Cmd-Q                		Ctrl-F4, Alt-F-E
//
// PLAYLIST MENU
// Load Playlist
// Save Playlist
// Next Song                K                            K                                K
// Prev Song
//
// MUSIC MENU
// play/pause               space                		space, Alt-M-P                    space
// rewind/stop              S, ESC, END, Cmd-.   		S, ESC, END, Alt-M-S, Ctrl-.
// rewind/play (playing)    HOME, . (while playing) 	HOME, .  (while playing)          .
// skip/back 5 sec          Cmd-RIGHT/LEFT,RIGHT/LEFT   Ctrl-RIGHT/LEFT, RIGHT/LEFT,
//                                                        Alt-M-A/Alt-M-B
// volume up/down           Cmd-UP/DOWN,UP/DOWN         Ctrl-UP/DOWN, UP/DOWN             N/B
// mute                     Cmd-M, M                	Ctrl-M, M
// go faster                Cmd-+,+,=            		Ctrl-+,+,=                        R
// go slower                Cmd--,-              		Ctrl--,-                          E
// force mono                                    		Alt-M-F
// clear search             Cmd-/                		Alt-M-S
// pitch up                 Cmd-U, U                	Ctrl-U, U, Alt-M-U                F
// pitch down               Cmd-D, D                	Ctrl-D, D, Alt-M-D                D

// GLOBALS:
bass_audio cBass;

// ----------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    timerCountUp(NULL),
    timerCountDown(NULL),
    trapKeypresses(true),
    reverseLabelTitle(false)
{
    songLoaded = false;     // no song is loaded, so don't update the currentLocLabel

    ui->setupUi(this);
    ui->statusBar->showMessage("");

    this->setWindowTitle(QString("SquareDesk Music Player/Editor"));

    ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));

    ui->playButton->setEnabled(false);
    ui->stopButton->setEnabled(false);

    ui->nextSongButton->setEnabled(false);
    ui->previousSongButton->setEnabled(false);

    // ============
    ui->menuFile->addSeparator();

    // ------------
    // NOTE: MAC OS X ONLY
#if defined(Q_OS_MAC)
    QAction *aboutAct = new QAction(QIcon(), tr("&About SquareDesk..."), this);
    aboutAct->setStatusTip(tr("SquareDesk Information"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(aboutBox()));
    ui->menuFile->addAction(aboutAct);
#endif

    // ==============
    // HELP MENU IS WINDOWS ONLY
#if defined(Q_OS_WIN)
    QMenu *helpMenu = new QMenu("&Help");

    // ------------
    QAction *aboutAct2 = new QAction(QIcon(), tr("About &SquareDesk..."), this);
    aboutAct2->setStatusTip(tr("SquareDesk Information"));
    connect(aboutAct2, SIGNAL(triggered()), this, SLOT(aboutBox()));
    helpMenu->addAction(aboutAct2);
    menuBar()->addAction(helpMenu->menuAction());
#endif

#if defined(Q_OS_WIN)
    delete ui->mainToolBar; // remove toolbar on WINDOWS (not present on Mac)
#endif

    // ------------
#if defined(Q_OS_WIN)
    // NOTE: WINDOWS ONLY
    closeAct = new QAction(QIcon(), tr("&Exit"), this);
    closeAct->setShortcuts(QKeySequence::Close);
    closeAct->setStatusTip(tr("Exit the program"));
    connect(closeAct, SIGNAL(triggered()), this, SLOT(close()));
    ui->menuFile->addAction(closeAct);
#endif

    currentState = kStopped;
    currentPitch = 0;
    tempoIsBPM = false;

    notSorted = true;

    Info_Seekbar(false);

    // setup playback timer
    UIUpdateTimer = new QTimer(this);
    connect(UIUpdateTimer, SIGNAL(timeout()), this, SLOT(on_UIUpdateTimerTick()));
    UIUpdateTimer->start(1000);           //adjust from GUI with timer->setInterval(newValue)

    closeEventHappened = false;

    ui->songTable->clearSelection();
    ui->songTable->clearFocus();

    //Create Bass audio system
    cBass.Init();

    //Set UI update
    cBass.SetVolume(100);
    currentVolume = 100;
    Info_Volume();

    // VU Meter
    vuMeterTimer = new QTimer(this);
    connect(vuMeterTimer, SIGNAL(timeout()), this, SLOT(on_vuMeterTimerTick()));
    vuMeterTimer->start(50);           // adjust from GUI with timer->setInterval(newValue)

    vuMeter = new LevelMeter(this);
//    int x = ui->vuMeterProxy->x();
//    int y = ui->vuMeterProxy->y();
//    int w = ui->vuMeterProxy->width();
//    int h = ui->vuMeterProxy->height();
////    qDebug() << x << y << w << h;
//    vuMeter->setGeometry(x,y,100,h);
//    vuMeter->setFixedSize(QSize(10,100));
////    vuMeter->setGeometry(100,100,500,500);

    ui->gridLayout_2->addWidget(vuMeter, 1,5);  // add it to the layout in the right spot
    vuMeter->setFixedHeight(20);

    vuMeter->reset();
//    vuMeter->levelChanged(0.5,0.7,100);
    vuMeter->setEnabled(true);
    vuMeter->setVisible(true);

//    ui->vuMeterProxy->setHidden(true);

    // where is the root directory where all the music is stored?
    pathStack = new QList<QString>();

    QSettings MySettings; // Will be using application information for correct location of your settings
    musicRootPath = MySettings.value("musicPath").toString();
    if (musicRootPath.isNull()) {
        musicRootPath = QDir::homePath() + "/music";
        MySettings.setValue("musicPath", musicRootPath); // set to music subdirectory in user's Home directory, if nothing else
    }

    // used to store the file paths
    findMusic();  // get the filenames from the user's directories
    filterMusic(); // and filter them into the songTable

    ui->songTable->setColumnWidth(kNumberCol,36);
    ui->songTable->setColumnWidth(kTypeCol,96);
    ui->songTable->setColumnWidth(kLabelCol,80);
//  kTitleCol is always expandable, so don't set width here
    ui->songTable->setColumnWidth(kPitchCol,50);
    ui->songTable->setColumnWidth(kTempoCol,50);

    // ----------
    const QString EXPERIMENTALPITCHTEMPO_KEY("experimentalPitchTempoViewEnabled");  // default is not enabled
    QString pitchTempoViewEnabled = MySettings.value(EXPERIMENTALPITCHTEMPO_KEY).toString();
//    qDebug() << "pitchTempoViewEnabled" << pitchTempoViewEnabled;
    pitchAndTempoHidden = (pitchTempoViewEnabled != "true");  // if blank (not set), will be remembered as false

    updatePitchTempoView(); // update the actual view of these 2 columns in the songTable

    // -----------
    const QString AUTOSTART_KEY("autostartplayback");  // default is AUTOSTART ENABLED
    QString autoStartChecked = MySettings.value(AUTOSTART_KEY).toString();

    if (autoStartChecked.isNull()) {
        // first time through, AUTOPLAY is UNCHECKED
        autoStartChecked = "unchecked";
        MySettings.setValue(AUTOSTART_KEY, "unchecked");
    }

    if (autoStartChecked == "checked") {
        ui->actionAutostart_playback->setChecked(true);
    } else {
        ui->actionAutostart_playback->setChecked(false);
    }

    // -----------

    restoreCheckBoxState("startplaybackoncountdowntimer", ui->checkBoxPlayOnEnd, false);
    restoreCheckBoxState("startcountuptimeronplay", ui->checkBoxStartOnPlay, false);


    // -------
    const QString FORCEMONO_KEY("forcemono");  // default is FALSE (use stereo)
    QString forceMonoChecked = MySettings.value(FORCEMONO_KEY).toString();

    if (forceMonoChecked.isNull()) {
        // first time through, FORCE MONO is FALSE (stereo mode is the default)
        forceMonoChecked = "false";  // FIX: needed?
        MySettings.setValue(FORCEMONO_KEY, "false");
    }

    if (forceMonoChecked == "true") {
        ui->monoButton->setChecked(true);
        on_monoButton_toggled(true);  // sets button and menu item
    } else {
        ui->monoButton->setChecked(false);
        on_monoButton_toggled(false);  // sets button and menu item
    }

    // -------
    const QString REVERSELABELTITLE_KEY("reverselabeltitle");  // default is FALSE (use stereo)
    QString reverselabeltitleChecked = MySettings.value(REVERSELABELTITLE_KEY).toString();

    if (reverselabeltitleChecked.isNull()) {
        // first time through, FORCE MONO is FALSE (stereo mode is the default)
        reverselabeltitleChecked = "false";  // FIX: needed?
        MySettings.setValue(REVERSELABELTITLE_KEY, "false");
    }
    reverseLabelTitle = (reverselabeltitleChecked == "true") ? true : false;

    
    setFontSizes();

    // Volume, Pitch, and Mix can be set before loading a music file.  NOT tempo.
    ui->pitchSlider->setEnabled(true);
    ui->pitchSlider->setValue(0);
    ui->currentPitchLabel->setText("0 semitones");

    ui->volumeSlider->setEnabled(true);
    ui->volumeSlider->setValue(100);
    ui->currentVolumeLabel->setText("Max");

    ui->mixSlider->setEnabled(true);
    ui->mixSlider->setValue(0);
    ui->currentMixLabel->setText("50% L / 50% R");

    // ...and the EQ sliders, too...
    ui->bassSlider->setEnabled(true);
    ui->midrangeSlider->setEnabled(true);
    ui->trebleSlider->setEnabled(true);

    // in the Designer, these have values, making it easy to visualize there
    //   must clear those out, because a song is not loaded yet.
    ui->currentLocLabel->setText("");
    ui->songLengthLabel->setText("");

    inPreferencesDialog = false;

    // save info about the experimental timers tab
    // experimental timers tab is tab #1 (second tab)
    tabmap.insert(1,QPair<QWidget*,QString>(ui->tabWidget->widget(1),ui->tabWidget->tabText(1)));

    // ----------
    const QString EXPERIMENTALTIMERS_KEY("experimentalTimersTabEnabled");  // default is not enabled
    QString timersEnabled = MySettings.value(EXPERIMENTALTIMERS_KEY).toString();

    if (timersEnabled != "true") {
        ui->tabWidget->removeTab(1);  // it's remembered, don't worry!
    }
    ui->tabWidget->setCurrentIndex(0); // music tab is primary, regardless of last setting in Qt Designer

    // ----------
    connect(ui->songTable->horizontalHeader(),&QHeaderView::sectionResized,
            this, &MainWindow::columnHeaderResized);

    resize(QDesktopWidget().availableGeometry(this).size() * 0.55);  // initial size is 55% of screen
}

// ----------------------------------------------------------------------
MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setFontSizes() {
    //    // -------
    //    // FIX: This is partial code for changing the font size dynamically...
    //    const QString FONTSIZE_KEY("fontsize");  // default is 13
    //    QString fontsize = MySettings.value(FONTSIZE_KEY).toString();

    //    if (fontsize.isNull()) {
    //        // first time through, FORCE MONO is FALSE (stereo mode is the default)
    //        fontsize = "13";  // FIX: needed?
    //        MySettings.setValue(FONTSIZE_KEY, "13");
    //    }

    //    iFontsize = fontsize.toInt();
    //    qDebug() << "font size preference is:" << iFontsize;

    int preferredSmallFontSize;
#if defined(Q_OS_MAC)
    preferredSmallFontSize = 13;
#elif defined(Q_OS_WIN32)
    preferredSmallFontSize = 8;
#elif defined(Q_OS_LINUX)
    preferredSmallFontSize = 13;  // FIX: is this right?
#endif

    QFont font = ui->currentTempoLabel->font();
    font.setPointSize(preferredSmallFontSize);

    ui->tabWidget->setFont(font);  // most everything inherits from this one
    ui->statusBar->setFont(font);
    ui->currentLocLabel->setFont(font);
    ui->songLengthLabel->setFont(font);
    ui->bassLabel->setFont(font);
    ui->midrangeLabel->setFont(font);
    ui->trebleLabel->setFont(font);
    ui->EQgroup->setFont(font);

    font.setPointSize(preferredSmallFontSize-2);
    ui->loopButton->setFont(font);
    ui->monoButton->setFont(font);

    QString styleForCallerlabDefinitions("QLabel{font-size:12pt;}");
#if defined(Q_OS_WIN)
    styleForCallerlabDefinitions = "QLabel{font-size:6pt;}";
#endif
    ui->basicLabel1->setStyleSheet(styleForCallerlabDefinitions);
    ui->basicLabel2->setStyleSheet(styleForCallerlabDefinitions);
    ui->MainstreamLabel1->setStyleSheet(styleForCallerlabDefinitions);
    ui->MainstreamLabel2->setStyleSheet(styleForCallerlabDefinitions);
    ui->PlusLabel1->setStyleSheet(styleForCallerlabDefinitions);
    ui->PlusLabel2->setStyleSheet(styleForCallerlabDefinitions);

    font.setPointSize(preferredSmallFontSize+6);
    ui->nowPlayingLabel->setFont(font);
}

// ----------------------------------------------------------------------
void MainWindow::updatePitchTempoView() {
//    qDebug() << "updatePitchTempoView()";
    if (pitchAndTempoHidden) {
        ui->songTable->setColumnHidden(kPitchCol,true); // hide the pitch column
        ui->songTable->setColumnHidden(kTempoCol,true); // hide the tempo column
    } else {
//        ui->songTable->setColumnWidth(kTitleCol,450);  // FIX: this is a guess, and Title no longer expands/contracts with window size

        ui->songTable->setColumnHidden(kPitchCol,false); // show the pitch column
        ui->songTable->setColumnHidden(kTempoCol,false); // show the tempo column

        // http://www.qtcentre.org/threads/3417-QTableWidget-stretch-a-column-other-than-the-last-one
        QHeaderView *headerView = ui->songTable->horizontalHeader();
        headerView->setSectionResizeMode(kNumberCol, QHeaderView::Interactive);
        headerView->setSectionResizeMode(kTypeCol, QHeaderView::Interactive);
        headerView->setSectionResizeMode(kLabelCol, QHeaderView::Interactive);
        headerView->setSectionResizeMode(kTitleCol, QHeaderView::Stretch);
        headerView->setSectionResizeMode(kPitchCol, QHeaderView::Fixed);
        headerView->setSectionResizeMode(kTempoCol, QHeaderView::Fixed);
        headerView->setStretchLastSection(false);
    }
}


// ----------------------------------------------------------------------
void MainWindow::on_loopButton_toggled(bool checked)
{
    if (checked) {
        // regular text: "LOOP"
        QFont f = ui->loopButton->font();
        f.setStrikeOut(false);
        ui->loopButton->setFont(f);
        ui->loopButton->setText("LOOP");
        ui->actionLoop->setChecked(true);

        ui->seekBar->SetLoop(true);
        ui->seekBarCuesheet->SetLoop(true);

        double songLength = cBass.FileLength;
        cBass.SetLoop(songLength * 0.9, songLength * 0.1); // FIX: use parameters in the MP3 file

    }
    else {
        // strikethrough text: "LOOP"
        QFont f = ui->loopButton->font();
        f.setStrikeOut(true);
        ui->loopButton->setFont(f);
        ui->loopButton->setText("LOOP");
        ui->actionLoop->setChecked(false);

        ui->seekBar->SetLoop(false);
        ui->seekBarCuesheet->SetLoop(false);

        cBass.ClearLoop();
    }
}

// ----------------------------------------------------------------------
void MainWindow::on_monoButton_toggled(bool checked)
{
    if (checked) {
        ui->monoButton->setText("MONO");
        ui->actionForce_Mono_Aahz_mode->setChecked(true);
        cBass.SetMono(true);
    }
    else {
        ui->monoButton->setText("STEREO");
        ui->actionForce_Mono_Aahz_mode->setChecked(false);
        cBass.SetMono(false);
    }

    // the Force Mono (Aahz Mode) setting is persistent across restarts of the application
    QSettings MySettings; // Will be using application information for correct location of your settings
    const QString FORCEMONO_KEY("forcemono");  // default is AUTOSTART ENABLED

    if (ui->actionForce_Mono_Aahz_mode->isChecked()) {
        MySettings.setValue(FORCEMONO_KEY, "true");
    } else {
        MySettings.setValue(FORCEMONO_KEY, "false");
    }


}

// ----------------------------------------------------------------------
void MainWindow::on_stopButton_clicked()
{
    ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));  // change PAUSE to PLAY
    ui->actionPlay->setText("Play");  // now stopped, press Cmd-P to Play
    currentState = kStopped;

    cBass.Stop();  // Stop playback, rewind to the beginning

    ui->seekBar->setValue(0);
    ui->seekBarCuesheet->setValue(0);
    Info_Seekbar(false);  // update just the text
}

// ----------------------------------------------------------------------
void MainWindow::on_playButton_clicked()
{
    cBass.Play();  // currently paused, so start playing
    if (currentState == kStopped || currentState == kPaused) {
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));  // change PLAY to PAUSE
        ui->actionPlay->setText("Pause");
        currentState = kPlaying;
    }
    else {
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));  // change PAUSE to PLAY
        ui->actionPlay->setText("Play");
        currentState = kPaused;
    }
    if (ui->checkBoxStartOnPlay->isChecked())
    {
        on_pushButtonCountUpTimerStartStop_clicked();
    }
}



bool MainWindow::timerStopStartClick(QTimer *&timer, QPushButton *button)
{
    if (timer)
    {
        button->setText("Start");
        timer->stop();
        delete timer;
        timer = NULL;
    }
    else
    {
        button->setText("Stop");
        timer = new QTimer(this);
        timer->start(1000);
    }
    return NULL != timer;
}

int MainWindow::updateTimer(qint64 timeZeroEpochMs, QLabel *label)
{
    QDateTime now(QDateTime::currentDateTime());
    qint64 timeNowEpochMs = now.currentMSecsSinceEpoch();
    int signedSeconds = (int)((timeNowEpochMs - timeZeroEpochMs) / 1000);
    int seconds = signedSeconds;
    char sign = ' ';

    if (seconds < 0)
    {
        sign = '-';
        seconds = -seconds;
    }

    stringstream ss;
    int hours = seconds / (60*60);
    int minutes = (seconds / 60) % 60;

    ss << sign;
    if (hours)
        ss << hours << ":" << setw(2);
    ss << setfill('0') << minutes << ":" << setw(2) << setfill('0') << (seconds % 60);
    string s(ss.str());
    label->setText(s.c_str());
    return signedSeconds;
}

// ----------------------------------------------------------------------
void MainWindow::on_pushButtonCountDownTimerStartStop_clicked()
{
    if (timerStopStartClick(timerCountDown,
                            ui->pushButtonCountDownTimerStartStop))
    {
        on_pushButtonCountDownTimerReset_clicked();
        connect(timerCountDown, SIGNAL(timeout()), this, SLOT(timerCountDown_update()));
    }
}



// ----------------------------------------------------------------------

const qint64 timerJitter = 50;

void MainWindow::on_pushButtonCountDownTimerReset_clicked()
{
    QString offset(ui->lineEditCountDownTimer->text());

    int seconds = 0;
    int minutes = 0;
    bool found_colon = false;

    for (int i = 0; i < offset.length(); ++i)
    {
        int ch = offset[i].unicode();

        if (ch >= '0' && ch <= '9')
        {
            if (found_colon)
            {
                seconds *= 10;
                seconds += ch - '0';
            }
            else
            {
                minutes *= 10;
                minutes += ch - '0';
            }
        }
        else if (ch == ':')
        {
            found_colon = true;
        }
    }
    timeCountDownZeroMs = QDateTime::currentDateTime().currentMSecsSinceEpoch();
    timeCountDownZeroMs += (qint64)(minutes * 60 + seconds) * (qint64)(1000) + timerJitter;
    updateTimer(timeCountDownZeroMs, ui->labelCountDownTimer);
}

// ----------------------------------------------------------------------
void MainWindow::on_pushButtonCountUpTimerStartStop_clicked()
{
    if (timerStopStartClick(timerCountUp,
                            ui->pushButtonCountUpTimerStartStop))
    {
        on_pushButtonCountUpTimerReset_clicked();
        connect(timerCountUp, SIGNAL(timeout()), this, SLOT(timerCountUp_update()));
    }
}

// ----------------------------------------------------------------------
void MainWindow::on_pushButtonCountUpTimerReset_clicked()
{
    timeCountUpZeroMs = QDateTime::currentDateTime().currentMSecsSinceEpoch() + timerJitter;
    updateTimer(timeCountUpZeroMs, ui->labelCountUpTimer);
}

// ----------------------------------------------------------------------
void MainWindow::timerCountUp_update()
{
    updateTimer(timeCountUpZeroMs, ui->labelCountUpTimer);
}

// ----------------------------------------------------------------------
void MainWindow::timerCountDown_update()
{
    if (updateTimer(timeCountDownZeroMs, ui->labelCountDownTimer) >= 0
        && ui->checkBoxPlayOnEnd->isChecked()
        && currentState == kStopped)
    {
        on_playButton_clicked();
    }
}

// ----------------------------------------------------------------------
void MainWindow::on_pitchSlider_valueChanged(int value)
{
    cBass.SetPitch(value);
    currentPitch = value;
    QString plural;
    if (currentPitch == 1 || currentPitch == -1) {
        plural = "";
    }
    else {
        plural = "s";
    }
    QString sign = "";
    if (currentPitch > 0) {
        sign = "+";
    }
    ui->currentPitchLabel->setText(sign + QString::number(currentPitch) +" semitone" + plural);

    // update the hidden pitch column
    QItemSelectionModel* selectionModel = ui->songTable->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;
    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();
    } else {
        // FIX: more than 1 row or no rows at all selected (BAD)
        return;
    }

//    qDebug() << "pitchSliderChanged, setting text to " << currentPitch;
    ui->songTable->item(row, kPitchCol)->setText(QString::number(currentPitch));
}

// ----------------------------------------------------------------------
void MainWindow::Info_Volume(void)
{
    if (cBass.Stream_Volume == 0) {
        ui->currentVolumeLabel->setText("Mute");
    }
    else if (cBass.Stream_Volume == 100) {
        ui->currentVolumeLabel->setText("MAX");
    }
    else {
        ui->currentVolumeLabel->setText(QString::number(cBass.Stream_Volume)+"%");
    }
}

// ----------------------------------------------------------------------
void MainWindow::on_volumeSlider_valueChanged(int value)
{
    cBass.SetVolume(value);
    currentVolume = value;

    Info_Volume();

    if (value == 0) {
        ui->actionMute->setText("Un&mute");
    }
    else {
        ui->actionMute->setText("&Mute");
    }
}

// ----------------------------------------------------------------------
void MainWindow::on_actionMute_triggered()
{
    if (ui->volumeSlider->value() != 0) {
        previousVolume = ui->volumeSlider->value();
        ui->volumeSlider->setValue(0);
        ui->actionMute->setText("Un&mute");
    }
    else {
        ui->volumeSlider->setValue(previousVolume);
        ui->actionMute->setText("&Mute");
    }
}

// ----------------------------------------------------------------------
void MainWindow::on_tempoSlider_valueChanged(int value)
{
//    qDebug() << "on_tempoSlider_valueChanged: " << value;

    if (tempoIsBPM) {
        float baseBPM = (float)cBass.Stream_BPM;    // original detected BPM
        float desiredBPM = (float)value;            // desired BPM
        int newBASStempo = (int)(round(100.0*desiredBPM/baseBPM));
        cBass.SetTempo(newBASStempo);
        ui->currentTempoLabel->setText(QString::number(value) + " BPM (" + QString::number(newBASStempo) + "%)");
    } else {
        float basePercent = 100.0;                      // original detected percent
        float desiredPercent = (float)value;            // desired percent
        int newBASStempo = (int)(round(100.0*desiredPercent/basePercent));
        cBass.SetTempo(newBASStempo);
        ui->currentTempoLabel->setText(QString::number(value) + "%");
    }

    // update the hidden tempo column
    QItemSelectionModel* selectionModel = ui->songTable->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;
    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();
    } else {
        // FIX: more than 1 row or no rows at all selected (BAD)
        return;
    }

    if (tempoIsBPM) {
        ui->songTable->item(row, kTempoCol)->setText(QString::number(value));
    } else {
        ui->songTable->item(row, kTempoCol)->setText(QString::number(value) + "%");
    }

}

// ----------------------------------------------------------------------
void MainWindow::on_mixSlider_valueChanged(int value)
{
    int Rpercent = (int)(100.0*((float)value + 100.0)/200.0);
    int Lpercent = 100-Rpercent;
    QString s = QString::number(Lpercent) + "% L / " + QString::number(Rpercent) + "% R ";
    ui->currentMixLabel->setText(s);
    cBass.SetPan(value/100.0);
}

// ----------------------------------------------------------------------
QString MainWindow::position2String(int position, bool pad = false)
{
    int songMin = position/60;
    int songSec = position - 60*songMin;
    QString songSecString = QString("%1").arg(songSec, 2, 10, QChar('0')); // pad with zeros
    QString s(QString::number(songMin) + ":" + songSecString);

    // pad on the left with zeros, if needed to prevent numbers shifting left and right
    if (pad) {
        // NOTE: don't use more than 7 chars total, or Possum Sop (long) will cause weird
        //   shift left/right effects when the slider moves.
        switch (s.length()) {
        case 4: s = "   " + s; // 4 + 3 = 7 chars
            break;
        case 5: s = "  " + s;  // 5 + 2 = 7 chars
            break;
        default:
            break;
        }
    }

    return s;
}

void InitializeSeekBar(MySlider *seekBar)
{
    seekBar->setMinimum(0);
    seekBar->setMaximum((int)cBass.FileLength-1); // NOTE: tricky, counts on == below
    seekBar->setTickInterval(10);  // 10 seconds per tick
}
void SetSeekBarPosition(MySlider *seekBar, int currentPos_i)
{
    seekBar->blockSignals(true); // setValue should NOT initiate a valueChanged()
    seekBar->setValue(currentPos_i);
    seekBar->blockSignals(false);
}
void SetSeekBarNoSongLoaded(MySlider *seekBar)
{
    seekBar->setMinimum(0);
    seekBar->setValue(0);
}

// ----------------------------------------------------------------------
void MainWindow::Info_Seekbar(bool forceSlider)
{
    static bool in_Info_Seekbar = false;
    if (in_Info_Seekbar)
        return;
    RecursionGuard recursion_guard(in_Info_Seekbar);

    if (songLoaded) {  // FIX: this needs to pay attention to the bool
        // FIX: this code doesn't need to be executed so many times.
        InitializeSeekBar(ui->seekBar);
        InitializeSeekBar(ui->seekBarCuesheet);

        cBass.StreamGetPosition();  // update cBass.Current_Position

        int currentPos_i = (int)cBass.Current_Position;
        if (forceSlider) {
            SetSeekBarPosition(ui->seekBar, currentPos_i);
            SetSeekBarPosition(ui->seekBarCuesheet, currentPos_i);
        }
        int fileLen_i = (int)cBass.FileLength;

        if (currentPos_i == fileLen_i) {  // NOTE: tricky, counts on -1 above
            // avoids the problem of manual seek to max slider value causing auto-STOP
//            qDebug() << "Reached the end of playback!";
            if (!ui->actionContinuous_Play->isChecked()) {
                on_stopButton_clicked(); // pretend we pressed the STOP button when EOS is reached
            } else {
                // figure out which row is currently selected
                QItemSelectionModel* selectionModel = ui->songTable->selectionModel();
                QModelIndexList selected = selectionModel->selectedRows();
                int row = -1;
                if (selected.count() == 1) {
                    // exactly 1 row was selected (good)
                    QModelIndex index = selected.at(0);
                    row = index.row();
                } else {
                    // more than 1 row or no rows at all selected (BAD)
            //        qDebug() << "nothing selected.";
                    return;
                }

                int maxRow = ui->songTable->rowCount() - 1;

                if (row != maxRow) {
                    on_nextSongButton_clicked(); // pretend we pressed the NEXT SONG button when EOS is reached, then:
                    on_playButton_clicked();     // pretend we pressed the PLAY button
                } else {
                    on_stopButton_clicked();     // pretend we pressed the STOP button when End of Playlist is reached
                }
            }
            return;
        }

        ui->currentLocLabel->setText(position2String(currentPos_i, true));  // pad on the left
        ui->songLengthLabel->setText("/ " + position2String(fileLen_i));    // no padding
    }
    else {
        SetSeekBarNoSongLoaded(ui->seekBar);
        SetSeekBarNoSongLoaded(ui->seekBarCuesheet);
    }
}



// --------------------------------1--------------------------------------

void MainWindow::on_pushButtonSetIntroTime_clicked()
{
    int length = ui->seekBarCuesheet->maximum();
    int position = ui->seekBarCuesheet->value();
    
    ui->seekBarCuesheet->SetIntro((float)((float)position / (float)length));
    ui->seekBar->SetIntro((float)((float)position / (float)length));
}

// --------------------------------1--------------------------------------

void MainWindow::on_pushButtonSetOutroTime_clicked()
{
    int length = ui->seekBarCuesheet->maximum();
    int position = ui->seekBarCuesheet->value();
    
    ui->seekBarCuesheet->SetOutro((float)((float)position / (float)length));
    ui->seekBar->SetOutro((float)((float)position / (float)length));
}

// --------------------------------1--------------------------------------
void MainWindow::on_seekBarCuesheet_valueChanged(int value)
{
    on_seekBar_valueChanged(value);
}

// ----------------------------------------------------------------------
void MainWindow::on_seekBar_valueChanged(int value)
{
    // These must happen in this order.
    cBass.StreamSetPosition(value);
    Info_Seekbar(false);
}

// ----------------------------------------------------------------------
void MainWindow::on_clearSearchButton_clicked()
{
    // FIX: bug when clearSearch is pressed, the order in the songTable can change.

    // figure out which row is currently selected
    QItemSelectionModel* selectionModel = ui->songTable->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;
    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();
    } else {
        // more than 1 row or no rows at all selected (BAD)
//        qDebug() << "nothing selected.";
    }

    ui->labelSearch->setPlainText("");
    ui->typeSearch->setPlainText("");
    ui->titleSearch->setPlainText("");

    if (row != -1) {
        // if a row was selected, restore it after a clear search
        // FIX: this works much of the time, but it doesn't handle the case where search field is typed, then cleared.  In this case,
        //   the row isn't highlighted again.
        ui->songTable->selectRow(row);
    }
}

// ----------------------------------------------------------------------
void MainWindow::on_actionLoop_triggered()
{
    on_loopButton_toggled(ui->actionLoop->isChecked());
    ui->loopButton->setChecked(ui->actionLoop->isChecked());
}

// ----------------------------------------------------------------------
void MainWindow::on_UIUpdateTimerTick(void)
{
    Info_Seekbar(true);
}

// ----------------------------------------------------------------------
void MainWindow::on_vuMeterTimerTick(void)
{
//    qDebug() << "VU: " << cBass.StreamGetVuMeter();
    float currentVolumeSlider = ui->volumeSlider->value();
    int level = cBass.StreamGetVuMeter();
    float levelF = (currentVolumeSlider/100.0)*((float)level)/32768.0;
    // TODO: iff music is playing.
//    vuMeter->levelChanged(levelF/2.0,levelF,1*1024);  // 10X/sec, update the vuMeter
    vuMeter->levelChanged(levelF/2.0,levelF,256);  // 10X/sec, update the vuMeter
}

// --------------
void MainWindow::closeEvent(QCloseEvent *event)
{
    // Work around bug: https://codereview.qt-project.org/#/c/125589/
    if (closeEventHappened) {
        event->accept();
        return;
    }
    closeEventHappened = true;
    if (true) {
        on_actionAutostart_playback_triggered();  // write AUTOPLAY setting back
        event->accept();  // OK to close, if user said "OK" or "SAVE"
    }
    else {
        event->ignore();  // do not close, if used said "CANCEL"
        closeEventHappened = false;
    }
}

// ------------------------------------------------------------------------------------------
void MainWindow::aboutBox()
{
    QMessageBox msgBox;
    msgBox.setText(QString("<p><h2>SquareDesk Player, V0.5.1</h2>") +
                   QString("<p>See our website at <a href=\"http://squaredesk.net\">squaredesk.net</a></p>") +
                   QString("Uses: <a href=\"http://www.un4seen.com/bass.html\">libbass</a> and ") +
                   QString("<a href=\"http://www.jobnik.org/?mnu=bass_fx\">libbass_fx</a>") +
                   QString("<p>Thanks to: <a href=\"http://all8.com\">all8.com</a>"));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
}

// ------------------------------------------------------------------------
// http://www.codeprogress.com/cpp/libraries/qt/showQtExample.php?key=QApplicationInstallEventFilter&index=188
bool GlobalEventFilter::eventFilter(QObject *Object, QEvent *Event)
{
    if (Event->type() == QEvent::KeyPress) {
        QKeyEvent *KeyEvent = (QKeyEvent *)Event;

        if (!(ui->labelSearch->hasFocus() || // ui->labelNumberSearch->hasFocus() ||
                ui->typeSearch->hasFocus() || ui->titleSearch->hasFocus()
              || ui->lineEditCountDownTimer->hasFocus()
              || ui->songTable->isEditing())) {
            // call handleKeypress on the Applications's active window
            return ((MainWindow *)(((QApplication *)Object)->activeWindow()))->handleKeypress(KeyEvent->key());
        }

    }
    return QObject::eventFilter(Object,Event);
}

// ----------------------------------------------------------------------
//void MainWindow::keyPressEvent(QKeyEvent *event)
bool MainWindow::handleKeypress(int key)
{
//    qDebug() << "MainWindow::handleKeypress(), key =" << key << ", isPreferencesDialog =" << inPreferencesDialog;
    if (inPreferencesDialog || !trapKeypresses) {
        return false;
    }

    switch (key) {

        case Qt::Key_Escape:
        case Qt::Key_End:  // FIX: should END go to the end of the song? or stop playback?
        case Qt::Key_S:
            on_stopButton_clicked();
            break;

        case Qt::Key_P:
        case Qt::Key_Space:  // for SqView compatibility ("play/pause")
                             // if Stopped, PLAY;  if Playing, Pause.  If Paused, Resume.
            on_playButton_clicked();
            break;

        case Qt::Key_Home:
        case Qt::Key_Period:  // for SqView compatibility ("restart")
            on_stopButton_clicked();
            on_playButton_clicked();
            break;

        case Qt::Key_Right:
            on_actionSkip_Ahead_15_sec_triggered();
            break;
        case Qt::Key_Left:
            on_actionSkip_Back_15_sec_triggered();
            break;

        case Qt::Key_Backspace:  // either one will delete a row
        case Qt::Key_Delete:
            break;

        case Qt::Key_Down:
            ui->volumeSlider->setValue(ui->volumeSlider->value() - 5);
            break;
        case Qt::Key_Up:
            ui->volumeSlider->setValue(ui->volumeSlider->value() + 5);
            break;

        case Qt::Key_Plus:
        case Qt::Key_Equal:
            on_actionSpeed_Up_triggered();
            break;
        case Qt::Key_Minus:
            on_actionSlow_Down_triggered();
            break;

        case Qt::Key_K:
            on_actionNext_Playlist_Item_triggered();  // compatible with SqView!
            break;

        case Qt::Key_M:
            on_actionMute_triggered();
            break;

        case Qt::Key_U:
            on_actionPitch_Up_triggered();
            break;

        case Qt::Key_D:
            on_actionPitch_Down_triggered();
            break;

        default:
            break;
    }
    Info_Seekbar(true);
    return true;
}

// ------------------------------------------------------------------------
void MainWindow::on_actionSpeed_Up_triggered()
{
    ui->tempoSlider->setValue(ui->tempoSlider->value() + 1);
    on_tempoSlider_valueChanged(ui->tempoSlider->value());
}

void MainWindow::on_actionSlow_Down_triggered()
{
    ui->tempoSlider->setValue(ui->tempoSlider->value() - 1);
    on_tempoSlider_valueChanged(ui->tempoSlider->value());
}

// ------------------------------------------------------------------------
void MainWindow::on_actionSkip_Ahead_15_sec_triggered()
{
    cBass.StreamGetPosition();  // update the position
    // set the position to one second before the end, so that RIGHT ARROW works as expected
    cBass.StreamSetPosition((int)fmin(cBass.Current_Position + 15.0, cBass.FileLength-1.0));
    Info_Seekbar(true);
}

void MainWindow::on_actionSkip_Back_15_sec_triggered()
{
    Info_Seekbar(true);
    cBass.StreamGetPosition();  // update the position
    cBass.StreamSetPosition((int)fmax(cBass.Current_Position - 15.0, 0.0));
}

// ------------------------------------------------------------------------
void MainWindow::on_actionVolume_Up_triggered()
{
    ui->volumeSlider->setValue(ui->volumeSlider->value() + 5);
}

void MainWindow::on_actionVolume_Down_triggered()
{
    ui->volumeSlider->setValue(ui->volumeSlider->value() - 5);
}

// ------------------------------------------------------------------------
void MainWindow::on_actionPlay_triggered()
{
    on_playButton_clicked();
}

void MainWindow::on_actionStop_triggered()
{
    on_stopButton_clicked();
}

// ------------------------------------------------------------------------
void MainWindow::on_actionForce_Mono_Aahz_mode_triggered()
{
    on_monoButton_toggled(ui->actionForce_Mono_Aahz_mode->isChecked());
}

// ------------------------------------------------------------------------
void MainWindow::on_bassSlider_valueChanged(int value)
{
    cBass.SetEq(0, (float)value);
}

void MainWindow::on_midrangeSlider_valueChanged(int value)
{
    cBass.SetEq(1, (float)value);
}

void MainWindow::on_trebleSlider_valueChanged(int value)
{
    cBass.SetEq(2, (float)value);
}

void MainWindow::loadCuesheet(QString MP3FileName)
{
    int extensionPos = MP3FileName.lastIndexOf('.');
    QString cuesheetFilenameBase(MP3FileName);
    cuesheetFilenameBase.truncate(extensionPos + 1);
    const char *extensions[] = { "htm", "html" };

    for (size_t i = 0; i < sizeof(extensions) / sizeof(extensions[0]); ++i)
    {
        QString cuesheetFilename = cuesheetFilenameBase + extensions[i];
        if (QFile::exists(cuesheetFilename))
        {
            QUrl cuesheetUrl(QUrl::fromLocalFile(cuesheetFilename));
            ui->textBrowserCueSheet->setSource(cuesheetUrl);
            break;
        }
    }
        
}


void MainWindow::loadMP3File(QString MP3FileName, QString songTitle, QString songType) {
    loadCuesheet(MP3FileName);
    
    QStringList pieces = MP3FileName.split( "/" );
    QString filebase = pieces.value(pieces.length()-1);
    QStringList pieces2 = filebase.split(".");

    currentMP3filename = pieces2.value(pieces2.length()-2);
    
    
    if (songTitle != "") {
        ui->nowPlayingLabel->setText(songTitle);
    } else {
        ui->nowPlayingLabel->setText(currentMP3filename);  // FIX?  convert to short version?
    }

    QDir md(MP3FileName);
    QString canonicalFN = md.canonicalPath();

    cBass.StreamCreate(MP3FileName.toStdString().c_str());

    QStringList ss = MP3FileName.split('/');
    QString fn = ss.at(ss.size()-1);
    this->setWindowTitle(fn + QString(" - SquareDesk MP3 Player/Editor"));

    int length_sec = cBass.FileLength;
    int songBPM = round(cBass.Stream_BPM);

    // Intentionally compare against a narrower range here than BPM detection, because BPM detection
    //   returns a number at the limits, when it's actually out of range.
    // Also, turn off BPM for xtras (they are all over the place, including round dance cues, which have no BPM at all).
    //
    // TODO: make the types for turning off BPM detection a preference
    if ((songBPM>=125-10) && (songBPM<=125+10) && songType != "xtras") {
        tempoIsBPM = true;
        ui->currentTempoLabel->setText(QString::number(songBPM) + " BPM (100%)"); // initial load always at 100%

        ui->tempoSlider->setMinimum(songBPM-15);
        ui->tempoSlider->setMaximum(songBPM+15);
        ui->tempoSlider->setValue(songBPM);
        ui->tempoSlider->valueChanged(songBPM);  // fixes bug where second song with same BPM doesn't update songtable::tempo
        ui->tempoSlider->SetOrigin(songBPM);    // when double-clicked, goes here
        ui->tempoSlider->setEnabled(true);
        statusBar()->showMessage(QString("Song length: ") + position2String(length_sec) +
                                 ", base tempo: " + QString::number(songBPM) + " BPM");
    }
    else {
        tempoIsBPM = false;
        // if we can't figure out a BPM, then use percent as a fallback (centered: 100%, range: +/-20%)
        ui->currentTempoLabel->setText("100%");
        ui->tempoSlider->setMinimum(100-20);        // allow +/-20%
        ui->tempoSlider->setMaximum(100+20);
        ui->tempoSlider->setValue(100);
        ui->tempoSlider->valueChanged(100);  // fixes bug where second song with same 100% doesn't update songtable::tempo
        ui->tempoSlider->SetOrigin(100);  // when double-clicked, goes here
        ui->tempoSlider->setEnabled(true);
        statusBar()->showMessage(QString("Song length: ") + position2String(length_sec) +
                                 ", base tempo: 100%");
    }

    fileModified = false;

    ui->playButton->setEnabled(true);
    ui->stopButton->setEnabled(true);

    ui->actionPlay->setEnabled(true);
    ui->actionStop->setEnabled(true);
    ui->actionSkip_Ahead_15_sec->setEnabled(true);
    ui->actionSkip_Back_15_sec->setEnabled(true);

    ui->seekBar->setEnabled(true);
    ui->seekBarCuesheet->setEnabled(true);

    // when we add Pitch to the songTable as a hidden column, we do NOT need to force pitch anymore, because it
    //   will be set by the loader to the correct value (which is zero, if the MP3 file wasn't on the current playlist).
//    ui->pitchSlider->valueChanged(ui->pitchSlider->value()); // force pitch change, if pitch slider preset before load
    ui->volumeSlider->valueChanged(ui->volumeSlider->value()); // force vol change, if vol slider preset before load
    ui->mixSlider->valueChanged(ui->mixSlider->value()); // force mix change, if mix slider preset before load

    ui->actionMute->setEnabled(true);
    ui->actionLoop->setEnabled(true);

    ui->actionVolume_Down->setEnabled(true);
    ui->actionVolume_Up->setEnabled(true);
    ui->actionSpeed_Up->setEnabled(true);
    ui->actionSlow_Down->setEnabled(true);
    ui->actionForce_Mono_Aahz_mode->setEnabled(true);
    ui->actionPitch_Down->setEnabled(true);
    ui->actionPitch_Up->setEnabled(true);

    ui->bassSlider->valueChanged(ui->bassSlider->value()); // force bass change, if bass slider preset before load
    ui->midrangeSlider->valueChanged(ui->midrangeSlider->value()); // force midrange change, if midrange slider preset before load
    ui->trebleSlider->valueChanged(ui->trebleSlider->value()); // force treble change, if treble slider preset before load

    ui->loopButton->setEnabled(true);
    ui->monoButton->setEnabled(true);

    cBass.Stop();

    songLoaded = true;
    Info_Seekbar(true);

    if (songType == "patter") {
        ui->loopButton->setChecked(true);
        on_loopButton_toggled(true); // default is to loop, if type is patter
    } else {
        // not patter, so Loop mode defaults to OFF
        ui->loopButton->setChecked(false);
        on_loopButton_toggled(false); // default is to loop, if type is patter
    }

    ui->seekBar->SetSingingCall(songType == "singing"); // if singing call, color the seek bar
    ui->seekBarCuesheet->SetSingingCall(songType == "singing"); // if singing call, color the seek bar
}

void MainWindow::on_actionOpen_MP3_file_triggered()
{
    on_stopButton_clicked();  // if we're loading a new MP3 file, stop current playback

    // http://stackoverflow.com/questions/3597900/qsettings-file-chooser-should-remember-the-last-directory
    const QString DEFAULT_DIR_KEY("default_dir");
    QSettings MySettings; // Will be using application informations for correct location of your settings


    QString startingDirectory = MySettings.value(DEFAULT_DIR_KEY).toString();
    if (startingDirectory.isNull()) {
        // first time through, start at HOME
        startingDirectory = QDir::homePath();
    }

    QString MP3FileName =
        QFileDialog::getOpenFileName(this,
                                     tr("Import Audio File"),
                                     startingDirectory,
                                     tr("Audio Files (*.mp3 *.wav)"));
    if (MP3FileName.isNull()) {
        return;  // user cancelled...so don't do anything, just return
    }

    // not null, so save it in Settings (File Dialog will open in same dir next time)
    QDir CurrentDir;
    MySettings.setValue(DEFAULT_DIR_KEY, CurrentDir.absoluteFilePath(MP3FileName));

    ui->songTable->clearSelection();  // if loaded via menu, then clear previous selection (if present)
    ui->nextSongButton->setEnabled(false);  // and, next/previous song buttons are disabled
    ui->previousSongButton->setEnabled(false);

    // --------
//    qDebug() << "loading: " << MP3FileName;
    loadMP3File(MP3FileName, QString(""), QString(""));  // "" means use title from the filename
}


// this function stores the absolute paths of each file in a QVector
void findFilesRecursively(QDir rootDir, QList<QString> *pathStack) {
    QDirIterator it(rootDir, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
    while(it.hasNext()) {
        QString s1 = it.next();
        // If alias, follow it.
        QString resolvedFilePath = it.fileInfo().symLinkTarget(); // path with the symbolic links followed/removed
        if (resolvedFilePath == "") {
            // If NOT alias, then use the original fileName
            resolvedFilePath = s1;
        }

        QFileInfo fi(s1);
        QStringList section = fi.canonicalPath().split("/");
        QString type = section[section.length()-1];  // must be the last item in the path, of where the alias is, not where the file is

        pathStack->append(type + "#!#" + resolvedFilePath);
    }
}

void MainWindow::findMusic()
{
    // always gets rid of the old one...
    if (pathStack) {
        delete pathStack;
    }
    pathStack = new QList<QString>();

    QDir musicRootDir(musicRootPath);
    musicRootDir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDot | QDir::NoDotDot); // FIX: temporarily don't allow symlinks  | QDir::NoSymLinks

    QStringList qsl;
    qsl.append("*.mp3");                // I only want MP3 files
    qsl.append("*.wav");                //          or WAV files
    musicRootDir.setNameFilters(qsl);

    // --------
    findFilesRecursively(musicRootDir, pathStack);
}

void MainWindow::filterMusic() {
    ui->songTable->setSortingEnabled(false);

    // Need to remember the PL# mapping here, and reapply it after the filter
    // left = path, right = number string
    QMap<QString, QString> path2playlistNum;

    // Iterate over the songTable, saving the mapping in "path2playlistNum"
    // TODO: optimization: save this once, rather than recreating each time.
    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
        QString playlistIndex = theItem->text();  // this is the playlist #
        QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();  // this is the full pathname
        if (playlistIndex != " " && playlistIndex != "") {
            // item HAS an index (that is, it is on the list, and has a place in the ordering)
//            qDebug() << "remembering playlistIndex:" << playlistIndex << ", origPath:" << pathToMP3;
            // TODO: reconcile int here with float elsewhere on insertion
            path2playlistNum[pathToMP3] = playlistIndex;
        }
    }

    // clear out the table
    ui->songTable->setRowCount(0);

    QStringList m_TableHeader;
    m_TableHeader << "#" << "Type" << "Label" << "Title" << "Age";
    ui->songTable->setHorizontalHeaderLabels(m_TableHeader);
    ui->songTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    ui->songTable->horizontalHeader()->setVisible(true);

    QListIterator<QString> iter(*pathStack);

    while (iter.hasNext()) {
        QString s = iter.next();

        QStringList sl1 = s.split("#!#");
        QString type = sl1[0];  // the type (of original pathname, before following aliases)
        s = sl1[1];  // everything else
        QString origPath = s;  // for when we double click it later on...

        QFileInfo fi(s);

        if (fi.canonicalPath() == musicRootPath) {
            // e.g. "/Users/mpogue/__squareDanceMusic/C 117 - Bad Puppy (Patter).mp3" --> NO TYPE PRESENT
            type = "";
        }

        QStringList section = fi.canonicalPath().split("/");
        QString label = "";
        QString labelnum = "";
        QString title = "";

        s = fi.baseName(); // e.g. "/Users/mpogue/__squareDanceMusic/patter/RIV 307 - Going to Ceili (Patter).mp3" --> "RIV 307 - Going to Ceili (Patter)"
        QRegularExpression re_square("^(.+) - (.+)$");
        QRegularExpressionMatch match_square = re_square.match(s);
        if (match_square.hasMatch()) {
            label = match_square.captured(reverseLabelTitle ? 2 : 1);   // label == "RIV 307"
            title = match_square.captured(reverseLabelTitle ? 1 : 2);   // title == "Going to Ceili (Patter)"
        } else {
            // e.g. /Users/mpogue/__squareDanceMusic/xtras/Virginia Reel.mp3
            title = s;
        }

        ui->songTable->setRowCount(ui->songTable->rowCount()+1);  // make one more row for this line

        QColor textCol = QColor::fromRgbF(0.0/255.0, 0.0/255.0, 0.0/255.0);  // defaults to Black
        if (type == "xtras") {
            textCol = (QColor::fromRgbF(156.0/255.0, 31.0/255.0, 0.0/255.0)); // other: dark red
        } else if (type == "patter") {
            textCol = (QColor::fromRgbF(121.0/255.0, 99.0/255.0, 255.0/255.0)); // patter: Purple
        } else if (type == "singing") {
            textCol = (QColor::fromRgbF(0.0/255.0, 175.0/255.0, 92.0/255.0)); // singing: dark green
        } else if (type == "singing_called") {
            textCol = (QColor::fromRgbF(171.0/255.0, 105.0/255.0, 0.0/255.0)); // singing: dark green
        }

        // look up origPath in the path2playlistNum map, and reset the s2 text to the user's playlist # setting (if any)
        QString s2("");
        if (path2playlistNum.contains(origPath)) {
            s2 = path2playlistNum[origPath];
        }
//        qDebug() << "origPath:" << origPath << ", s2:" << s2;

        TableNumberItem *newTableItem4 = new TableNumberItem(s2);

        newTableItem4->setTextAlignment(Qt::AlignCenter);                           // editable by default
        newTableItem4->setTextColor(textCol);
        ui->songTable->setItem(ui->songTable->rowCount()-1, kNumberCol, newTableItem4);      // add it to column 0

        QTableWidgetItem *newTableItem2 = new QTableWidgetItem( type );
        newTableItem2->setFlags(newTableItem2->flags() & ~Qt::ItemIsEditable);      // not editable
        newTableItem2->setTextColor(textCol);
        ui->songTable->setItem(ui->songTable->rowCount()-1, kTypeCol, newTableItem2);      // add it to column 2

        QTableWidgetItem *newTableItem0 = new QTableWidgetItem( label + " " + labelnum );
        newTableItem0->setFlags(newTableItem0->flags() & ~Qt::ItemIsEditable);      // not editable
        newTableItem0->setTextColor(textCol);
        ui->songTable->setItem(ui->songTable->rowCount()-1, kLabelCol, newTableItem0);      // add it to column 0

        QTableWidgetItem *newTableItem3 = new QTableWidgetItem( title );
        newTableItem3->setFlags(newTableItem3->flags() & ~Qt::ItemIsEditable);      // not editable
        newTableItem3->setTextColor(textCol);
        ui->songTable->setItem(ui->songTable->rowCount()-1, kTitleCol, newTableItem3);      // add it to column 3

        // pitch column is hidden
        QTableWidgetItem *newTableItem5 = new QTableWidgetItem("0");
        newTableItem5->setFlags(newTableItem5->flags() & ~Qt::ItemIsEditable);      // not editable
        newTableItem5->setTextColor(textCol);
        ui->songTable->setItem(ui->songTable->rowCount()-1, kPitchCol, newTableItem5);      // add it to column 5 (pitch, hidden)

        // tempo column is hidden
        QTableWidgetItem *newTableItem6 = new QTableWidgetItem("0");  // 0 means "use base tempo"
        newTableItem6->setFlags(newTableItem6->flags() & ~Qt::ItemIsEditable);      // not editable
        newTableItem6->setTextColor(textCol);
        ui->songTable->setItem(ui->songTable->rowCount()-1, kTempoCol, newTableItem6);      // add it to column 4 (tempo, hidden)

        // keep the path around, for loading in when we double click on it
        ui->songTable->item(ui->songTable->rowCount()-1, kPathCol)->setData(Qt::UserRole, QVariant(origPath)); // path set on cell in col 0

        // Filter out (hide) rows that we're not interested in, based on the search fields...
        //   4 if statements is clearer than a gigantic single if....
        QString labelPlusNumber = label + " " + labelnum;
        if (ui->labelSearch->toPlainText() != "" && !labelPlusNumber.contains(QString(ui->labelSearch->toPlainText()),Qt::CaseInsensitive)) {
            ui->songTable->setRowHidden(ui->songTable->rowCount()-1,true);
        }

        if (ui->typeSearch->toPlainText() != "" && !type.contains(QString(ui->typeSearch->toPlainText()),Qt::CaseInsensitive)) {
            ui->songTable->setRowHidden(ui->songTable->rowCount()-1,true);
        }

        if (ui->titleSearch->toPlainText() != "" && !title.contains(QString(ui->titleSearch->toPlainText()),Qt::CaseInsensitive)) {
            ui->songTable->setRowHidden(ui->songTable->rowCount()-1,true);
        }

    }

    if (notSorted) {
//        qDebug() << "SORTING FOR THE FIRST TIME";
        ui->songTable->sortItems(kLabelCol);  // sort second by label/label #
        ui->songTable->sortItems(kTypeCol);  // sort first by type (singing vs patter)

        notSorted = false;
    }

    ui->songTable->setSortingEnabled(true);

    QString msg1 = QString::number(ui->songTable->rowCount()) + QString(" audio files found.");
    ui->statusBar->showMessage(msg1);
}

void MainWindow::on_labelSearch_textChanged()
{
    filterMusic();
}

void MainWindow::on_typeSearch_textChanged()
{
    filterMusic();
}

void MainWindow::on_titleSearch_textChanged()
{
    filterMusic();
}

void MainWindow::on_songTable_itemDoubleClicked(QTableWidgetItem *item)
{
    on_stopButton_clicked();  // if we're loading a new MP3 file, stop current playback

    int row = item->row();
    QString pathToMP3 = ui->songTable->item(row,kPathCol)->data(Qt::UserRole).toString();

    QString songTitle = ui->songTable->item(row,kTitleCol)->text();
    // FIX:  This should grab the title from the MP3 metadata in the file itself instead.

    QString songType = ui->songTable->item(row,kTypeCol)->text().toLower();
    

    // these must be up here to get the correct values...
    QString pitch = ui->songTable->item(row,kPitchCol)->text();
    QString tempo = ui->songTable->item(row,kTempoCol)->text();

    loadMP3File(pathToMP3, songTitle, songType);

    // these must be down here, to set the correct values...
    int pitchInt = pitch.toInt();
    ui->pitchSlider->setValue(pitchInt);
//    qDebug() << "itemDoubleClicked, setting pitch slider to:" << pitchInt;

    if (tempo != "0") {
        // iff tempo is known, then update the table
        QString tempo2 = tempo.replace("%",""); // if percentage (not BPM) just get rid of the "%" (setValue knows what to do)
        int tempoInt = tempo2.toInt();
        ui->tempoSlider->setValue(tempoInt);
//        qDebug() << "itemDoubleClicked, setting tempo slider to:" << tempoInt << ", tempo=" << tempo;
    }
    if (ui->actionAutostart_playback->isChecked()) {
        on_playButton_clicked();
    }
}

void MainWindow::on_actionClear_Search_triggered()
{
    on_clearSearchButton_clicked();
}

void MainWindow::on_actionPitch_Up_triggered()
{
    ui->pitchSlider->setValue(ui->pitchSlider->value() + 1);
}

void MainWindow::on_actionPitch_Down_triggered()
{
    ui->pitchSlider->setValue(ui->pitchSlider->value() - 1);
}

void MainWindow::on_actionAutostart_playback_triggered()
{
    // the Autostart on Playback mode setting is persistent across restarts of the application
    QSettings MySettings; // Will be using application information for correct location of your settings
    const QString AUTOSTART_KEY("autostartplayback");  // default is AUTOSTART ENABLED

    if (ui->actionAutostart_playback->isChecked()) {
        MySettings.setValue(AUTOSTART_KEY, "checked");
    } else {
        MySettings.setValue(AUTOSTART_KEY, "unchecked");
    }
}

// --------------------------------------------------------
void MainWindow::saveCheckBoxState(const char *key_string, QCheckBox *checkBox)
{
    const QString key(key_string);
    QSettings MySettings;
    MySettings.setValue(key, checkBox->isChecked() ? "checked" : "unchecked");
}

void MainWindow::restoreCheckBoxState(const char *key_string, QCheckBox *checkBox, bool checkedDefault)
{
    QSettings MySettings;
    const QString key(key_string);
    QString value = MySettings.value(key).toString();
    if (value.isNull())
    {
        value = checkedDefault ? "checked" : "unchecked";
        MySettings.setValue(key, value);
    }
    checkBox->setChecked(value == "checked");
}

void MainWindow::on_checkBoxPlayOnEnd_clicked()
{
    saveCheckBoxState("startplaybackoncountdowntimer", ui->checkBoxPlayOnEnd);
}

void MainWindow::on_checkBoxStartOnPlay_clicked()
{
    saveCheckBoxState("startcountuptimeronplay", ui->checkBoxStartOnPlay);
}

// --------------------------------------------------------
void MainWindow::on_actionPreferences_triggered()
{
    inPreferencesDialog = true;
    on_stopButton_clicked();  // stop music, if it was playing...

    PreferencesDialog *dialog = new PreferencesDialog;

    // modal dialog
    int dialogCode = dialog->exec();

    // act on dialog return code
    if(dialogCode == QDialog::Accepted)
    {
        // OK clicked
        QSettings MySettings;

        // Save the new value for musicPath --------
        MySettings.setValue("musicPath", dialog->musicPath); // fish out the new dir from the Preferences dialog, and save it

        if (dialog->musicPath != musicRootPath) { // path has changed!
            musicRootPath = dialog->musicPath;
            findMusic();
            filterMusic();
        }

        // Save the new value for experimentalTimersTabEnabled --------
        QString tabsetting;
        if (dialog->experimentalTimersTabEnabled == "true") {
            tabsetting = "true";
            if (!showTimersTab) {
                // iff the tab was NOT showing, make it show up now
                ui->tabWidget->insertTab(1, tabmap.value(1).first, tabmap.value(1).second);  // bring it back now!
            }
            showTimersTab = true;
        } else {
            tabsetting = "false";
            if (showTimersTab) {
                // iff timers tab was showing, remove it
                ui->tabWidget->removeTab(1);  // hidden, but we can bring it back later
            }
            showTimersTab = false;
        }
        MySettings.setValue("experimentalTimersTabEnabled", tabsetting); // save the new experimental tab setting

        // Save the new value for experimentalCuesheetTabEnabled --------
        int cuesheetTabNum = showTimersTab ? 2 : 1;
        if (dialog->experimentalCuesheetTabEnabled == "true") {
            tabsetting = "true";
            if (!showCuesheetTab) {
                // iff the tab was NOT showing, make it show up now
                ui->tabWidget->insertTab(cuesheetTabNum, tabmap.value(2).first, tabmap.value(2).second);  // bring it back now!
            }
            showCuesheetTab = true;
        } else {
            tabsetting = "false";
            if (showCuesheetTab) {
                // iff timers tab was showing, remove it
                ui->tabWidget->removeTab(cuesheetTabNum);  // hidden, but we can bring it back later
            }
            showCuesheetTab = false;
        }
        
        // Save the new value for experimentalCuesheetTabEnabled --------
        bool oldReverseLabelTitle = reverseLabelTitle;
        if (dialog->reverseLabelTitle == "true") {
            tabsetting = "true";
            reverseLabelTitle = true;
        } else {
            tabsetting = "false";
            reverseLabelTitle = false;
        }
        MySettings.setValue("experimentalCuesheetTabEnabled", tabsetting); // save the new experimental tab setting
        if (oldReverseLabelTitle != reverseLabelTitle)
        {
            filterMusic();
        }

        // Save the new value for experimentalPitchTempoViewEnabled --------
        QString viewsetting;
        if (dialog->experimentalPitchTempoViewEnabled == "true") {
            viewsetting = "true";
            pitchAndTempoHidden = false;
        } else {
            viewsetting = "false";
            pitchAndTempoHidden = true;
        }
        MySettings.setValue("experimentalPitchTempoViewEnabled", viewsetting); // save the new experimental tab setting
        updatePitchTempoView();  // update the columns in songTable, as per the user's NEW setting
    }

    inPreferencesDialog = false;
}

QString MainWindow::removePrefix(QString prefix, QString s) {
    QString s2 = s.remove( prefix );
//    qDebug() << "prefix:" << prefix << ", s:" << s << ", s2:" << s2;
    return s2;
}

// PLAYLIST MANAGEMENT ===============================================
void MainWindow::on_actionLoad_Playlist_triggered()
{
    on_stopButton_clicked();  // if we're loading a new PLAYLIST file, stop current playback

    // http://stackoverflow.com/questions/3597900/qsettings-file-chooser-should-remember-the-last-directory
    const QString DEFAULT_PLAYLIST_DIR_KEY("default_playlist_dir");
    QSettings MySettings; // Will be using application informations for correct location of your settings
    QString musicRootPath = MySettings.value("musicPath").toString();

    QString startingPlaylistDirectory = MySettings.value(DEFAULT_PLAYLIST_DIR_KEY).toString();
    if (startingPlaylistDirectory.isNull()) {
        // first time through, start at HOME
        startingPlaylistDirectory = QDir::homePath();
    }
    trapKeypresses = false;
    QString PlaylistFileName =
        QFileDialog::getOpenFileName(this,
                                     tr("Load Playlist"),
                                     startingPlaylistDirectory,
                                     tr("Playlist Files (*.m3u *.csv)"));
    trapKeypresses = true;
    if (PlaylistFileName.isNull()) {
        return;  // user cancelled...so don't do anything, just return
    }

    // not null, so save it in Settings (File Dialog will open in same dir next time)
    QDir CurrentDir;
    MySettings.setValue(DEFAULT_PLAYLIST_DIR_KEY, CurrentDir.absoluteFilePath(PlaylistFileName));

    // --------
    QString firstBadSongLine = "";
    int songCount = 0;
    QFile inputFile(PlaylistFileName);
    if (inputFile.open(QIODevice::ReadOnly))  // defaults to Text mode
    {
        ui->songTable->setSortingEnabled(false);  // sorting must be disabled to clear

        // first, clear all the playlist numbers that are there now.
        for (int i = 0; i < ui->songTable->rowCount(); i++) {
//            QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();
//            qDebug() << "clearing: " << pathToMP3;
            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
            theItem->setText("");

            QTableWidgetItem *theItem2 = ui->songTable->item(i,kPitchCol);  // clear out the hidden pitches, too
            theItem2->setText("0");

            QTableWidgetItem *theItem3 = ui->songTable->item(i,kTempoCol);  // clear out the hidden tempos, too
            theItem3->setText("0");
        }

        QTextStream in(&inputFile);

        if (PlaylistFileName.endsWith(".csv")) {
            // CSV FILE =================================
            int lineCount = 1;

            QString header = in.readLine();  // read header (and throw away for now), should be "abspath,pitch,tempo"

            while (!in.atEnd())
            {
                QString line = in.readLine();
//                qDebug() << "line:" << line;
                if (line == "abspath") {
                    // V1 of the CSV file format has exactly one field, an absolute pathname in quotes
                } else if (line == "") {
                    // ignore, it's a blank line
                } else {
                    songCount++;  // it's a real song path
                    QStringList list1 = line.split(",");

                    list1[0].replace("\"","");  // get rid of all double quotes in the abspath
                    list1[1].replace("\"","");  // get rid of all double quotes in the pitch (they should not be there at all, this is an INT)
//                    qDebug() << "SONG #" << songCount << "SONG PATH:" << list1[0] << "PITCH:" << list1[1];

                    bool match = false;
                    // exit the loop early, if we find a match
                    for (int i = 0; (i < ui->songTable->rowCount())&&(!match); i++) {
                        QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();
                        if (list1[0] == pathToMP3) { // FIX: this is fragile, if songs are moved around
                            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
                            theItem->setText(QString::number(songCount));

                            QTableWidgetItem *theItem2 = ui->songTable->item(i,kPitchCol);
                            theItem2->setText(list1[1]);

                            QTableWidgetItem *theItem3 = ui->songTable->item(i,kTempoCol);
                            theItem3->setText(list1[2]);

                            match = true;
                        }
                    }
                    // if we had no match, remember the first non-matching song path
                    if (!match && firstBadSongLine == "") {
                        firstBadSongLine = line;
                    }

                }

                lineCount++;
            } // while
        } else {
            // M3U FILE =================================
            int lineCount = 1;

            while (!in.atEnd())
            {
                QString line = in.readLine();
                //          qDebug() << "line:" << line;

                if (line == "#EXTM3U") {
                    // ignore, it's the first line of the M3U file
                } else if (line == "") {
                    // ignore, it's a blank line
                } else if (line.at( 0 ) == '#' ) {
                    // it's a comment line
                    if (line.mid(0,7) == "#EXTINF") {
                        // it's information about the next line, ignore for now.
                    }
                } else {
                    songCount++;  // it's a real song path
                    //              qDebug() << "SONG #" << songCount << "SONG PATH:" << line;

                    bool match = false;
                    // exit the loop early, if we find a match
                    for (int i = 0; (i < ui->songTable->rowCount())&&(!match); i++) {
                        QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();
                        if (line == pathToMP3) { // FIX: this is fragile, if songs are moved around
                            QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
                            theItem->setText(QString::number(songCount));

                            QTableWidgetItem *theItem2 = ui->songTable->item(i,kPitchCol);
                            theItem2->setText("0");  // M3U doesn't have pitch yet

                            QTableWidgetItem *theItem3 = ui->songTable->item(i,kTempoCol);
                            theItem3->setText("0");  // M3U doesn't have tempo yet

                            match = true;
                        }
                    }
                    // if we had no match, remember the first non-matching song path
                    if (!match && firstBadSongLine == "") {
                        firstBadSongLine = line;
                    }

                }

                lineCount++;
            }
        }

        inputFile.close();

    } else {
        // file didn't open...
        return;
    }

    ui->songTable->sortItems(kTitleCol);  // sort by title as last
    ui->songTable->sortItems(kLabelCol);  // sort by label/label# as secondary
    ui->songTable->sortItems(kNumberCol);  // sort by playlist # as primary
    notSorted = false;
    ui->songTable->setSortingEnabled(true);  // sorting must be disabled to clear

    // select the very first row, and trigger a GO TO PREVIOUS, which will load row 0 (and start it, if autoplay is ON).
    // only do this, if there were no errors in loading the playlist numbers.
    if (firstBadSongLine == "") {
        ui->songTable->selectRow(0); // select first row of newly loaded and sorted playlist!
        on_actionPrevious_Playlist_Item_triggered();
    }

    QString msg1 = QString("Loaded playlist with ") + QString::number(songCount) + QString(" items.");
    if (firstBadSongLine != "") {
        // if there was a non-matching path, tell the user what the first one of those was
        msg1 = QString("ERROR: could not find '") + firstBadSongLine + QString("'");
        ui->songTable->clearSelection(); // select nothing, if error
    }
    ui->statusBar->showMessage(msg1);

}

// TODO: strip off the root directory before saving...
void MainWindow::on_actionSave_Playlist_triggered()
{
    on_stopButton_clicked();  // if we're saving a new PLAYLIST file, stop current playback

    // http://stackoverflow.com/questions/3597900/qsettings-file-chooser-should-remember-the-last-directory
    const QString DEFAULT_PLAYLIST_DIR_KEY("default_playlist_dir");
    QSettings MySettings; // Will be using application informations for correct location of your settings

    QString startingPlaylistDirectory = MySettings.value(DEFAULT_PLAYLIST_DIR_KEY).toString();
    if (startingPlaylistDirectory.isNull()) {
        // first time through, start at HOME
        startingPlaylistDirectory = QDir::homePath();
    }

    QString preferred("CSV files (*.csv)");
    trapKeypresses = false;
    QString PlaylistFileName =
        QFileDialog::getSaveFileName(this,
                                     tr("Save Playlist"),
                                     startingPlaylistDirectory,
//                                     tr("Playlist Files (*.m3u *.csv)"),
                                     tr("M3U playlists (*.m3u);;CSV files (*.csv)"),
                                     &preferred);  // preferred is CSV
    trapKeypresses = true;
    if (PlaylistFileName.isNull()) {
        return;  // user cancelled...so don't do anything, just return
    }

    // not null, so save it in Settings (File Dialog will open in same dir next time)
    QDir CurrentDir;
    MySettings.setValue(DEFAULT_PLAYLIST_DIR_KEY, CurrentDir.absoluteFilePath(PlaylistFileName));

    // --------
//    qDebug() << "TODO: saving playlist: " << PlaylistFileName;

    QMap<int, QString> imports, importsPitch, importsTempo;

    // Iterate over the songTable
    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
        QString playlistIndex = theItem->text();
        QString pathToMP3 = ui->songTable->item(i,kPathCol)->data(Qt::UserRole).toString();
        QString songTitle = ui->songTable->item(i,kTitleCol)->text();
        QString pitch = ui->songTable->item(i,kPitchCol)->text();
        QString tempo = ui->songTable->item(i,kTempoCol)->text();

        if (playlistIndex != "") {
            // item HAS an index (that is, it is on the list, and has a place in the ordering)
//            qDebug() << "playlistIndex:" << playlistIndex << ", MP3:" << pathToMP3 << ", Title:" << songTitle;
            // TODO: reconcile int here with float elsewhere on insertion
            imports[playlistIndex.toInt()] = pathToMP3;
            importsPitch[playlistIndex.toInt()] = pitch;
            importsTempo[playlistIndex.toInt()] = tempo;
        }
    }

    // TODO: strip the initial part of the path off the Paths, e.g.
    //   /Users/mpogue/__squareDanceMusic/patter/C 117 - Restless Romp (Patter).mp3
    //   becomes
    //   patter/C 117 - Restless Romp (Patter).mp3
    //
    //   So, the remaining path is relative to the root music directory.
    //   When loading, first look at the patter and the rest
    //     if no match, try looking at the rest only
    //     if no match, then error (dialog?)
    //   Then on Save Playlist, write out the NEW patter and the rest

    // TODO: get rid of the single space, replace with nothing

    QFile file(PlaylistFileName);
    if (PlaylistFileName.endsWith(".m3u")) {
        if (file.open(QIODevice::ReadWrite)) {
            QTextStream stream(&file);
            stream << "#EXTM3U" << endl << endl;

            // list is auto-sorted here
            QMapIterator<int, QString> i(imports);
            while (i.hasNext()) {
                i.next();
                //            qDebug() << i.key() << ": " << i.value();
                stream << "#EXTINF:-1," << endl;  // nothing after the comma = no special name
                stream << i.value() << endl;
            }
            file.close();
        } else {
            ui->statusBar->showMessage(QString("ERROR: could not open M3U file."));
        }
    } else if (PlaylistFileName.endsWith(".csv")) {
        if (file.open(QIODevice::ReadWrite)) {
            QTextStream stream(&file);
            stream << "abspath,pitch,tempo" << endl;

            // list is auto-sorted here
            QMapIterator<int, QString> i(imports);
            while (i.hasNext()) {
                i.next();
                //            qDebug() << i.key() << ": " << i.value();
                stream << "\"" << i.value() << "\"," <<
                          importsPitch[i.key()] << "," <<
                          importsTempo[i.key()] << endl; // quoted absolute path, integer pitch (no quotes), integer tempo (opt % or 0)
            }
            file.close();
        } else {
            ui->statusBar->showMessage(QString("ERROR: could not open CSV file."));
        }
    }

    // TODO: if there are no songs specified in the playlist (yet, because not edited, or yet, because
    //   no playlist was loaded), Save Playlist... should be greyed out.

    if (PlaylistFileName.endsWith(".csv")) {
        ui->statusBar->showMessage(QString("Playlist items saved as CSV file."));
    } else if (PlaylistFileName.endsWith(".m3u")) {
        ui->statusBar->showMessage(QString("Playlist items saved as M3U file."));
    } else {
        ui->statusBar->showMessage(QString("ERROR: Can't save to that format."));
    }
}

void MainWindow::on_actionNext_Playlist_Item_triggered()
{
    // This code is similar to the row double clicked code...
    on_stopButton_clicked();  // if we're going to the next file in the playlist, stop current playback

    // figure out which row is currently selected
    QItemSelectionModel* selectionModel = ui->songTable->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;
    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();
    } else {
        // more than 1 row or no rows at all selected (BAD)
//        qDebug() << "nothing selected.";
        return;
    }

    int maxRow = ui->songTable->rowCount() - 1;
    row = (maxRow < row+1 ? maxRow : row+1); // bump up by 1
    ui->songTable->selectRow(row); // select new row!

    // load all the UI fields, as if we double-clicked on the new row
    QString pathToMP3 = ui->songTable->item(row,kPathCol)->data(Qt::UserRole).toString();
    QString songTitle = ui->songTable->item(row,kTitleCol)->text();
    // FIX:  This should grab the title from the MP3 metadata in the file itself instead.

    QString songType = ui->songTable->item(row,kTypeCol)->text();

    // must be up here...
    QString pitch = ui->songTable->item(row,kPitchCol)->text();
    QString tempo = ui->songTable->item(row,kTempoCol)->text();

    loadMP3File(pathToMP3, songTitle, songType);

    // must be down here...
    int pitchInt = pitch.toInt();
    ui->pitchSlider->setValue(pitchInt);

    if (tempo != "0") {
        QString tempo2 = tempo.replace("%",""); // get rid of optional "%", slider->setValue will do the right thing
        int tempoInt = tempo2.toInt();
        ui->tempoSlider->setValue(tempoInt);
    }

    if (ui->actionAutostart_playback->isChecked()) {
        on_playButton_clicked();
    }

    // TODO: Continuous Play mode to allow playing through an entire playlist without stopping (do not play blank playIndex files).
    //       Continuous Play should be disabled and OFF, unless a playlist is loaded.
}

void MainWindow::on_actionPrevious_Playlist_Item_triggered()
{
    // This code is similar to the row double clicked code...
    on_stopButton_clicked();  // if we're going to the next file in the playlist, stop current playback

    QItemSelectionModel* selectionModel = ui->songTable->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;
    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();
    } else {
        // more than 1 row or no rows at all selected (BAD)
        return;
    }

//    int rowCount = ui->songTable->rowCount();
    row = (row-1 < 0 ? 0 : row-1); // bump backwards by 1
    ui->songTable->selectRow(row); // select new row!

    // load all the UI fields, as if we double-clicked on the new row
    QString pathToMP3 = ui->songTable->item(row,kPathCol)->data(Qt::UserRole).toString();
    QString songTitle = ui->songTable->item(row,kTitleCol)->text();
    // FIX:  This should grab the title from the MP3 metadata in the file itself instead.

    QString songType = ui->songTable->item(row,kTypeCol)->text();

    // must be up here...
    QString pitch = ui->songTable->item(row,kPitchCol)->text();
    QString tempo = ui->songTable->item(row,kTempoCol)->text();

    loadMP3File(pathToMP3, songTitle, songType);

    // must be down here...
    int pitchInt = pitch.toInt();
    ui->pitchSlider->setValue(pitchInt);

    if (tempo != "0") {
        QString tempo2 = tempo.replace("%",""); // get rid of optional "%", setValue will take care of it
        int tempoInt = tempo.toInt();
        ui->tempoSlider->setValue(tempoInt);
    }

    if (ui->actionAutostart_playback->isChecked()) {
        on_playButton_clicked();
    }
}

void MainWindow::on_previousSongButton_clicked()
{
    on_actionPrevious_Playlist_Item_triggered();
}

void MainWindow::on_nextSongButton_clicked()
{
    on_actionNext_Playlist_Item_triggered();
}

void MainWindow::on_songTable_itemSelectionChanged()
{
    // When item selection is changed, enable Next/Previous song buttons,
    //   if at least one item in the table is selected.
    //
    // figure out which row is currently selected
    QItemSelectionModel* selectionModel = ui->songTable->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
//    qDebug() << "songTable:itemSelectionChanged(), selected.count(): " << selected.count();
    if (selected.count() == 1) {
        ui->nextSongButton->setEnabled(true);
        ui->previousSongButton->setEnabled(true);
    } else {
        ui->nextSongButton->setEnabled(false);
        ui->previousSongButton->setEnabled(false);
    }
}

void MainWindow::on_actionClear_Playlist_triggered()
{
    // Iterate over the songTable
    ui->songTable->setSortingEnabled(false);  // must turn sorting off, or else sorting on # will not clear all
    for (int i=0; i<ui->songTable->rowCount(); i++) {
        QTableWidgetItem *theItem = ui->songTable->item(i,kNumberCol);
        theItem->setText(""); // clear out the current list

        // let's intentionally NOT clear the pitches.  They are persistent within a session.
//        QTableWidgetItem *theItem2 = ui->songTable->item(i,kPitchCol);
//        theItem2->setText("0"); // clear out the current list

        // let's intentionally NOT clear the tempos.  They are persistent within a session.
//        QTableWidgetItem *theItem3 = ui->songTable->item(i,kTempoCol);
//        theItem2->setText("100%"); // clear out the current list
    }

    ui->songTable->sortItems(kLabelCol);  // sort second by label/label #
    ui->songTable->sortItems(kTypeCol);  // sort first by type (singing vs patter)

    notSorted = false;
    ui->songTable->setSortingEnabled(true);  // reenable sorting
}

// ---------------------------------------------------------
void MainWindow::showInFinderOrExplorer(QString filePath)
{
// From: http://lynxline.com/show-in-finder-show-in-explorer/
#if defined(Q_OS_MAC)
    QStringList args;
    args << "-e";
    args << "tell application \"Finder\"";
    args << "-e";
    args << "activate";
    args << "-e";
    args << "select POSIX file \""+filePath+"\"";
    args << "-e";
    args << "end tell";
    QProcess::startDetached("osascript", args);
#endif

#if defined(Q_OS_WIN)
    QStringList args;
    args << "/select," << QDir::toNativeSeparators(filePath);
    QProcess::startDetached("explorer", args);
#endif

#ifdef Q_OS_LINUX
    QStringList args;
    args << QDir::toNativeSeparators(filePath);
    QProcess::startDetached("xdg-open", args);
#endif // ifdef Q_OS_LINUX
}

void MainWindow::on_songTable_customContextMenuRequested(const QPoint &pos)
{
    Q_UNUSED(pos);

    // TODO: this function isn't called on Windows yet...

    if (ui->songTable->selectionModel()->hasSelection()) {
        QMenu menu(this);

#if defined(Q_OS_MAC)
        menu.addAction ( "Reveal in Finder" , this , SLOT (revealInFinder()) );
#endif

#if defined(Q_OS_WIN)
        menu.addAction ( "Show in Explorer" , this , SLOT (revealInFinder()) );
#endif

        // TODO: Linux equivalent?

        menu.popup(QCursor::pos());
        menu.exec();
    }
}

void MainWindow::revealInFinder() {
    QItemSelectionModel* selectionModel = ui->songTable->selectionModel();
    QModelIndexList selected = selectionModel->selectedRows();
    int row = -1;
    if (selected.count() == 1) {
        // exactly 1 row was selected (good)
        QModelIndex index = selected.at(0);
        row = index.row();

        QString pathToMP3 = ui->songTable->item(row,kPathCol)->data(Qt::UserRole).toString();
        showInFinderOrExplorer(pathToMP3);
    } else {
        // more than 1 row or no rows at all selected (BAD)
//        qDebug() << "nothing selected.";
    }
}

void MainWindow::columnHeaderResized(int logicalIndex, int /* oldSize */, int newSize) {
    int x1,y1,w1,h1;
    int x2,y2,w2,h2;
    int x3,y3,w3,h3;

    switch (logicalIndex) {
    case 0: // #
        // FIX: there's a bug here if the # column width is changed.  Qt doesn't seem to keep track of
        //  the correct size of the # column thereafter.  This is particularly visible on Win10, but it's
        //  also present on Mac OS X (Sierra).
        x1 = newSize + 14;
        y1 = ui->typeSearch->y();
        w1 = ui->songTable->columnWidth(1) - 5;
        h1 = ui->typeSearch->height();
        ui->typeSearch->setGeometry(x1,y1,w1,h1);

        x2 = x1 + w1 + 6 - 1;
        y2 = ui->labelSearch->y();
        w2 = ui->songTable->columnWidth(2) - 5;
        h2 = ui->labelSearch->height();
        ui->labelSearch->setGeometry(x2,y2,w2,h2);

        x3 = x2 + w2 + 6;
        y3 = ui->titleSearch->y();
        w3 = ui->songTable->width() - ui->clearSearchButton->width() - x3;
        h3 = ui->titleSearch->height();
        ui->titleSearch->setGeometry(x3,y3,w3,h3);

        break;

    case 1: // Type
        x1 = ui->songTable->columnWidth(0) + 35;
        y1 = ui->typeSearch->y();
        w1 = newSize - 4;
        h1 = ui->typeSearch->height();
        ui->typeSearch->setGeometry(x1,y1,w1,h1);
        ui->typeSearch->setFixedWidth(w1);

        x2 = x1 + w1 + 6;
        y2 = ui->labelSearch->y();
//        w2 = ui->labelSearch->width();
        w2 = ui->songTable->columnWidth(2) - 6;
        h2 = ui->labelSearch->height();
        ui->labelSearch->setGeometry(x2,y2,w2,h2);
        ui->labelSearch->setFixedWidth(w2);

        x3 = x2 + w2 + 6;
        y3 = ui->titleSearch->y();
        w3 = ui->songTable->width() - ui->clearSearchButton->width() - x3 + 17;
        h3 = ui->titleSearch->height();
        ui->titleSearch->setGeometry(x3,y3,w3,h3);
        break;

    case 2: // Label
        x1 = ui->typeSearch->x();
        y1 = ui->typeSearch->y();
        w1 = ui->typeSearch->width();
        h1 = ui->typeSearch->height();

        x2 = x1 + w1 + 6;
        y2 = ui->labelSearch->y();
        w2 = newSize - 6;
        h2 = ui->labelSearch->height();
        ui->labelSearch->setGeometry(x2,y2,w2,h2);
        ui->labelSearch->setFixedWidth(w2);

        x3 = x2 + w2 + 6;
        y3 = ui->titleSearch->y();
        w3 = ui->songTable->width() - ui->clearSearchButton->width() - x3 + 17;
        h3 = ui->titleSearch->height();
        ui->titleSearch->setGeometry(x3,y3,w3,h3);
        break;

    case 3: // Title
        break;

    default:
        break;
    }

}
