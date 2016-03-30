/* ManipulationEditor.cpp
 *
 * Copyright (C) 1992-2011,2012,2013,2014,2015 Paul Boersma
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

#include "ManipulationEditor.h"
#include "PitchTier_to_PointProcess.h"
#include "Sound_to_PointProcess.h"
#include "Sound_to_Pitch.h"
#include "Pitch_to_PitchTier.h"
#include "Pitch_to_PointProcess.h"
#include "EditorM.h"

#include "enums_getText.h"
#include "ManipulationEditor_enums.h"
#include "enums_getValue.h"
#include "ManipulationEditor_enums.h"

Thing_implement (ManipulationEditor, FunctionEditor, 0);

#include "prefs_define.h"
#include "ManipulationEditor_prefs.h"
#include "prefs_install.h"
#include "ManipulationEditor_prefs.h"
#include "prefs_copyToInstance.h"
#include "ManipulationEditor_prefs.h"

/*
 * How to add a synthesis method (in an interruptable order):
 * 1. add an Manipulation_ #define in Manipulation.h;
 * 2. add a synthesize_ routine in Manipulation.cpp, and a reference to it in Manipulation_to_Sound;
 * 3. add a button in ManipulationEditor.h;
 * 4. add a cb_Synth_ callback.
 * 5. create the button in createMenus and update updateMenus;
 */

static const char32 *units_strings [] = { 0, U"Hz", U"st" };

static int prefs_synthesisMethod = Manipulation_OVERLAPADD;   /* Remembered across editor creations, not across Praat sessions. */

/* BUG: 25 should be fmin */
#define YLIN(freq)  (my p_pitch_units == kManipulationEditor_pitchUnits_HERTZ ? ((freq) < 25 ? 25 : (freq)) : NUMhertzToSemitones ((freq) < 25 ? 25 : (freq)))
#define YLININV(freq)  (my p_pitch_units == kManipulationEditor_pitchUnits_HERTZ ? (freq) : NUMsemitonesToHertz (freq))

static void updateMenus (ManipulationEditor me) {
	Melder_assert (my synthPulsesButton != NULL);
	GuiMenuItem_check (my synthPulsesButton, my synthesisMethod == Manipulation_PULSES);
	Melder_assert (my synthPulsesHumButton != NULL);
	GuiMenuItem_check (my synthPulsesHumButton, my synthesisMethod == Manipulation_PULSES_HUM);
	Melder_assert (my synthPulsesLpcButton != NULL);
	GuiMenuItem_check (my synthPulsesLpcButton, my synthesisMethod == Manipulation_PULSES_LPC);
	Melder_assert (my synthPitchButton != NULL);
	GuiMenuItem_check (my synthPitchButton, my synthesisMethod == Manipulation_PITCH);
	Melder_assert (my synthPitchHumButton != NULL);
	GuiMenuItem_check (my synthPitchHumButton, my synthesisMethod == Manipulation_PITCH_HUM);
	Melder_assert (my synthPulsesPitchButton != NULL);
	GuiMenuItem_check (my synthPulsesPitchButton, my synthesisMethod == Manipulation_PULSES_PITCH);
	Melder_assert (my synthPulsesPitchHumButton != NULL);
	GuiMenuItem_check (my synthPulsesPitchHumButton, my synthesisMethod == Manipulation_PULSES_PITCH_HUM);
	Melder_assert (my synthOverlapAddButton != NULL);
	GuiMenuItem_check (my synthOverlapAddButton, my synthesisMethod == Manipulation_OVERLAPADD);
	Melder_assert (my synthPitchLpcButton != NULL);
	GuiMenuItem_check (my synthPitchLpcButton, my synthesisMethod == Manipulation_PITCH_LPC);
}

/*
 * The "sound area" contains the original sound and the pulses.
 */
static int getSoundArea (ManipulationEditor me, double *ymin, double *ymax) {
	Manipulation ana = (Manipulation) my data;
	*ymin = 0.66;
	*ymax = 1.00;
	return ana -> sound != NULL || ana -> pulses != NULL;
}
/*
 * The "pitch area" contains the grey pitch analysis based on the pulses, and the blue pitch tier.
 */
static int getPitchArea (ManipulationEditor me, double *ymin, double *ymax) {
	Manipulation ana = (Manipulation) my data;
	*ymin = ana -> duration ? 0.16 : 0.00;
	*ymax = 0.65;
	return ana -> pulses != NULL || ana -> pitch != NULL;
}
static int getDurationArea (ManipulationEditor me, double *ymin, double *ymax) {
	Manipulation ana = (Manipulation) my data;
	if (! ana -> duration) return FALSE;
	*ymin = 0.00;
	*ymax = 0.15;
	return TRUE;
}

/********** DESTRUCTION **********/

void structManipulationEditor :: v_destroy () {
	forget (our previousPulses);
	forget (our previousPitch);
	forget (our previousDuration);
	ManipulationEditor_Parent :: v_destroy ();
}

/********** MENU COMMANDS **********/

/***** FILE MENU *****/

static void menu_cb_extractOriginalSound (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	Manipulation ana = (Manipulation) my data;
	if (! ana -> sound) return;
	autoSound publish = Data_copy (ana -> sound);
	Editor_broadcastPublication (me, publish.transfer());
}

static void menu_cb_extractPulses (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	Manipulation ana = (Manipulation) my data;
	if (! ana -> pulses) return;
	autoPointProcess publish = Data_copy (ana -> pulses);
	Editor_broadcastPublication (me, publish.transfer());
}

static void menu_cb_extractPitchTier (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	Manipulation ana = (Manipulation) my data;
	if (! ana -> pitch) return;
	autoPitchTier publish = Data_copy (ana -> pitch);
	Editor_broadcastPublication (me, publish.transfer());
}

static void menu_cb_extractDurationTier (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	Manipulation ana = (Manipulation) my data;
	if (! ana -> duration) return;
	autoDurationTier publish = Data_copy (ana -> duration);
	Editor_broadcastPublication (me, publish.transfer());
}

static void menu_cb_extractManipulatedSound (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	Manipulation ana = (Manipulation) my data;
	autoSound publish = Manipulation_to_Sound (ana, my synthesisMethod);
	Editor_broadcastPublication (me, publish.transfer());
}

/***** EDIT MENU *****/

void structManipulationEditor :: v_saveData () {
	Manipulation ana = (Manipulation) our data;
	forget (our previousPulses);
	forget (our previousPitch);
	forget (our previousDuration);
	if (ana -> pulses)   our previousPulses   = Data_copy (ana -> pulses);
	if (ana -> pitch)    our previousPitch    = Data_copy (ana -> pitch);
	if (ana -> duration) our previousDuration = Data_copy (ana -> duration);
}

void structManipulationEditor :: v_restoreData () {
	Manipulation ana = (Manipulation) our data;
	Any dummy;
	dummy = ana -> pulses;   ana -> pulses   = our previousPulses;   our previousPulses   = (PointProcess) dummy;
	dummy = ana -> pitch;    ana -> pitch    = our previousPitch;    our previousPitch    = (PitchTier)    dummy;
	dummy = ana -> duration; ana -> duration = our previousDuration; our previousDuration = (DurationTier) dummy;
}

/***** PULSES MENU *****/

static void menu_cb_removePulses (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);	
	Manipulation ana = (Manipulation) my data;
	if (! ana -> pulses) return;
	Editor_save (me, U"Remove pulse(s)");
	if (my d_startSelection == my d_endSelection)
		PointProcess_removePointNear (ana -> pulses, my d_startSelection);
	else
		PointProcess_removePointsBetween (ana -> pulses, my d_startSelection, my d_endSelection);
	FunctionEditor_redraw (me);
	Editor_broadcastDataChanged (me);
}

static void menu_cb_addPulseAtCursor (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	Manipulation ana = (Manipulation) my data;
	if (! ana -> pulses) return;
	Editor_save (me, U"Add pulse");
	PointProcess_addPoint (ana -> pulses, 0.5 * (my d_startSelection + my d_endSelection));
	FunctionEditor_redraw (me);
	Editor_broadcastDataChanged (me);
}

static void menu_cb_addPulseAt (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	EDITOR_FORM (U"Add pulse", 0)
		REAL (U"Position (s)", U"0.0")
	EDITOR_OK
		SET_REAL (U"Position", 0.5 * (my d_startSelection + my d_endSelection))
	EDITOR_DO
		Manipulation ana = (Manipulation) my data;
		if (! ana -> pulses) return;
		Editor_save (me, U"Add pulse");
		PointProcess_addPoint (ana -> pulses, GET_REAL (U"Position"));
		FunctionEditor_redraw (me);
		Editor_broadcastDataChanged (me);
	EDITOR_END
}

