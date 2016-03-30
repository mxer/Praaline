/* EEGWindow.cpp
 *
 * Copyright (C) 2011-2012,2013,2014,2015 Paul Boersma
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "EEGWindow.h"
#include "EditorM.h"

Thing_implement (EEGWindow, TextGridEditor, 0);

#include "prefs_define.h"
#include "EEGWindow_prefs.h"
#include "prefs_install.h"
#include "EEGWindow_prefs.h"
#include "prefs_copyToInstance.h"
#include "EEGWindow_prefs.h"

static void menu_cb_EEGWindowHelp (EDITOR_ARGS) { EDITOR_IAM (EEGWindow); Melder_help (U"EEG window"); }

void structEEGWindow :: v_createMenus () {
	EEGWindow_Parent :: v_createMenus ();
}

void structEEGWindow :: v_createHelpMenuItems (EditorMenu menu) {
	TextGridEditor_Parent :: v_createHelpMenuItems (menu);
	EditorMenu_addCommand (menu, U"EEGWindow help", '?', menu_cb_EEGWindowHelp);
}

const char32 * structEEGWindow :: v_getChannelName (long channelNumber) {
	Melder_assert (our eeg != NULL);
	return our eeg -> channelNames [channelNumber];
}

static void menu_cb_ExtractSelectedEEG_preserveTimes (EDITOR_ARGS) {
	EDITOR_IAM (EEGWindow);
	if (my d_endSelection <= my d_startSelection) Melder_throw (U"No selection.");
	autoEEG extract = EEG_extractPart (my eeg, my d_startSelection, my d_endSelection, true);
	Editor_broadcastPublication (me, extract.transfer());
}

static void menu_cb_ExtractSelectedEEG_timeFromZero (EDITOR_ARGS) {
	EDITOR_IAM (EEGWindow);
	if (my d_endSelection <= my d_startSelection) Melder_throw (U"No selection.");
	autoEEG extract = EEG_extractPart (my eeg, my d_startSelection, my d_endSelection, false);
	Editor_broadcastPublication (me, extract.transfer());
}

void structEEGWindow :: v_createMenuItems_file_extract (EditorMenu menu) {
	EEGWindow_Parent :: v_createMenuItems_file_extract (menu);
	our extractSelectedEEGPreserveTimesButton =
		EditorMenu_addCommand (menu, U"Extract selected EEG (preserve times)", 0, menu_cb_ExtractSelectedEEG_preserveTimes);
	our extractSelectedEEGTimeFromZeroButton =
		EditorMenu_addCommand (menu, U"Extract selected EEG (time from zero)", 0, menu_cb_ExtractSelectedEEG_timeFromZero);
}

void structEEGWindow :: v_updateMenuItems_file () {
	EEGWindow_Parent :: v_updateMenuItems_file ();
	GuiThing_setSensitive (our extractSelectedEEGPreserveTimesButton, d_endSelection > d_startSelection);
	GuiThing_setSensitive (our extractSelectedEEGTimeFromZeroButton,  d_endSelection > d_startSelection);
}

void EEGWindow_init (EEGWindow me, const char32 *title, EEG eeg) {
	my eeg = eeg;   // before initing, because initing will already draw!
	TextGridEditor_init (me, title, eeg -> textgrid, eeg -> sound, false, NULL, NULL);
}

EEGWindow EEGWindow_create (const char32 *title, EEG eeg) {
	try {
		autoEEGWindow me = Thing_new (EEGWindow);
		EEGWindow_init (me.peek(), title, eeg);
		return me.transfer();
	} catch (MelderError) {
		Melder_throw (U"EEG window not created.");
	}
}

/* End of file EEGWindow.cpp */
