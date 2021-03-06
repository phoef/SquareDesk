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

/*  This is where preferences options get set. This file gets run
 *  through the preprocessor in a couple of different places, and
 *  different scenarios, so it's important that:
 *
 *  1. This file has no declarations that can't be processed twice.
 *  2. This file doesn't have "include" guards.
 *
 * When adding a combo box here, make sure that you add the associated
 * "SetPulldownValuesToItemNumberPlusOne" in preferencesdialog.cpp
 *
 * The four places this file gets processed are prefsdialog.{cpp,h},
 * and prefsmanager.{cpp,h}.
 *
 * CONFIG_ATTRIBUTE_*_NO_PREFS(preference_name, default_value) - Sets
 * up a preference of the appropriate type in the PreferencesManager
 * (prefsmanager.{cpp,h}), with accessors of the form
 * Get##preference_name and Set##preference_name.
 *
 * CONFIG_ATTRIBUTE_*(control_name, preference_name, default_value) -
 * Also sets up accessors in the PreferencesDialog structure for the
 * names in the preferences.ui file, and populates the dialog values
 * from the settings by way of populatePreferencesDialog(...), and the
 * settings from the dialog values by way of
 * extractValuesFromPreferencesDialog(...), both members in the
 * PreferencesManager structure in prefsmanager.{cpp,h}.
 *
 */

CONFIG_ATTRIBUTE_STRING(musicPath, musicPath, QDir::homePath() + "/music")

CONFIG_ATTRIBUTE_COLOR(patterColorButton,  patterColorString,  DEFAULTPATTERCOLOR)
CONFIG_ATTRIBUTE_COLOR(singingColorButton, singingColorString, DEFAULTSINGINGCOLOR)
CONFIG_ATTRIBUTE_COLOR(calledColorButton,  calledColorString,  DEFAULTCALLEDCOLOR)
CONFIG_ATTRIBUTE_COLOR(extrasColorButton,  extrasColorString,  DEFAULTEXTRASCOLOR)

CONFIG_ATTRIBUTE_BOOLEAN(longTipCheckbox,tipLengthTimerEnabled, false)
CONFIG_ATTRIBUTE_COMBO(longTipLength,tipLengthTimerLength, 7)
CONFIG_ATTRIBUTE_COMBO(afterLongTipAction,tipLengthAlarmAction, 0)
CONFIG_ATTRIBUTE_BOOLEAN(breakTimerCheckbox, breakLengthTimerEnabled, false)
CONFIG_ATTRIBUTE_COMBO(breakLength,breakLengthTimerLength, 10)
CONFIG_ATTRIBUTE_COMBO(afterBreakAction,breakLengthAlarmAction, 10)
CONFIG_ATTRIBUTE_COMBO(comboBoxMusicFormat, SongFilenameFormat, SongFilenameLabelDashName)
CONFIG_ATTRIBUTE_COMBO(comboBoxSessionDefault, SessionDefault, SessionDefaultPractice)
CONFIG_ATTRIBUTE_STRING(lineEditMusicTypeSinging, MusicTypeSinging, "singing;singers")
CONFIG_ATTRIBUTE_STRING(lineEditMusicTypePatter, MusicTypePatter, "patter;hoedown")

CONFIG_ATTRIBUTE_STRING(lineEditMusicTypeExtras, MusicTypeExtras, "extras;xtras")
CONFIG_ATTRIBUTE_STRING(lineEditMusicTypeCalled, MusicTypeCalled, "vocal;vocals;called")

CONFIG_ATTRIBUTE_BOOLEAN(EnableTimersTabCheckbox, experimentalTimersEnabled, false)
CONFIG_ATTRIBUTE_BOOLEAN(EnablePitchTempoViewCheckbox, experimentalPitchTempoViewEnabled, false)
CONFIG_ATTRIBUTE_BOOLEAN(EnableClockColoring,experimentalClockColoringEnabled, false)

CONFIG_ATTRIBUTE_BOOLEAN(checkBoxSaveSongPreferencesInConfig,SongPreferencesInConfig, false)

CONFIG_ATTRIBUTE_BOOLEAN(initialBPMcheckbox,tryToSetInitialBPM, false)
CONFIG_ATTRIBUTE_BOOLEAN(checkBoxSwitchToLyricsOnPlay,switchToLyricsOnPlay, false)
CONFIG_ATTRIBUTE_INT(initialBPMLineEdit,initialBPM, 125)

CONFIG_ATTRIBUTE_BOOLEAN(useTimeRemainingCheckbox,useTimeRemaining, false)

CONFIG_ATTRIBUTE_BOOLEAN(enableFlashCallsCheckbox,enableFlashCalls, false)

CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(autostartplayback, false);
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(forcemono, false);
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(startplaybackoncountdowntimer, false)
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(startcountuptimeronplay, false)
CONFIG_ATTRIBUTE_STRING_NO_PREFS(default_dir, QDir::homePath())
CONFIG_ATTRIBUTE_STRING_NO_PREFS(default_playlist_dir, QDir::homePath())

CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(enablevoiceinput, false);
CONFIG_ATTRIBUTE_BOOLEAN_NO_PREFS(enableautoscrolllyrics, false);

CONFIG_ATTRIBUTE_BOOLEAN(enableAutoAirplaneModeCheckbox, enableAutoAirplaneMode, false)