/***** PITCH MENU *****/

static void menu_cb_removePitchPoints (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	Manipulation ana = (Manipulation) my data;
	if (! ana -> pitch) return;
	Editor_save (me, U"Remove pitch point(s)");
	if (my d_startSelection == my d_endSelection)
		AnyTier_removePointNear (ana -> pitch, my d_startSelection);
	else
		AnyTier_removePointsBetween (ana -> pitch, my d_startSelection, my d_endSelection);
	FunctionEditor_redraw (me);
	Editor_broadcastDataChanged (me);
}

static void menu_cb_addPitchPointAtCursor (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	Manipulation ana = (Manipulation) my data;
	if (! ana -> pitch) return;
	Editor_save (me, U"Add pitch point");
	RealTier_addPoint (ana -> pitch, 0.5 * (my d_startSelection + my d_endSelection), YLININV (my pitchTier.cursor));
	FunctionEditor_redraw (me);
	Editor_broadcastDataChanged (me);
}

static void menu_cb_addPitchPointAtSlice (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	Manipulation ana = (Manipulation) my data;
	PointProcess pulses = ana -> pulses;
	if (! pulses) Melder_throw (U"There are no pulses.");
	if (! ana -> pitch) return;
	long ileft = PointProcess_getLowIndex (pulses, 0.5 * (my d_startSelection + my d_endSelection)), iright = ileft + 1, nt = pulses -> nt;
	double *t = pulses -> t;
	double f = my pitchTier.cursor;   // default
	Editor_save (me, U"Add pitch point");
	if (nt <= 1) {
		/* Ignore. */
	} else if (ileft <= 0) {
		double tright = t [2] - t [1];
		if (tright > 0.0 && tright <= 0.02) f = YLIN (1.0 / tright);
	} else if (iright > nt) {
		double tleft = t [nt] - t [nt - 1];
		if (tleft > 0.0 && tleft <= 0.02) f = YLIN (1.0 / tleft);
	} else {   /* Three-period median. */
		double tmid = t [iright] - t [ileft], tleft = 0.0, tright = 0.0;
		if (ileft > 1) tleft = t [ileft] - t [ileft - 1];
		if (iright < nt) tright = t [iright + 1] - t [iright];
		if (tleft > 0.02) tleft = 0;
		if (tmid > 0.02) tmid = 0;
		if (tright > 0.02) tright = 0;
		/* Bubble-sort. */
		if (tmid < tleft) { double dum = tmid; tmid = tleft; tleft = dum; }
		if (tright < tleft)  { double dum = tright; tright = tleft; tleft = dum; }
		if (tright < tmid)  { double dum = tright; tright = tmid; tmid = dum; }
		if (tleft != 0.0) f = YLIN (1 / tmid);   // median of 3
		else if (tmid != 0.0) f = YLIN (2 / (tmid + tright));   // median of 2
		else if (tright != 0.0) f = YLIN (1 / tright);   // median of 1
	}
	RealTier_addPoint (ana -> pitch, 0.5 * (my d_startSelection + my d_endSelection), YLININV (f));
	FunctionEditor_redraw (me);
	Editor_broadcastDataChanged (me);
}	

static void menu_cb_addPitchPointAt (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	EDITOR_FORM (U"Add pitch point", 0)
		REAL (U"Time (s)", U"0.0")
		REAL (U"Frequency (Hz or st)", U"100.0")
	EDITOR_OK
		SET_REAL (U"Time", 0.5 * (my d_startSelection + my d_endSelection))
		SET_REAL (U"Frequency", my pitchTier.cursor)
	EDITOR_DO
		Manipulation ana = (Manipulation) my data;
		if (! ana -> pitch) return;
		Editor_save (me, U"Add pitch point");
		RealTier_addPoint (ana -> pitch, GET_REAL (U"Time"), YLININV (GET_REAL (U"Frequency")));
		FunctionEditor_redraw (me);
		Editor_broadcastDataChanged (me);
	EDITOR_END
}

static void menu_cb_stylizePitch (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	EDITOR_FORM (U"Stylize pitch", U"PitchTier: Stylize...")
		REAL (U"Frequency resolution", my default_pitch_stylize_frequencyResolution ())
		RADIO (U"Units", my default_pitch_stylize_useSemitones () + 1)
			RADIOBUTTON (U"Hertz")
			RADIOBUTTON (U"semitones")
	EDITOR_OK
		SET_REAL    (U"Frequency resolution", my p_pitch_stylize_frequencyResolution)
		SET_INTEGER (U"Units",                my p_pitch_stylize_useSemitones + 1)
	EDITOR_DO
		Manipulation ana = (Manipulation) my data;
		if (! ana -> pitch) return;
		Editor_save (me, U"Stylize pitch");
		PitchTier_stylize (ana -> pitch,
			my pref_pitch_stylize_frequencyResolution () = my p_pitch_stylize_frequencyResolution = GET_REAL (U"Frequency resolution"),
			my pref_pitch_stylize_useSemitones        () = my p_pitch_stylize_useSemitones        = GET_INTEGER (U"Units") - 1);
		FunctionEditor_redraw (me);
		Editor_broadcastDataChanged (me);
	EDITOR_END
}

static void menu_cb_stylizePitch_2st (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	Manipulation ana = (Manipulation) my data;
	if (! ana -> pitch) return;
	Editor_save (me, U"Stylize pitch");
	PitchTier_stylize (ana -> pitch, 2.0, TRUE);
	FunctionEditor_redraw (me);
	Editor_broadcastDataChanged (me);
}

static void menu_cb_interpolateQuadratically (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	EDITOR_FORM (U"Interpolate quadratically", 0)
		NATURAL (U"Number of points per parabola", my default_pitch_interpolateQuadratically_numberOfPointsPerParabola ())
	EDITOR_OK
		SET_INTEGER (U"Number of points per parabola", my p_pitch_interpolateQuadratically_numberOfPointsPerParabola)
	EDITOR_DO
		Manipulation ana = (Manipulation) my data;
		if (! ana -> pitch) return;
		Editor_save (me, U"Interpolate quadratically");
		RealTier_interpolateQuadratically (ana -> pitch,
			my pref_pitch_interpolateQuadratically_numberOfPointsPerParabola () = my p_pitch_interpolateQuadratically_numberOfPointsPerParabola = GET_INTEGER (U"Number of points per parabola"),
			my p_pitch_units == kManipulationEditor_pitchUnits_SEMITONES);
		FunctionEditor_redraw (me);
		Editor_broadcastDataChanged (me);
	EDITOR_END
}

static void menu_cb_interpolateQuadratically_4pts (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	Manipulation ana = (Manipulation) my data;
	if (! ana -> pitch) return;
	Editor_save (me, U"Interpolate quadratically");
	RealTier_interpolateQuadratically (ana -> pitch, 4, my p_pitch_units == kManipulationEditor_pitchUnits_SEMITONES);
	FunctionEditor_redraw (me);
	Editor_broadcastDataChanged (me);
}

static void menu_cb_shiftPitchFrequencies (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	EDITOR_FORM (U"Shift pitch frequencies", 0)
		REAL (U"Frequency shift", U"-20.0")
		OPTIONMENU (U"Unit", 1)
			OPTION (U"Hertz")
			OPTION (U"mel")
			OPTION (U"logHertz")
			OPTION (U"semitones")
			OPTION (U"ERB")
	EDITOR_OK
	EDITOR_DO
		Manipulation ana = (Manipulation) my data;
		int unit = GET_INTEGER (U"Unit");
		unit =
			unit == 1 ? kPitch_unit_HERTZ :
			unit == 2 ? kPitch_unit_MEL :
			unit == 3 ? kPitch_unit_LOG_HERTZ :
			unit == 4 ? kPitch_unit_SEMITONES_1 :
			kPitch_unit_ERB;
		if (! ana -> pitch) return;
		Editor_save (me, U"Shift pitch frequencies");
		try {
			PitchTier_shiftFrequencies (ana -> pitch, my d_startSelection, my d_endSelection, GET_REAL (U"Frequency shift"), unit);
			FunctionEditor_redraw (me);
			Editor_broadcastDataChanged (me);
		} catch (MelderError) {
			// the PitchTier may have partially changed
			FunctionEditor_redraw (me);
			Editor_broadcastDataChanged (me);
			throw;
		}
	EDITOR_END
}

