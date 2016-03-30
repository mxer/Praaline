/* InfoEditor.cpp
 *
 * Copyright (C) 2004-2011,2012,2013,2015 Paul Boersma
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

#include "InfoEditor.h"

Thing_implement (InfoEditor, TextEditor, 0);

static InfoEditor theInfoEditor;

void structInfoEditor :: v_destroy () {
	theInfoEditor = NULL;   // undangle
	InfoEditor_Parent :: v_destroy ();
}

void structInfoEditor :: v_clear () {
	Melder_clearInfo ();
}

void gui_information (const char32 *message);   // BUG
void gui_information (const char32 *message) {
	if (! theInfoEditor) {
		theInfoEditor = Thing_new (InfoEditor);
		TextEditor_init (theInfoEditor, U"");
		Thing_setName (theInfoEditor, U"Praat Info");
	}
	GuiText_setString (theInfoEditor -> textWidget, message);
	GuiThing_show (theInfoEditor -> d_windowForm);
	/*
	 * Try to make sure that the invalidated text widget and the elements of the fronted window are redrawn before the next event.
	 */
	#if cocoa
		//[theInfoEditor -> d_windowForm -> d_cocoaWindow   displayIfNeeded];   // apparently, this does not suffice
		[theInfoEditor -> d_windowForm -> d_cocoaWindow   display];   // this displays the menu as well as the text
		//[theInfoEditor -> textWidget -> d_cocoaTextView   displayIfNeeded];   // this displays only the text
		//[theInfoEditor -> textWidget -> d_cocoaTextView   display];
	#elif defined (macintosh)
		GuiShell_drain (theInfoEditor -> d_windowForm);
	#endif
}

/* End of file InfoEditor.cpp */
