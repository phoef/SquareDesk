#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>

#include <QDebug>
#include <QMainWindow>
#include <QTimer>
#include <QFileDialog>
#include <QDir>
#include <QDirIterator>
#include <QList>
#include <QListIterator>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QProxyStyle>
#include <QSettings>
#include <QSlider>
#include <QStack>
#include <QTableWidgetItem>
#include <QToolTip>
#include <QVariant>
#include <QWheelEvent>

#include <QDateTime>

#include "math.h"
#include "bass_audio.h"
#include "myslider.h"
#include "preferencesdialog.h"
#include "levelmeter.h"
#include "analogclock.h"

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    Ui::MainWindow *ui;
    void handleKeypress(int key);

protected:
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

private slots:
    void on_loopButton_toggled(bool checked);

    void on_monoButton_toggled(bool checked);

    void on_stopButton_clicked();

    void on_playButton_clicked();

    void on_pitchSlider_valueChanged(int value);

    void on_volumeSlider_valueChanged(int value);

    void on_actionMute_triggered();

    void on_tempoSlider_valueChanged(int value);

    void on_mixSlider_valueChanged(int value);

    void on_seekBar_valueChanged(int value);

    void on_clearSearchButton_clicked();

    void on_actionLoop_triggered();

    void on_UIUpdateTimerTick(void);
    void on_vuMeterTimerTick(void);

    void aboutBox();

    void on_actionSpeed_Up_triggered();

    void on_actionSlow_Down_triggered();

    void on_actionSkip_Ahead_15_sec_triggered();

    void on_actionSkip_Back_15_sec_triggered();

    void on_actionVolume_Up_triggered();

    void on_actionVolume_Down_triggered();

    void on_actionPlay_triggered();

    void on_actionStop_triggered();

    void on_actionForce_Mono_Aahz_mode_triggered();

    void on_bassSlider_valueChanged(int value);

    void on_midrangeSlider_valueChanged(int value);

    void on_trebleSlider_valueChanged(int value);

    void on_actionOpen_MP3_file_triggered();

    void on_songTable_itemDoubleClicked(QTableWidgetItem *item);

    void on_labelSearch_textChanged();

    void on_typeSearch_textChanged();

    void on_titleSearch_textChanged();

    void on_actionClear_Search_triggered();

    void on_actionPitch_Up_triggered();

    void on_actionPitch_Down_triggered();

    void on_actionAutostart_playback_triggered();

    void on_actionPreferences_triggered();

    void on_pushButtonCountDownTimerStartStop_clicked();
    void on_pushButtonCountDownTimerReset_clicked();
    void on_pushButtonCountUpTimerStartStop_clicked();
    void on_pushButtonCountUpTimerReset_clicked();

    // TODO: add these back in, when the widget/pushbutton is added...
//    void on_pushButtonSetIntroTime_clicked();

    // TODO: change to use the auto-wiring naming convention, when manual slot/signal wiring is removed...
    void timerCountUp_update();
    void timerCountDown_update();

    void on_actionLoad_Playlist_triggered();

    void on_actionSave_Playlist_triggered();

    void on_actionNext_Playlist_Item_triggered();

    void on_actionPrevious_Playlist_Item_triggered();

    void on_previousSongButton_clicked();

    void on_nextSongButton_clicked();

    void on_songTable_itemSelectionChanged();

    void on_actionClear_Playlist_triggered();

    void showInFinderOrExplorer(QString s);

    void on_songTable_customContextMenuRequested(const QPoint &pos);
    void revealInFinder();

    void columnHeaderResized(int logicalIndex, int oldSize, int newSize);

private:
    QAction *closeAct;  // WINDOWS only

    bool notSorted;

    int iFontsize;  // preferred font size (for eyeballs that can use some help)
    bool inPreferencesDialog;
    QString musicRootPath;

    bool showTimersTab;  // EXPERIMENTAL TIMERS STUFF
    bool pitchAndTempoHidden;  // EXPERIMENTAL PITCH/TEMPO VIEW STUFF

    QMap<int,QPair<QWidget*,QString> > tabmap; // keep track of experimental tabs

    unsigned char currentState;
    short int currentPitch;
    unsigned short currentVolume;
    int previousVolume;

    bool tempoIsBPM;

    void Info_Volume(void);
    void Info_Seekbar(bool forceSlider);
    QString position2String(int position, bool pad);

    bool closeEventHappened;

    QString currentMP3filename;
    bool songLoaded;
    bool fileModified;

    void loadMP3File(QString filepath, QString songTitle, QString songType);
    void findMusic();    // get the filenames into pathStack
    void filterMusic();  // filter them into the songTable

    QList<QString> *pathStack;

    // Experimental Timer stuff
    QTimer *timerCountUp;
    qint64 timeCountUpZeroMs;
    QTimer *timerCountDown;
    qint64 timeCountDownZeroMs;

    bool timerStopStartClick(QTimer *&timer, QPushButton *button);
    void updateTimer(qint64 timeZero, QLabel *label);

    QString removePrefix(QString prefix, QString s);

    void updatePitchTempoView();
    void setFontSizes();

    // VU Meter support
    QTimer *UIUpdateTimer;
    QTimer *vuMeterTimer;

    LevelMeter *vuMeter;

    AnalogClock *analogClock;
};

// currentState:
#define kStopped 0
#define kPlaying 1
#define kPaused  2

// columns in songTable
#define kNumberCol 0
#define kTypeCol 1
#define kPathCol 1
// path is stored in the userData portion of the Type column...
#define kLabelCol 2
#define kTitleCol 3

// hidden columns:
#define kPitchCol 4
#define kTempoCol 5

// ---------------------------------------------
// http://stackoverflow.com/questions/24719739/how-to-use-qstylesh-tooltip-wakeupdelay-to-set-tooltip-wake-up-time
class MyProxyStyle : public QProxyStyle
{
    Q_OBJECT
public:
    int styleHint(StyleHint hint,
                  const QStyleOption *option,
                  const QWidget *widget,
                  QStyleHintReturn *returnData) const Q_DECL_OVERRIDE
    {
        if (hint == QStyle::SH_ToolTip_WakeUpDelay) {
            return 2000;    // 2 seconds for tooltips
        }

        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

// ---------------------
class GlobalEventFilter: public QObject
{

public:
    GlobalEventFilter(Ui::MainWindow *ui1)
    {
        ui = ui1;
    }
    Ui::MainWindow *ui;
    bool eventFilter(QObject *Object, QEvent *Event);
};


#endif // MAINWINDOW_H