static void menu_cb_multiplyPitchFrequencies (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	EDITOR_FORM (U"Multiply pitch frequencies", 0)
		POSITIVE (U"Factor", U"1.2")
		LABEL (U"", U"The multiplication is always done in hertz.")
	EDITOR_OK
	EDITOR_DO
		Manipulation ana = (Manipulation) my data;
		if (! ana -> pitch) return;
		Editor_save (me, U"Multiply pitch frequencies");
		PitchTier_multiplyFrequencies (ana -> pitch, my d_startSelection, my d_endSelection, GET_REAL (U"Factor"));
		FunctionEditor_redraw (me);
		Editor_broadcastDataChanged (me);
	EDITOR_END
}

static void menu_cb_setPitchRange (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	EDITOR_FORM (U"Set pitch range", 0)
		/* BUG: should include Minimum */
		REAL (U"Maximum (Hz or st)", my default_pitch_maximum ())
	EDITOR_OK
		SET_REAL (U"Maximum", my p_pitch_maximum)
	EDITOR_DO
		double maximum = GET_REAL (U"Maximum");
		if (maximum <= my pitchTier.minPeriodic)
			Melder_throw (U"Maximum pitch must be greater than ", Melder_half (my pitchTier.minPeriodic), U" ", units_strings [my p_pitch_units], U".");
		my pref_pitch_maximum () = my p_pitch_maximum = maximum;
		FunctionEditor_redraw (me);
	EDITOR_END
}

static void menu_cb_setPitchUnits (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	EDITOR_FORM (U"Set pitch units", 0)
		RADIO_ENUM (U"Pitch units", kManipulationEditor_pitchUnits, my default_pitch_units ())
	EDITOR_OK
		SET_ENUM (U"Pitch units", kManipulationEditor_pitchUnits, my p_pitch_units)
	EDITOR_DO
		enum kManipulationEditor_pitchUnits oldPitchUnits = my p_pitch_units;
		my pref_pitch_units () = my p_pitch_units = GET_ENUM (kManipulationEditor_pitchUnits, U"Pitch units");
		if (my p_pitch_units == oldPitchUnits) return;
		if (my p_pitch_units == kManipulationEditor_pitchUnits_HERTZ) {
			my p_pitch_minimum = 25.0;
			my pitchTier.minPeriodic = 50.0;
			my pref_pitch_maximum () = my p_pitch_maximum = NUMsemitonesToHertz (my p_pitch_maximum);
			my pitchTier.cursor = NUMsemitonesToHertz (my pitchTier.cursor);
		} else {
			my p_pitch_minimum = -24.0;
			my pitchTier.minPeriodic = -12.0;
			my pref_pitch_maximum () = my p_pitch_maximum = NUMhertzToSemitones (my p_pitch_maximum);
			my pitchTier.cursor = NUMhertzToSemitones (my pitchTier.cursor);
		}
		FunctionEditor_redraw (me);
	EDITOR_END
}

/***** DURATION MENU *****/

static void menu_cb_setDurationRange (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	EDITOR_FORM (U"Set duration range", 0)
		REAL (U"Minimum", my default_duration_minimum ())
		REAL (U"Maximum", my default_duration_maximum ())
	EDITOR_OK
		SET_REAL (U"Minimum", my p_duration_minimum)
		SET_REAL (U"Maximum", my p_duration_maximum)
	EDITOR_DO
		Manipulation ana = (Manipulation) my data;
		double minimum = GET_REAL (U"Minimum"), maximum = GET_REAL (U"Maximum");
		double minimumValue = ana -> duration ? RealTier_getMinimumValue (ana -> duration) : NUMundefined;
		double maximumValue = ana -> duration ? RealTier_getMaximumValue (ana -> duration) : NUMundefined;
		if (minimum > 1) Melder_throw (U"Minimum relative duration must not be greater than 1.");
		if (maximum < 1) Melder_throw (U"Maximum relative duration must not be less than 1.");
		if (minimum >= maximum) Melder_throw (U"Maximum relative duration must be greater than minimum.");
		if (NUMdefined (minimumValue) && minimum > minimumValue)
			Melder_throw (U"Minimum relative duration must not be greater than the minimum value present, "
				U"which is ", Melder_half (minimumValue), U".");
		if (NUMdefined (maximumValue) && maximum < maximumValue)
			Melder_throw (U"Maximum relative duration must not be less than the maximum value present, "
				U"which is ", Melder_half (maximumValue), U".");
		my pref_duration_minimum () = my p_duration_minimum = minimum;
		my pref_duration_maximum () = my p_duration_maximum = maximum;
		FunctionEditor_redraw (me);
	EDITOR_END
}

static void menu_cb_setDraggingStrategy (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	EDITOR_FORM (U"Set dragging strategy", U"ManipulationEditor")
		RADIO_ENUM (U"Dragging strategy", kManipulationEditor_draggingStrategy, my default_pitch_draggingStrategy ())
	EDITOR_OK
		SET_INTEGER (U"Dragging strategy", my p_pitch_draggingStrategy)
	EDITOR_DO
		my pref_pitch_draggingStrategy () = my p_pitch_draggingStrategy = GET_ENUM (kManipulationEditor_draggingStrategy, U"Dragging strategy");
	EDITOR_END
}

static void menu_cb_removeDurationPoints (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	Manipulation ana = (Manipulation) my data;
	if (! ana -> duration) return;
	Editor_save (me, U"Remove duration point(s)");
	if (my d_startSelection == my d_endSelection)
		AnyTier_removePointNear (ana -> duration, 0.5 * (my d_startSelection + my d_endSelection));
	else
		AnyTier_removePointsBetween (ana -> duration, my d_startSelection, my d_endSelection);
	FunctionEditor_redraw (me);
	Editor_broadcastDataChanged (me);
}

static void menu_cb_addDurationPointAtCursor (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	Manipulation ana = (Manipulation) my data;
	if (! ana -> duration) return;
	Editor_save (me, U"Add duration point");
	RealTier_addPoint (ana -> duration, 0.5 * (my d_startSelection + my d_endSelection), my duration.cursor);
	FunctionEditor_redraw (me);
	Editor_broadcastDataChanged (me);
}

static void menu_cb_addDurationPointAt (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	EDITOR_FORM (U"Add duration point", 0)
		REAL (U"Time (s)", U"0.0");
		REAL (U"Relative duration", U"1.0");
	EDITOR_OK
		SET_REAL (U"Time", 0.5 * (my d_startSelection + my d_endSelection))
	EDITOR_DO
		Manipulation ana = (Manipulation) my data;
		if (! ana -> duration) return;
		Editor_save (me, U"Add duration point");
		RealTier_addPoint (ana -> duration, GET_REAL (U"Time"), GET_REAL (U"Relative duration"));
		FunctionEditor_redraw (me);
		Editor_broadcastDataChanged (me);
	EDITOR_END
}

static void menu_cb_newDuration (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	Manipulation ana = (Manipulation) my data;
	Editor_save (me, U"New duration");
	forget (ana -> duration);
	ana -> duration = DurationTier_create (ana -> xmin, ana -> xmax);
	FunctionEditor_redraw (me);
	Editor_broadcastDataChanged (me);
}

static void menu_cb_forgetDuration (EDITOR_ARGS) {
	EDITOR_IAM (ManipulationEditor);
	Manipulation ana = (Manipulation) my data;
	forget (ana -> duration);
	FunctionEditor_redraw (me);
	Editor_broadcastDataChanged (me);
}
	
static void menu_cb_ManipulationEditorHelp (EDITOR_ARGS) { EDITOR_IAM (ManipulationEditor); Melder_help (U"ManipulationEditor"); }
static void menu_cb_ManipulationHelp (EDITOR_ARGS) { EDITOR_IAM (ManipulationEditor); Melder_help (U"Manipulation"); }

#define menu_cb_Synth_common(menu_cb,meth) \
static void menu_cb (EDITOR_ARGS) { \
	EDITOR_IAM (ManipulationEditor); \
	prefs_synthesisMethod = my synthesisMethod = meth; \
	updateMenus (me); \
}
menu_cb_Synth_common (menu_cb_Synth_Pulses, Manipulation_PULSES)
menu_cb_Synth_common (menu_cb_Synth_Pulses_hum, Manipulation_PULSES_HUM)
menu_cb_Synth_common (menu_cb_Synth_Pulses_Lpc, Manipulation_PULSES_LPC)
menu_cb_Synth_common (menu_cb_Synth_Pitch, Manipulation_PITCH)
menu_cb_Synth_common (menu_cb_Synth_Pitch_hum, Manipulation_PITCH_HUM)
menu_cb_Synth_common (menu_cb_Synth_Pulses_Pitch, Manipulation_PULSES_PITCH)
menu_cb_Synth_common (menu_cb_Synth_Pulses_Pitch_hum, Manipulation_PULSES_PITCH_HUM)
menu_cb_Synth_common (menu_cb_Synth_OverlapAdd_nodur, Manipulation_OVERLAPADD_NODUR)
menu_cb_Synth_common (menu_cb_Synth_OverlapAdd, Manipulation_OVERLAPADD)
menu_cb_Synth_common (menu_cb_Synth_Pitch_Lpc, Manipulation_PITCH_LPC)

void structManipulationEditor :: v_createMenus () {
	ManipulationEditor_Parent :: v_createMenus ();

	Editor_addCommand (this, U"File", U"Extract original sound", 0, menu_cb_extractOriginalSound);
	Editor_addCommand (this, U"File", U"Extract pulses", 0, menu_cb_extractPulses);
	Editor_addCommand (this, U"File", U"Extract pitch tier", 0, menu_cb_extractPitchTier);
	Editor_addCommand (this, U"File", U"Extract duration tier", 0, menu_cb_extractDurationTier);
	Editor_addCommand (this, U"File", U"Publish resynthesis", 0, menu_cb_extractManipulatedSound);
	Editor_addCommand (this, U"File", U"-- close --", 0, NULL);

	Editor_addMenu (this, U"Pulse", 0);
	Editor_addCommand (this, U"Pulse", U"Add pulse at cursor", 'P', menu_cb_addPulseAtCursor);
	Editor_addCommand (this, U"Pulse", U"Add pulse at...", 0, menu_cb_addPulseAt);
	Editor_addCommand (this, U"Pulse", U"-- remove pulses --", 0, NULL);
	Editor_addCommand (this, U"Pulse", U"Remove pulse(s)", GuiMenu_OPTION + 'P', menu_cb_removePulses);

	Editor_addMenu (this, U"Pitch", 0);
	Editor_addCommand (this, U"Pitch", U"Add pitch point at cursor", 'T', menu_cb_addPitchPointAtCursor);
	Editor_addCommand (this, U"Pitch", U"Add pitch point at time slice", 0, menu_cb_addPitchPointAtSlice);
	Editor_addCommand (this, U"Pitch", U"Add pitch point at...", 0, menu_cb_addPitchPointAt);
	Editor_addCommand (this, U"Pitch", U"-- remove pitch --", 0, NULL);
	Editor_addCommand (this, U"Pitch", U"Remove pitch point(s)", GuiMenu_OPTION + 'T', menu_cb_removePitchPoints);
	Editor_addCommand (this, U"Pitch", U"-- pitch prefs --", 0, NULL);
	Editor_addCommand (this, U"Pitch", U"Set pitch range...", 0, menu_cb_setPitchRange);
	Editor_addCommand (this, U"Pitch", U"Set pitch units...", 0, menu_cb_setPitchUnits);
	Editor_addCommand (this, U"Pitch", U"Set pitch dragging strategy...", 0, menu_cb_setDraggingStrategy);
	Editor_addCommand (this, U"Pitch", U"-- modify pitch --", 0, NULL);
	Editor_addCommand (this, U"Pitch", U"Shift pitch frequencies...", 0, menu_cb_shiftPitchFrequencies);
	Editor_addCommand (this, U"Pitch", U"Multiply pitch frequencies...", 0, menu_cb_multiplyPitchFrequencies);
	Editor_addCommand (this, U"Pitch", U"All:", GuiMenu_INSENSITIVE, menu_cb_stylizePitch);
	Editor_addCommand (this, U"Pitch", U"Stylize pitch...", 0, menu_cb_stylizePitch);
	Editor_addCommand (this, U"Pitch", U"Stylize pitch (2 st)", '2', menu_cb_stylizePitch_2st);
	Editor_addCommand (this, U"Pitch", U"Interpolate quadratically...", 0, menu_cb_interpolateQuadratically);
	Editor_addCommand (this, U"Pitch", U"Interpolate quadratically (4 pts)", '4', menu_cb_interpolateQuadratically_4pts);

	Editor_addMenu (this, U"Dur", 0);
	Editor_addCommand (this, U"Dur", U"Add duration point at cursor", 'D', menu_cb_addDurationPointAtCursor);
	Editor_addCommand (this, U"Dur", U"Add duration point at...", 0, menu_cb_addDurationPointAt);
	Editor_addCommand (this, U"Dur", U"-- remove duration --", 0, NULL);
	Editor_addCommand (this, U"Dur", U"Remove duration point(s)", GuiMenu_OPTION + 'D', menu_cb_removeDurationPoints);
	Editor_addCommand (this, U"Dur", U"-- duration prefs --", 0, NULL);
	Editor_addCommand (this, U"Dur", U"Set duration range...", 0, menu_cb_setDurationRange);
	Editor_addCommand (this, U"Dur", U"-- refresh duration --", 0, NULL);
	Editor_addCommand (this, U"Dur", U"New duration", 0, menu_cb_newDuration);
	Editor_addCommand (this, U"Dur", U"Forget duration", 0, menu_cb_forgetDuration);

	Editor_addMenu (this, U"Synth", 0);
	our synthPulsesButton = Editor_addCommand (this, U"Synth", U"Pulses --", GuiMenu_RADIO_FIRST, menu_cb_Synth_Pulses);
	our synthPulsesHumButton = Editor_addCommand (this, U"Synth", U"Pulses (hum) --", GuiMenu_RADIO_NEXT, menu_cb_Synth_Pulses_hum);

	our synthPulsesLpcButton = Editor_addCommand (this, U"Synth", U"Pulses & LPC -- (\"LPC resynthesis\")", GuiMenu_RADIO_NEXT, menu_cb_Synth_Pulses_Lpc);
	Editor_addCommand (this, U"Synth", U"-- pitch resynth --", 0, NULL);
	our synthPitchButton = Editor_addCommand (this, U"Synth", U" -- Pitch", GuiMenu_RADIO_NEXT, menu_cb_Synth_Pitch);
	our synthPitchHumButton = Editor_addCommand (this, U"Synth", U" -- Pitch (hum)", GuiMenu_RADIO_NEXT, menu_cb_Synth_Pitch_hum);
	our synthPulsesPitchButton = Editor_addCommand (this, U"Synth", U"Pulses -- Pitch", GuiMenu_RADIO_NEXT, menu_cb_Synth_Pulses_Pitch);
	our synthPulsesPitchHumButton = Editor_addCommand (this, U"Synth", U"Pulses -- Pitch (hum)", GuiMenu_RADIO_NEXT, menu_cb_Synth_Pulses_Pitch_hum);
	Editor_addCommand (this, U"Synth", U"-- full resynth --", 0, NULL);
	our synthOverlapAddButton = Editor_addCommand (this, U"Synth", U"Sound & Pulses -- Pitch & Duration  (\"Overlap-add manipulation\")", GuiMenu_RADIO_NEXT | GuiMenu_TOGGLE_ON, menu_cb_Synth_OverlapAdd);
	our synthPitchLpcButton = Editor_addCommand (this, U"Synth", U"LPC -- Pitch  (\"LPC pitch manipulation\")", GuiMenu_RADIO_NEXT, menu_cb_Synth_Pitch_Lpc);
}

void structManipulationEditor :: v_createHelpMenuItems (EditorMenu menu) {
	ManipulationEditor_Parent :: v_createHelpMenuItems (menu);
	EditorMenu_addCommand (menu, U"ManipulationEditor help", '?', menu_cb_ManipulationEditorHelp);
	EditorMenu_addCommand (menu, U"Manipulation help", 0, menu_cb_ManipulationHelp);
}

/********** DRAWING AREA **********/

static void drawSoundArea (ManipulationEditor me, double ymin, double ymax) {
	Manipulation ana = (Manipulation) my data;
	Sound sound = ana -> sound;
	PointProcess pulses = ana -> pulses;
	long first, last, i;
	Graphics_Viewport viewport = Graphics_insetViewport (my d_graphics, 0, 1, ymin, ymax);
	Graphics_setWindow (my d_graphics, 0, 1, 0, 1);
	Graphics_setColour (my d_graphics, Graphics_WHITE);
	Graphics_fillRectangle (my d_graphics, 0, 1, 0, 1);
	Graphics_setColour (my d_graphics, Graphics_BLACK);
	Graphics_rectangle (my d_graphics, 0, 1, 0, 1);
	Graphics_setTextAlignment (my d_graphics, Graphics_RIGHT, Graphics_TOP);
	Graphics_setFont (my d_graphics, kGraphics_font_TIMES);
	Graphics_text (my d_graphics, 1, 1, U"%%Sound");
	Graphics_setColour (my d_graphics, Graphics_BLUE);
	Graphics_text (my d_graphics, 1, 1 - Graphics_dyMMtoWC (my d_graphics, 3), U"%%Pulses");
	Graphics_setFont (my d_graphics, kGraphics_font_HELVETICA);

	/*
	 * Draw blue pulses.
	 */
	if (pulses) {
		Graphics_setWindow (my d_graphics, my d_startWindow, my d_endWindow, 0.0, 1.0);
		Graphics_setColour (my d_graphics, Graphics_BLUE);
		for (i = 1; i <= pulses -> nt; i ++) {
			double t = pulses -> t [i];
			if (t >= my d_startWindow && t <= my d_endWindow)
				Graphics_line (my d_graphics, t, 0.05, t, 0.95);
		}
	}

	/*
	 * Draw sound.
	 */
	if (sound && Sampled_getWindowSamples (sound, my d_startWindow, my d_endWindow, & first, & last) > 1) {
		double minimum, maximum, scaleMin, scaleMax;
		Matrix_getWindowExtrema (sound, first, last, 1, 1, & minimum, & maximum);
		if (minimum == maximum) minimum = -0.5, maximum = +0.5;

		/*
		 * Scaling.
		 */
		scaleMin = 0.83 * minimum + 0.17 * my soundmin;
		scaleMax = 0.83 * maximum + 0.17 * my soundmax;
		Graphics_setWindow (my d_graphics, my d_startWindow, my d_endWindow, scaleMin, scaleMax);
		FunctionEditor_drawRangeMark (me, scaleMin, Melder_float (Melder_half (scaleMin)), U"", Graphics_BOTTOM);
		FunctionEditor_drawRangeMark (me, scaleMax, Melder_float (Melder_half (scaleMax)), U"", Graphics_TOP);

		/*
		 * Draw dotted zero line.
		 */
		if (minimum < 0.0 && maximum > 0.0) {
			Graphics_setColour (my d_graphics, Graphics_CYAN);
			Graphics_setLineType (my d_graphics, Graphics_DOTTED);
			Graphics_line (my d_graphics, my d_startWindow, 0.0, my d_endWindow, 0.0);
			Graphics_setLineType (my d_graphics, Graphics_DRAWN);
		} 

		/*
		 * Draw samples.
		 */    
		Graphics_setColour (my d_graphics, Graphics_BLACK);
		Graphics_function (my d_graphics, sound -> z [1], first, last,
			Sampled_indexToX (sound, first), Sampled_indexToX (sound, last));
	}

	Graphics_resetViewport (my d_graphics, viewport);
}

static void drawPitchArea (ManipulationEditor me, double ymin, double ymax) {
	Manipulation ana = (Manipulation) my data;
	PointProcess pulses = ana -> pulses;
	PitchTier pitch = ana -> pitch;
	long ifirstSelected, ilastSelected, n = pitch ? pitch -> points -> size : 0, imin, imax, i;
	int cursorVisible = my d_startSelection == my d_endSelection && my d_startSelection >= my d_startWindow && my d_startSelection <= my d_endWindow;
	double minimumFrequency = YLIN (50);
	int rangePrecisions [] = { 0, 1, 2 };
	const char32 *rangeUnits [] = { U"", U" Hz", U" st" };

	/*
	 * Pitch contours.
	 */
	Graphics_Viewport viewport = Graphics_insetViewport (my d_graphics, 0, 1, ymin, ymax);
	Graphics_setWindow (my d_graphics, 0, 1, 0, 1);
	Graphics_setColour (my d_graphics, Graphics_WHITE);
	Graphics_fillRectangle (my d_graphics, 0, 1, 0, 1);
	Graphics_setColour (my d_graphics, Graphics_BLACK);
	Graphics_rectangle (my d_graphics, 0, 1, 0, 1);
	Graphics_setColour (my d_graphics, Graphics_GREEN);
	Graphics_setFont (my d_graphics, kGraphics_font_TIMES);
	Graphics_setTextAlignment (my d_graphics, Graphics_RIGHT, Graphics_TOP);
	Graphics_text (my d_graphics, 1, 1, U"%%Pitch manip");
	Graphics_setGrey (my d_graphics, 0.7);
	Graphics_text (my d_graphics, 1, 1 - Graphics_dyMMtoWC (my d_graphics, 3), U"%%Pitch from pulses");
	Graphics_setFont (my d_graphics, kGraphics_font_HELVETICA);

	Graphics_setWindow (my d_graphics, my d_startWindow, my d_endWindow, my p_pitch_minimum, my p_pitch_maximum);

	/*
	 * Draw pitch contour based on pulses.
	 */
	Graphics_setGrey (my d_graphics, 0.7);
	if (pulses) for (i = 1; i < pulses -> nt; i ++) {
		double tleft = pulses -> t [i], tright = pulses -> t [i + 1], t = 0.5 * (tleft + tright);
		if (t >= my d_startWindow && t <= my d_endWindow) {
			if (tleft != tright) {
				double f = YLIN (1 / (tright - tleft));
				if (f >= my pitchTier.minPeriodic && f <= my p_pitch_maximum) {
					Graphics_fillCircle_mm (my d_graphics, t, f, 1);
				}
			}
		}
	}
	Graphics_setGrey (my d_graphics, 0.0);

	FunctionEditor_drawGridLine (me, minimumFrequency);
	FunctionEditor_drawRangeMark (me, my p_pitch_maximum,
		Melder_fixed (my p_pitch_maximum, rangePrecisions [my p_pitch_units]), rangeUnits [my p_pitch_units], Graphics_TOP);
	FunctionEditor_drawRangeMark (me, my p_pitch_minimum,
		Melder_fixed (my p_pitch_minimum, rangePrecisions [my p_pitch_units]), rangeUnits [my p_pitch_units], Graphics_BOTTOM);
	if (my d_startSelection == my d_endSelection && my pitchTier.cursor >= my p_pitch_minimum && my pitchTier.cursor <= my p_pitch_maximum)
		FunctionEditor_drawHorizontalHair (me, my pitchTier.cursor,
			Melder_fixed (my pitchTier.cursor, rangePrecisions [my p_pitch_units]), rangeUnits [my p_pitch_units]);
	if (cursorVisible && n > 0) {
		double y = YLIN (RealTier_getValueAtTime (pitch, my d_startSelection));
		FunctionEditor_insertCursorFunctionValue (me, y,
			Melder_fixed (y, rangePrecisions [my p_pitch_units]), rangeUnits [my p_pitch_units],
			my p_pitch_minimum, my p_pitch_maximum);
	}
	if (pitch) {
		ifirstSelected = AnyTier_timeToHighIndex (pitch, my d_startSelection);
		ilastSelected = AnyTier_timeToLowIndex (pitch, my d_endSelection);
		imin = AnyTier_timeToHighIndex (pitch, my d_startWindow);
		imax = AnyTier_timeToLowIndex (pitch, my d_endWindow);
	}
	Graphics_setLineWidth (my d_graphics, 2);
	if (n == 0) {
		Graphics_setTextAlignment (my d_graphics, Graphics_CENTRE, Graphics_HALF);
		Graphics_setColour (my d_graphics, Graphics_BLACK);
		Graphics_text (my d_graphics, 0.5 * (my d_startWindow + my d_endWindow), 0.5 * (my p_pitch_minimum + my p_pitch_maximum), U"(no pitch points)");
	} else if (imax < imin) {
		double fleft = YLIN (RealTier_getValueAtTime (pitch, my d_startWindow));
		double fright = YLIN (RealTier_getValueAtTime (pitch, my d_endWindow));
		Graphics_setColour (my d_graphics, Graphics_GREEN);
		Graphics_line (my d_graphics, my d_startWindow, fleft, my d_endWindow, fright);
	} else {
		for (i = imin; i <= imax; i ++) {
			RealPoint point = (RealPoint) pitch -> points -> item [i];
			double t = point -> number, f = YLIN (point -> value);
			Graphics_setColour (my d_graphics, Graphics_GREEN);
			if (i == 1)
				Graphics_line (my d_graphics, my d_startWindow, f, t, f);
			else if (i == imin)
				Graphics_line (my d_graphics, t, f, my d_startWindow, YLIN (RealTier_getValueAtTime (pitch, my d_startWindow)));
			if (i == n)
				Graphics_line (my d_graphics, t, f, my d_endWindow, f);
			else if (i == imax)
				Graphics_line (my d_graphics, t, f, my d_endWindow, YLIN (RealTier_getValueAtTime (pitch, my d_endWindow)));
			else {
				RealPoint pointRight = (RealPoint) pitch -> points -> item [i + 1];
				Graphics_line (my d_graphics, t, f, pointRight -> number, YLIN (pointRight -> value));
			}
		}
		for (i = imin; i <= imax; i ++) {
			RealPoint point = (RealPoint) pitch -> points -> item [i];
			double t = point -> number, f = YLIN (point -> value);
			if (i >= ifirstSelected && i <= ilastSelected)
				Graphics_setColour (my d_graphics, Graphics_RED);	
			else
				Graphics_setColour (my d_graphics, Graphics_GREEN);
			Graphics_fillCircle_mm (my d_graphics, t, f, 3);
		}
	}
	Graphics_setLineWidth (my d_graphics, 1);

	Graphics_setColour (my d_graphics, Graphics_BLACK);
	Graphics_resetViewport (my d_graphics, viewport);
}

static void drawDurationArea (ManipulationEditor me, double ymin, double ymax) {
	Manipulation ana = (Manipulation) my data;
	DurationTier duration = ana -> duration;
	long ifirstSelected, ilastSelected, n = duration ? duration -> points -> size : 0, imin, imax, i;
	int cursorVisible = my d_startSelection == my d_endSelection && my d_startSelection >= my d_startWindow && my d_startSelection <= my d_endWindow;

	/*
	 * Duration contours.
	 */
	Graphics_Viewport viewport = Graphics_insetViewport (my d_graphics, 0, 1, ymin, ymax);
	Graphics_setWindow (my d_graphics, 0, 1, 0, 1);
	Graphics_setColour (my d_graphics, Graphics_WHITE);
	Graphics_fillRectangle (my d_graphics, 0, 1, 0, 1);
	Graphics_setColour (my d_graphics, Graphics_BLACK);
	Graphics_rectangle (my d_graphics, 0, 1, 0, 1);
	Graphics_setColour (my d_graphics, Graphics_GREEN);
	Graphics_setFont (my d_graphics, kGraphics_font_TIMES);
	Graphics_setTextAlignment (my d_graphics, Graphics_RIGHT, Graphics_TOP);
	Graphics_text (my d_graphics, 1, 1, U"%%Duration manip");
	Graphics_setFont (my d_graphics, kGraphics_font_HELVETICA);

	Graphics_setWindow (my d_graphics, my d_startWindow, my d_endWindow, my p_duration_minimum, my p_duration_maximum);
	FunctionEditor_drawGridLine (me, 1.0);
	FunctionEditor_drawRangeMark (me, my p_duration_maximum, Melder_fixed (my p_duration_maximum, 3), U"", Graphics_TOP);
	FunctionEditor_drawRangeMark (me, my p_duration_minimum, Melder_fixed (my p_duration_minimum, 3), U"", Graphics_BOTTOM);
	if (my d_startSelection == my d_endSelection && my duration.cursor >= my p_duration_minimum && my duration.cursor <= my p_duration_maximum)
		FunctionEditor_drawHorizontalHair (me, my duration.cursor, Melder_fixed (my duration.cursor, 3), U"");
	if (cursorVisible && n > 0) {
		double y = RealTier_getValueAtTime (duration, my d_startSelection);
		FunctionEditor_insertCursorFunctionValue (me, y, Melder_fixed (y, 3), U"", my p_duration_minimum, my p_duration_maximum);
	}

	/*
	 * Draw duration tier.
	 */
	if (duration) {
		ifirstSelected = AnyTier_timeToHighIndex (duration, my d_startSelection);
		ilastSelected = AnyTier_timeToLowIndex (duration, my d_endSelection);
		imin = AnyTier_timeToHighIndex (duration, my d_startWindow);
		imax = AnyTier_timeToLowIndex (duration, my d_endWindow);
	}
	Graphics_setLineWidth (my d_graphics, 2);
	if (n == 0) {
		Graphics_setColour (my d_graphics, Graphics_BLACK);
		Graphics_setTextAlignment (my d_graphics, Graphics_CENTRE, Graphics_HALF);
		Graphics_text (my d_graphics, 0.5 * (my d_startWindow + my d_endWindow),
			0.5 * (my p_duration_minimum + my p_duration_maximum), U"(no duration points)");
	} else if (imax < imin) {
		double fleft = RealTier_getValueAtTime (duration, my d_startWindow);
		double fright = RealTier_getValueAtTime (duration, my d_endWindow);
		Graphics_setColour (my d_graphics, Graphics_GREEN);
		Graphics_line (my d_graphics, my d_startWindow, fleft, my d_endWindow, fright);
	} else {
		for (i = imin; i <= imax; i ++) {
			RealPoint point = (RealPoint) duration -> points -> item [i];
			double t = point -> number, dur = point -> value;
			Graphics_setColour (my d_graphics, Graphics_GREEN);
			if (i == 1)
				Graphics_line (my d_graphics, my d_startWindow, dur, t, dur);
			else if (i == imin)
				Graphics_line (my d_graphics, t, dur, my d_startWindow, RealTier_getValueAtTime (duration, my d_startWindow));
			if (i == n)
				Graphics_line (my d_graphics, t, dur, my d_endWindow, dur);
			else if (i == imax)
				Graphics_line (my d_graphics, t, dur, my d_endWindow, RealTier_getValueAtTime (duration, my d_endWindow));
			else {
				RealPoint pointRight = (RealPoint) duration -> points -> item [i + 1];
				Graphics_line (my d_graphics, t, dur, pointRight -> number, pointRight -> value);
			}
		}
		for (i = imin; i <= imax; i ++) {
			RealPoint point = (RealPoint) duration -> points -> item [i];
			double t = point -> number, dur = point -> value;
			if (i >= ifirstSelected && i <= ilastSelected)
				Graphics_setColour (my d_graphics, Graphics_RED);	
			else
				Graphics_setColour (my d_graphics, Graphics_GREEN);	
			Graphics_fillCircle_mm (my d_graphics, t, dur, 3);
		}
	}

	Graphics_setLineWidth (my d_graphics, 1);
	Graphics_setColour (my d_graphics, Graphics_BLACK);
	Graphics_resetViewport (my d_graphics, viewport);
}

void structManipulationEditor :: v_draw () {
	double ysoundmin, ysoundmax;
	double ypitchmin, ypitchmax, ydurationmin, ydurationmax;
	int hasSoundArea = getSoundArea (this, & ysoundmin, & ysoundmax);
	int hasPitchArea = getPitchArea (this, & ypitchmin, & ypitchmax);
	int hasDurationArea = getDurationArea (this, & ydurationmin, & ydurationmax);

	if (hasSoundArea) drawSoundArea (this, ysoundmin, ysoundmax);
	if (hasPitchArea) drawPitchArea (this, ypitchmin, ypitchmax);
	if (hasDurationArea) drawDurationArea (this, ydurationmin, ydurationmax);

	Graphics_setWindow (our d_graphics, 0.0, 1.0, 0.0, 1.0);
	Graphics_setGrey (our d_graphics, 0.85);
	Graphics_fillRectangle (our d_graphics, -0.001, 1.001, ypitchmax, ysoundmin);
	Graphics_setGrey (our d_graphics, 0.00);
	Graphics_line (our d_graphics, 0, ysoundmin, 1, ysoundmin);
	Graphics_line (our d_graphics, 0, ypitchmax, 1, ypitchmax);
	if (hasDurationArea) {
		Graphics_setGrey (our d_graphics, 0.85);
		Graphics_fillRectangle (our d_graphics, -0.001, 1.001, ydurationmax, ypitchmin);
		Graphics_setGrey (our d_graphics, 0.00);
		Graphics_line (our d_graphics, 0, ypitchmin, 1, ypitchmin);
		Graphics_line (our d_graphics, 0, ydurationmax, 1, ydurationmax);
	}
	updateMenus (this);
}

static void drawWhileDragging (ManipulationEditor me, double xWC, double yWC, long first, long last, double dt, double df) {
	Manipulation ana = (Manipulation) my data;
	PitchTier pitch = ana -> pitch;
	(void) xWC;
	(void) yWC;

	/*
	 * Draw all selected pitch points as magenta empty circles, if inside the window.
	 */
	for (long i = first; i <= last; i ++) {
		RealPoint point = (RealPoint) pitch -> points -> item [i];
		double t = point -> number + dt, f = YLIN (point -> value) + df;
		if (t >= my d_startWindow && t <= my d_endWindow)
			Graphics_circle_mm (my d_graphics, t,
				f < my pitchTier.minPeriodic ? my pitchTier.minPeriodic : f > my p_pitch_maximum ? my p_pitch_maximum : f, 3);
	}

	if (last == first) {
		/*
		 * Draw a crosshair with time and frequency.
		 */
		RealPoint point = (RealPoint) pitch -> points -> item [first];
		double t = point -> number + dt, fWC = YLIN (point -> value) + df;
		Graphics_line (my d_graphics, t, my p_pitch_minimum, t, my p_pitch_maximum - Graphics_dyMMtoWC (my d_graphics, 4.0));
		Graphics_setTextAlignment (my d_graphics, Graphics_CENTRE, Graphics_TOP);
		Graphics_text (my d_graphics, t, my p_pitch_maximum, Melder_fixed (t, 6));
		Graphics_line (my d_graphics, my d_startWindow, fWC, my d_endWindow, fWC);
		Graphics_setTextAlignment (my d_graphics, Graphics_LEFT, Graphics_BOTTOM);
		Graphics_text (my d_graphics, my d_startWindow, fWC, Melder_fixed (fWC, 5));
	}
}

static int clickPitch (ManipulationEditor me, double xWC, double yWC, bool shiftKeyPressed) {
	Manipulation ana = (Manipulation) my data;
	PitchTier pitch = ana -> pitch;
	long inearestPoint, ifirstSelected, ilastSelected, i;
	RealPoint nearestPoint;
	double dt = 0, df = 0;
	int draggingSelection, dragHorizontal, dragVertical;

	my pitchTier.cursor = my p_pitch_minimum + yWC * (my p_pitch_maximum - my p_pitch_minimum);
	if (! pitch) {
		Graphics_resetViewport (my d_graphics, my inset);
		return my ManipulationEditor_Parent :: v_click (xWC, yWC, shiftKeyPressed);
	}
	Graphics_setWindow (my d_graphics, my d_startWindow, my d_endWindow, my p_pitch_minimum, my p_pitch_maximum);
	yWC = my pitchTier.cursor;

	/*
	 * Clicked on a pitch point?
	 */
	inearestPoint = AnyTier_timeToNearestIndex (pitch, xWC);
	if (inearestPoint == 0) {
		Graphics_resetViewport (my d_graphics, my inset);
		return my ManipulationEditor_Parent :: v_click (xWC, yWC, shiftKeyPressed);
	}
	nearestPoint = (RealPoint) pitch -> points -> item [inearestPoint];
	if (Graphics_distanceWCtoMM (my d_graphics, xWC, yWC, nearestPoint -> number, YLIN (nearestPoint -> value)) > 1.5) {
		Graphics_resetViewport (my d_graphics, my inset);
		return my ManipulationEditor_Parent :: v_click (xWC, yWC, shiftKeyPressed);
	}

	/*
	 * Clicked on a selected pitch point?
	 */
	draggingSelection = shiftKeyPressed &&
		nearestPoint -> number > my d_startSelection && nearestPoint -> number < my d_endSelection;
	if (draggingSelection) {
		ifirstSelected = AnyTier_timeToHighIndex (pitch, my d_startSelection);
		ilastSelected = AnyTier_timeToLowIndex (pitch, my d_endSelection);
		Editor_save (me, U"Drag pitch points");
	} else {
		ifirstSelected = ilastSelected = inearestPoint;
		Editor_save (me, U"Drag pitch point");
	}

	/*
	 * Drag.
	 */
	 /*
	  * Draw at the old location once.
	  * Since some systems do double buffering,
	  * the undrawing at the old position and redrawing at the new have to be bracketed by Graphics_mouseStillDown ().
	  */
	Graphics_xorOn (my d_graphics, Graphics_MAROON);
	drawWhileDragging (me, xWC, yWC, ifirstSelected, ilastSelected, dt, df);
	dragHorizontal = my p_pitch_draggingStrategy != kManipulationEditor_draggingStrategy_VERTICAL &&
		(! shiftKeyPressed || my p_pitch_draggingStrategy != kManipulationEditor_draggingStrategy_HYBRID);
	dragVertical = my p_pitch_draggingStrategy != kManipulationEditor_draggingStrategy_HORIZONTAL;
	while (Graphics_mouseStillDown (my d_graphics)) {
		double xWC_new, yWC_new;
		Graphics_getMouseLocation (my d_graphics, & xWC_new, & yWC_new);
		if (xWC_new != xWC || yWC_new != yWC) {
			drawWhileDragging (me, xWC, yWC, ifirstSelected, ilastSelected, dt, df);
			if (dragHorizontal) dt += xWC_new - xWC;
			if (dragVertical) df += yWC_new - yWC;
			xWC = xWC_new, yWC = yWC_new;
			drawWhileDragging (me, xWC, yWC, ifirstSelected, ilastSelected, dt, df);
		}
	}
	Graphics_xorOff (my d_graphics);

	/*
	 * Dragged inside window?
	 */
	if (xWC < my d_startWindow || xWC > my d_endWindow) return 1;

	/*
	 * Points not dragged past neighbours?
	 */
	{
		RealPoint *points = (RealPoint *) pitch -> points -> item;
		double newTime = points [ifirstSelected] -> number + dt;
		if (newTime < my tmin) return 1;   // outside domain
		if (ifirstSelected > 1 && newTime <= points [ifirstSelected - 1] -> number)
			return 1;   /* Past left neighbour. */
		newTime = points [ilastSelected] -> number + dt;
		if (newTime > my tmax) return 1;   // outside domain
		if (ilastSelected < pitch -> points -> size && newTime >= points [ilastSelected + 1] -> number)
			return 1;   // past right neighbour
	}

	/*
	 * Drop.
	 */
	for (i = ifirstSelected; i <= ilastSelected; i ++) {
		RealPoint point = (RealPoint) pitch -> points -> item [i];
		point -> number += dt;
		point -> value = YLININV (YLIN (point -> value) + df);
		if (point -> value < 50.0) point -> value = 50.0;
		if (point -> value > YLININV (my p_pitch_maximum)) point -> value = YLININV (my p_pitch_maximum);
	}

	/*
	 * Make sure that the same pitch points are still selected (a problem with Undo...).
	 */

	if (draggingSelection) my d_startSelection += dt, my d_endSelection += dt;
	if (my d_startSelection == my d_endSelection) {
		RealPoint point = (RealPoint) pitch -> points -> item [ifirstSelected];
		my d_startSelection = my d_endSelection = point -> number;
		my pitchTier.cursor = YLIN (point -> value);
	}

	Editor_broadcastDataChanged (me);
	return 1;   /* Update needed. */
}

static void drawDurationWhileDragging (ManipulationEditor me, double xWC, double yWC, long first, long last, double dt, double df) {
	Manipulation ana = (Manipulation) my data;
	DurationTier duration = ana -> duration;
	long i;
	(void) xWC;
	(void) yWC;

	/*
	 * Draw all selected duration points as magenta empty circles, if inside the window.
	 */
	for (i = first; i <= last; i ++) {
		RealPoint point = (RealPoint) duration -> points -> item [i];
		double t = point -> number + dt, dur = point -> value + df;
		if (t >= my d_startWindow && t <= my d_endWindow)
			Graphics_circle_mm (my d_graphics, t, dur < my p_duration_minimum ? my p_duration_minimum :
				dur > my p_duration_maximum ? my p_duration_maximum : dur, 3);
	}

	if (last == first) {
		/*
		 * Draw a crosshair with time and duration.
		 */
		RealPoint point = (RealPoint) duration -> points -> item [first];
		double t = point -> number + dt, durWC = point -> value + df;
		Graphics_line (my d_graphics, t, my p_duration_minimum, t, my p_duration_maximum - Graphics_dyMMtoWC (my d_graphics, 4.0));
		Graphics_setTextAlignment (my d_graphics, Graphics_CENTRE, Graphics_TOP);
		Graphics_text (my d_graphics, t, my p_duration_maximum, Melder_fixed (t, 6));
		Graphics_line (my d_graphics, my d_startWindow, durWC, my d_endWindow, durWC);
		Graphics_setTextAlignment (my d_graphics, Graphics_LEFT, Graphics_BOTTOM);
		Graphics_text (my d_graphics, my d_startWindow, durWC, Melder_fixed (durWC, 2));
	}
}

static int clickDuration (ManipulationEditor me, double xWC, double yWC, int shiftKeyPressed) {
	Manipulation ana = (Manipulation) my data;
	DurationTier duration = ana -> duration;
	long inearestPoint, ifirstSelected, ilastSelected, i;
	RealPoint nearestPoint;
	double dt = 0, df = 0;
	int draggingSelection;

	/*
	 * Convert from FunctionEditor's [0, 1] coordinates to world coordinates.
	 */
	yWC = my p_duration_minimum + yWC * (my p_duration_maximum - my p_duration_minimum);

	/*
	 * Move horizontal hair to clicked position.
	 */
	my duration.cursor = yWC;

	if (! duration) {
		Graphics_resetViewport (my d_graphics, my inset);
		return my ManipulationEditor_Parent :: v_click (xWC, yWC, shiftKeyPressed);
	}
	Graphics_setWindow (my d_graphics, my d_startWindow, my d_endWindow, my p_duration_minimum, my p_duration_maximum);

	/*
	 * Clicked on a duration point?
	 */
	inearestPoint = AnyTier_timeToNearestIndex (duration, xWC);
	if (inearestPoint == 0) {
		Graphics_resetViewport (my d_graphics, my inset);
		return my ManipulationEditor_Parent :: v_click (xWC, yWC, shiftKeyPressed);
	}
	nearestPoint = (RealPoint) duration -> points -> item [inearestPoint];
	if (Graphics_distanceWCtoMM (my d_graphics, xWC, yWC, nearestPoint -> number, nearestPoint -> value) > 1.5) {
		Graphics_resetViewport (my d_graphics, my inset);
		return my ManipulationEditor_Parent :: v_click (xWC, yWC, shiftKeyPressed);
	}

	/*
	 * Clicked on a selected duration point?
	 */
	draggingSelection = shiftKeyPressed &&
		nearestPoint -> number > my d_startSelection && nearestPoint -> number < my d_endSelection;
	if (draggingSelection) {
		ifirstSelected = AnyTier_timeToHighIndex (duration, my d_startSelection);
		ilastSelected = AnyTier_timeToLowIndex (duration, my d_endSelection);
		Editor_save (me, U"Drag duration points");
	} else {
		ifirstSelected = ilastSelected = inearestPoint;
		Editor_save (me, U"Drag duration point");
	}

	/*
	 * Drag.
	 */
	Graphics_xorOn (my d_graphics, Graphics_MAROON);
	drawDurationWhileDragging (me, xWC, yWC, ifirstSelected, ilastSelected, dt, df);
	while (Graphics_mouseStillDown (my d_graphics)) {
		double xWC_new, yWC_new;
		Graphics_getMouseLocation (my d_graphics, & xWC_new, & yWC_new);
		if (xWC_new != xWC || yWC_new != yWC) {
			drawDurationWhileDragging (me, xWC, yWC, ifirstSelected, ilastSelected, dt, df);
			dt += xWC_new - xWC, xWC = xWC_new;
			df += yWC_new - yWC, yWC = yWC_new;
			drawDurationWhileDragging (me, xWC_new, yWC_new, ifirstSelected, ilastSelected, dt, df);
		}
	}
	Graphics_xorOff (my d_graphics);

	/*
	 * Dragged inside window?
	 */
	if (xWC < my d_startWindow || xWC > my d_endWindow) return 1;

	/*
	 * Points not dragged past neighbours?
	 */
	{
		RealPoint *points = (RealPoint *) duration -> points -> item;
		double newTime = points [ifirstSelected] -> number + dt;
		if (newTime < my tmin) return 1;   // outside domain
		if (ifirstSelected > 1 && newTime <= points [ifirstSelected - 1] -> number)
			return 1;   /* Past left neighbour. */
		newTime = points [ilastSelected] -> number + dt;
		if (newTime > my tmax) return 1;   // outside domain
		if (ilastSelected < duration -> points -> size && newTime >= points [ilastSelected + 1] -> number)
			return 1;   // past right neighbour
	}

	/*
	 * Drop.
	 */
	for (i = ifirstSelected; i <= ilastSelected; i ++) {
		RealPoint point = (RealPoint) duration -> points -> item [i];
		point -> number += dt;
		point -> value += df;
		if (point -> value < my p_duration_minimum) point -> value = my p_duration_minimum;
		if (point -> value > my p_duration_maximum) point -> value = my p_duration_maximum;
	}

	/*
	 * Make sure that the same duration points are still selected (a problem with Undo...).
	 */

	if (draggingSelection) my d_startSelection += dt, my d_endSelection += dt;
	if (my d_startSelection == my d_endSelection) {
		RealPoint point = (RealPoint) duration -> points -> item [ifirstSelected];
		my d_startSelection = my d_endSelection = point -> number;
		my duration.cursor = point -> value;
	}

	Editor_broadcastDataChanged (me);
	return 1;   /* Update needed. */
}

int structManipulationEditor :: v_click (double xWC, double yWC, bool shiftKeyPressed) {
	double ypitchmin, ypitchmax, ydurationmin, ydurationmax;
	int hasPitchArea = getPitchArea (this, & ypitchmin, & ypitchmax);
	int hasDurationArea = getDurationArea (this, & ydurationmin, & ydurationmax);

	/*
	 * Dispatch click to clicked area.
	 */
	if (hasPitchArea && yWC > ypitchmin && yWC < ypitchmax) {   // clicked in pitch area?
		inset = Graphics_insetViewport (our d_graphics, 0, 1, ypitchmin, ypitchmax);
		return clickPitch (this, xWC, (yWC - ypitchmin) / (ypitchmax - ypitchmin), shiftKeyPressed);
	} else if (hasDurationArea && yWC > ydurationmin && yWC < ydurationmax) {   // clicked in duration area?
		inset = Graphics_insetViewport (our d_graphics, 0, 1, ydurationmin, ydurationmax);
		return clickDuration (this, xWC, (yWC - ydurationmin) / (ydurationmax - ydurationmin), shiftKeyPressed);
	}
	/*
	 * Perform the default action: move cursor or drag selection.
	 */
	return our ManipulationEditor_Parent :: v_click (xWC, yWC, shiftKeyPressed);
}

void structManipulationEditor :: v_play (double a_tmin, double a_tmax) {
	Manipulation ana = (Manipulation) our data;
	if (our shiftKeyPressed) {
		if (ana -> sound)
			Sound_playPart (ana -> sound, a_tmin, a_tmax, theFunctionEditor_playCallback, this);
	} else {
		Manipulation_playPart (ana, a_tmin, a_tmax, our synthesisMethod);
	}
}

ManipulationEditor ManipulationEditor_create (const char32 *title, Manipulation ana) {
	try {
		autoManipulationEditor me = Thing_new (ManipulationEditor);
		FunctionEditor_init (me.peek(), title, ana);

		double maximumPitchValue = RealTier_getMaximumValue (ana -> pitch);
		if (my p_pitch_units == kManipulationEditor_pitchUnits_HERTZ) {
			my p_pitch_minimum = 25.0;
			my pitchTier.minPeriodic = 50.0;
			my p_pitch_maximum = maximumPitchValue;
			my pitchTier.cursor = my p_pitch_maximum * 0.8;
			my p_pitch_maximum *= 1.2;
		} else {
			my p_pitch_minimum = -24.0;
			my pitchTier.minPeriodic = -12.0;
			my p_pitch_maximum = NUMdefined (maximumPitchValue) ? NUMhertzToSemitones (maximumPitchValue) : NUMundefined;
			my pitchTier.cursor = my p_pitch_maximum - 4.0;
			my p_pitch_maximum += 3.0;
		}
		if (my p_pitch_maximum == NUMundefined || my p_pitch_maximum < my pref_pitch_maximum ())
			my p_pitch_maximum = my pref_pitch_maximum ();

		double minimumDurationValue = ana -> duration ? RealTier_getMinimumValue (ana -> duration) : NUMundefined;
		my p_duration_minimum = NUMdefined (minimumDurationValue) ? minimumDurationValue : 1.0;
		if (my pref_duration_minimum () > 1)
			my pref_duration_minimum () = Melder_atof (my default_duration_minimum ());
		if (my p_duration_minimum > my pref_duration_minimum ())
			my p_duration_minimum = my pref_duration_minimum ();
		double maximumDurationValue = ana -> duration ? RealTier_getMaximumValue (ana -> duration) : NUMundefined;
		my p_duration_maximum = NUMdefined (maximumDurationValue) ? maximumDurationValue : 1.0;
		if (my pref_duration_maximum () < 1)
			my pref_duration_maximum () = Melder_atof (my default_duration_maximum ());
		if (my pref_duration_maximum () <= my pref_duration_minimum ()) {
			my pref_duration_minimum () = Melder_atof (my default_duration_minimum ());
			my pref_duration_maximum () = Melder_atof (my default_duration_maximum ());
		}
		if (my p_duration_maximum < my pref_duration_maximum ())
			my p_duration_maximum = my pref_duration_maximum ();
		my duration.cursor = 1.0;

		my synthesisMethod = prefs_synthesisMethod;
		if (ana -> sound)
			Matrix_getWindowExtrema (ana -> sound, 0, 0, 0, 0, & my soundmin, & my soundmax);
		if (my soundmin == my soundmax) my soundmin = -1.0, my soundmax = +1.0;
		updateMenus (me.peek());
		return me.transfer();
	} catch (MelderError) {
		Melder_throw (U"Manipulation window not created.");
	}
}

/* End of file ManipulationEditor.cpp */
