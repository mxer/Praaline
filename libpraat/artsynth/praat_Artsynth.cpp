/* praat_Artsynth.cpp
 *
 * Copyright (C) 1992-2012,2015 Paul Boersma
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

#include "Art_Speaker.h"
#include "Artword_Speaker.h"
#include "Art_Speaker_to_VocalTract.h"
#include "Artword_Speaker_Sound.h"
#include "Artword_Speaker_to_Sound.h"
#include "Artword_to_Art.h"
#include "ArtwordEditor.h"
#include "VocalTract_to_Spectrum.h"
#include "praat.h"

extern "C" Graphics Movie_create (const char32 *title, int width, int height);

#undef iam
#define iam iam_LOOP

/***** ART *****/

FORM (Art_create, U"Create a default Articulation", U"Articulatory synthesis")
	WORD (U"Name", U"articulation")
	OK
DO
	praat_new (Art_create (), GET_STRING (U"Name"));
END

FORM (Art_edit, U"View & Edit Articulation", 0)
	for (int i = 1; i <= kArt_muscle_MAX; i ++)
		REAL (kArt_muscle_getText (i), U"0.0")
	OK
{
	Art object = (Art) ONLY_OBJECT;
	for (int i = 1; i <= kArt_muscle_MAX; i ++)
		SET_REAL (kArt_muscle_getText (i), object -> art [i]);
}
DO
	Art object = (Art) ONLY_OBJECT;
	if (theCurrentPraatApplication -> batch)
		Melder_throw (U"Cannot edit an Art from batch.");
	for (int i = 1; i <= kArt_muscle_MAX; i ++)
		object -> art [i] = GET_REAL (kArt_muscle_getText (i));
END

/***** ARTWORD *****/

FORM (Artword_create, U"Create an empty Artword", U"Create Artword...")
	WORD (U"Name", U"hallo")
	POSITIVE (U"Duration (seconds)", U"1.0")
	OK
DO
	praat_new (Artword_create (GET_REAL (U"Duration")), GET_STRING (U"Name"));
END

FORM (Artword_draw, U"Draw one Artword tier", NULL)
	OPTIONMENU (U"Muscle", kArt_muscle_LUNGS)
	for (int ienum = 1; ienum <= kArt_muscle_MAX; ienum ++)
		OPTION (kArt_muscle_getText (ienum))
	BOOLEAN (U"Garnish", 1)
	OK
DO
	
	autoPraatPicture picture;
	LOOP {
		iam (Artword);
		Artword_draw (me, GRAPHICS, GET_INTEGER (U"Muscle"), GET_INTEGER (U"Garnish"));
	}
END

DIRECT (Artword_edit)
	if (theCurrentPraatApplication -> batch) Melder_throw (U"Cannot view or edit an Artword from batch.");
	WHERE (SELECTED) {
		iam_LOOP (Artword);
		autoArtwordEditor editor = ArtwordEditor_create (ID_AND_FULL_NAME, me);
		praat_installEditor (editor.transfer(), IOBJECT);
	}
END

FORM (Artword_getTarget, U"Get one Artword target", 0)
	REAL (U"Time (seconds)", U"0.0")
	OPTIONMENU (U"Muscle", kArt_muscle_LUNGS)
	for (int ienum = 1; ienum <= kArt_muscle_MAX; ienum ++)
		OPTION (kArt_muscle_getText (ienum))
	OK
DO
	LOOP {
		iam (Artword);
		double target = Artword_getTarget (me, GET_INTEGER (U"Muscle"), GET_REAL (U"Time"));
		Melder_information (target);
	}
END

DIRECT (Artword_help)
	Melder_help (U"Artword");
END

FORM (Artword_setTarget, U"Set one Artword target", 0)
	REAL (U"Time (seconds)", U"0.0")
	REAL (U"Target value (0-1)", U"0.0")
	OPTIONMENU (U"Muscle", kArt_muscle_LUNGS)
	for (int ienum = 1; ienum <= kArt_muscle_MAX; ienum ++)
		OPTION (kArt_muscle_getText (ienum))
	OK
DO
	double time = GET_REAL (U"Time");
	if (time < 0.0) Melder_throw (U"Specified time should not be less than 0.");
	LOOP {
		iam (Artword);
		Artword_setTarget (me, GET_INTEGER (U"Muscle"), time, GET_REAL (U"Target value"));
		praat_dataChanged (me);
	}
END

FORM (Artword_to_Art, U"From Artword to Art", 0)
	REAL (U"Time (seconds)", U"0.0")
	OK
DO
	LOOP {
		iam (Artword);
		autoArt thee = Artword_to_Art (me, GET_REAL (U"Time"));
		praat_new (thee.transfer(), my name);
	}
END

/***** ART & SPEAKER *****/

DIRECT (Art_Speaker_draw)
	autoPraatPicture picture;
	iam_ONLY (Art);
	thouart_ONLY (Speaker);
	Art_Speaker_draw (me, thee, GRAPHICS);
END

DIRECT (Art_Speaker_fillInnerContour)
	autoPraatPicture picture;
	iam_ONLY (Art);
	thouart_ONLY (Speaker);
	Art_Speaker_fillInnerContour (me, thee, GRAPHICS);
END

DIRECT (Art_Speaker_drawMesh)
	autoPraatPicture picture;
	iam_ONLY (Art);
	thouart_ONLY (Speaker);
	Art_Speaker_drawMesh (me, thee, GRAPHICS);
END

DIRECT (Art_Speaker_to_VocalTract)
	iam_ONLY (Art);
	thouart_ONLY (Speaker);
	autoVocalTract him = Art_Speaker_to_VocalTract (me, thee);
	praat_new (him.transfer(), my name, U"_", thy name);
END

/***** ARTWORD & SPEAKER *****/

FORM (Artword_Speaker_draw, U"Draw Artword & Speaker", 0)
	NATURAL (U"Number of steps", U"5")
	OK
DO
	autoPraatPicture picture;
	iam_ONLY (Artword);
	thouart_ONLY (Speaker);
	Artword_Speaker_draw (me, thee, GRAPHICS, GET_INTEGER (U"Number of steps"));
END

FORM (Artword_Speaker_to_Sound, U"Articulatory synthesizer", U"Artword & Speaker: To Sound...")
	POSITIVE (U"Sampling frequency (Hz)", U"22050")
	NATURAL (U"Oversampling factor", U"25")
	INTEGER (U"Width 1", U"0")
	INTEGER (U"Width 2", U"0")
	INTEGER (U"Width 3", U"0")
	INTEGER (U"Pressure 1", U"0")
	INTEGER (U"Pressure 2", U"0")
	INTEGER (U"Pressure 3", U"0")
	INTEGER (U"Velocity 1", U"0")
	INTEGER (U"Velocity 2", U"0")
	INTEGER (U"Velocity 3", U"0")
	OK
DO
	Sound w1, w2, w3, p1, p2, p3, v1, v2, v3;
	int iw1 = GET_INTEGER (U"Width 1");
	int iw2 = GET_INTEGER (U"Width 2");
	int iw3 = GET_INTEGER (U"Width 3");
	int ip1 = GET_INTEGER (U"Pressure 1");
	int ip2 = GET_INTEGER (U"Pressure 2");
	int ip3 = GET_INTEGER (U"Pressure 3");
	int iv1 = GET_INTEGER (U"Velocity 1");
	int iv2 = GET_INTEGER (U"Velocity 2");
	int iv3 = GET_INTEGER (U"Velocity 3");
	iam_ONLY (Artword);
	thouart_ONLY (Speaker);
	autoSound him = Artword_Speaker_to_Sound (me, thee,
			GET_REAL (U"Sampling frequency"), GET_INTEGER (U"Oversampling factor"),
			& w1, iw1, & w2, iw2, & w3, iw3,
			& p1, ip1, & p2, ip2, & p3, ip3,
			& v1, iv1, & v2, iv2, & v3, iv3);
	praat_new (him.transfer(), my name, U"_", thy name);
	if (iw1) praat_new (w1, U"width", iw1);
	if (iw2) praat_new (w2, U"width", iw2);
	if (iw3) praat_new (w3, U"width", iw3);
	if (ip1) praat_new (p1, U"pressure", ip1);
	if (ip2) praat_new (p2, U"pressure", ip2);
	if (ip3) praat_new (p3, U"pressure", ip3);
	if (iv1) praat_new (v1, U"velocity", iv1);
	if (iv2) praat_new (v2, U"velocity", iv2);
	if (iv3) praat_new (v3, U"velocity", iv3);
END

/***** ARTWORD & SPEAKER [ & SOUND ] *****/

DIRECT (Artword_Speaker_movie)
	Graphics g = Movie_create (U"Artword & Speaker movie", 300, 300);
	iam_ONLY (Artword);
	thouart_ONLY (Speaker);
	heis_ONLY (Sound);   // can be null
	Artword_Speaker_Sound_movie (me, thee, him, g);
END

/***** SPEAKER *****/

FORM (Speaker_create, U"Create a Speaker", U"Create Speaker...")
	WORD (U"Name", U"speaker")
	OPTIONMENU (U"Kind of speaker", 1)
		OPTION (U"Female")
		OPTION (U"Male")
		OPTION (U"Child")
	OPTIONMENU (U"Number of tubes in glottis", 2)
		OPTION (U"1")
		OPTION (U"2")
		OPTION (U"10")
	OK
DO
	autoSpeaker me = Speaker_create (GET_STRING (U"Kind of speaker"), Melder_atoi (GET_STRING (U"Number of tubes in glottis")));
	praat_new (me.transfer(), GET_STRING (U"Name"));
END

DIRECT (Speaker_help) Melder_help (U"Speaker"); END

/***** VOCAL TRACT *****/

FORM (VocalTract_createFromPhone, U"Create Vocal Tract from phone", U"Create Vocal Tract from phone...")
	OPTIONMENU (U"Phone", 1)
		OPTION (U"a")
		OPTION (U"e")
		OPTION (U"i")
		OPTION (U"o")
		OPTION (U"u")
		OPTION (U"y1")
		OPTION (U"y2")
		OPTION (U"y3")
		OPTION (U"jery")
		OPTION (U"p")
		OPTION (U"t")
		OPTION (U"k")
		OPTION (U"x")
		OPTION (U"pa")
		OPTION (U"ta")
		OPTION (U"ka")
		OPTION (U"pi")
		OPTION (U"ti")
		OPTION (U"ki")
		OPTION (U"pu")
		OPTION (U"tu")
		OPTION (U"ku")
	OK
DO
	autoVocalTract me = VocalTract_createFromPhone (GET_STRING (U"Phone"));
	praat_new (me.transfer(), GET_STRING (U"Phone"));
END

DIRECT (VocalTract_draw)
	autoPraatPicture picture;
	WHERE (SELECTED) {
		iam_LOOP (VocalTract);
		VocalTract_draw (me, GRAPHICS);
	}
END

FORM (VocalTract_formula, U"VocalTract Formula", U"Matrix: Formula...")
	LABEL (U"label", U"`x' is the distance form the glottis in metres, `col' is the section number, `self' is in m\u00B2")
	LABEL (U"label", U"x := x1;   for col := 1 to ncol do { self [col] := `formula' ; x := x + dx }")
	TEXTFIELD (U"formula", U"0")
	OK
DO
	LOOP {
		iam (VocalTract);
		try {
			Matrix_formula (me, GET_STRING (U"formula"), interpreter, NULL);
			praat_dataChanged (me);
		} catch (MelderError) {
			praat_dataChanged (me);
			throw;
		}
	}
END

DIRECT (VocalTract_help) Melder_help (U"VocalTract"); END

DIRECT (VocalTract_to_Matrix)
	WHERE (SELECTED) {
		iam_LOOP (VocalTract);
		autoMatrix thee = VocalTract_to_Matrix (me);
		praat_new (thee.transfer(), my name);
	}
END

FORM (VocalTract_to_Spectrum, U"From Vocal Tract to Spectrum", 0)
	LABEL (U"", U"Compute transfer function")
	NATURAL (U"Number of frequencies", U"4097")
	POSITIVE (U"Maximum frequency (Hz)", U"5000")
	REAL (U"Glottal damping", U"0.1")
	BOOLEAN (U"Radiation damping", 1)
	BOOLEAN (U"Internal damping", 1)
	OK
DO
	LOOP {
		iam (VocalTract);
		autoSpectrum thee = VocalTract_to_Spectrum (me, GET_INTEGER (U"Number of frequencies"),
			GET_REAL (U"Maximum frequency"), GET_REAL (U"Glottal damping"),
			GET_INTEGER (U"Radiation damping"), GET_INTEGER (U"Internal damping"));
		praat_new (thee.transfer(), my name);
	}
END

DIRECT (ArticulatorySynthesisTutorial) Melder_help (U"Articulatory synthesis"); END

void manual_Artsynth_init (ManPages me);

void praat_uvafon_Artsynth_init (void);
void praat_uvafon_Artsynth_init (void) {
	Thing_recognizeClassesByName (classArt, classArtword, classSpeaker, NULL);

	praat_addMenuCommand (U"Objects", U"New", U"Articulatory synthesis", 0, 0, 0);
	praat_addMenuCommand (U"Objects", U"New", U"Articulatory synthesis tutorial", 0, 1, DO_ArticulatorySynthesisTutorial);
	praat_addMenuCommand (U"Objects", U"New", U"-- new articulatory synthesis -- ", 0, 1, 0);
	praat_addMenuCommand (U"Objects", U"New", U"Create Articulation...", 0, 1, DO_Art_create);
	praat_addMenuCommand (U"Objects", U"New", U"Create Speaker...", 0, 1, DO_Speaker_create);
	praat_addMenuCommand (U"Objects", U"New", U"Create Artword...", 0, 1, DO_Artword_create);
	praat_addMenuCommand (U"Objects", U"New", U"-- new vocal tract --", 0, 1, 0);
	praat_addMenuCommand (U"Objects", U"New", U"Create Vocal Tract from phone...", 0, 1, DO_VocalTract_createFromPhone);

	praat_addAction1 (classArt, 1, U"View & Edit", 0, praat_ATTRACTIVE, DO_Art_edit);
	praat_addAction1 (classArt, 1, U"Edit", 0, praat_HIDDEN, DO_Art_edit);

	praat_addAction1 (classArtword, 0, U"Artword help", 0, 0, DO_Artword_help);
	praat_addAction1 (classArtword, 1, U"View & Edit", 0, praat_ATTRACTIVE, DO_Artword_edit);
	praat_addAction1 (classArtword, 1, U"Edit", 0, praat_HIDDEN, DO_Artword_edit);
	praat_addAction1 (classArtword, 0, U"Info", 0, 0, 0);
	praat_addAction1 (classArtword, 1, U"Get target...", 0, 0, DO_Artword_getTarget);
	praat_addAction1 (classArtword, 0, U"Draw", 0, 0, 0);
	praat_addAction1 (classArtword, 0, U"Draw...", 0, 0, DO_Artword_draw);
	praat_addAction1 (classArtword, 0, U"Modify", 0, 0, 0);
	praat_addAction1 (classArtword, 1, U"Set target...", 0, 0, DO_Artword_setTarget);
	praat_addAction1 (classArtword, 0, U"Analyse", 0, 0, 0);
	praat_addAction1 (classArtword, 0, U"To Art (slice)...", 0, 0, DO_Artword_to_Art);

	praat_addAction2 (classArt, 1, classSpeaker, 1, U"Draw", 0, 0, 0);
	praat_addAction2 (classArt, 1, classSpeaker, 1, U"Draw", 0, 0, DO_Art_Speaker_draw);
	praat_addAction2 (classArt, 1, classSpeaker, 1, U"Fill inner contour", 0, 0, DO_Art_Speaker_fillInnerContour);
	praat_addAction2 (classArt, 1, classSpeaker, 1, U"Draw mesh", 0, 0, DO_Art_Speaker_drawMesh);
	praat_addAction2 (classArt, 1, classSpeaker, 1, U"Synthesize", 0, 0, 0);
	praat_addAction2 (classArt, 1, classSpeaker, 1, U"To VocalTract", 0, 0, DO_Art_Speaker_to_VocalTract);

	praat_addAction2 (classArtword, 1, classSpeaker, 1, U"Movie", 0, 0, DO_Artword_Speaker_movie);
	praat_addAction2 (classArtword, 1, classSpeaker, 1, U"Draw", 0, 0, 0);
	praat_addAction2 (classArtword, 1, classSpeaker, 1, U"Draw...", 0, 0, DO_Artword_Speaker_draw);
	praat_addAction2 (classArtword, 1, classSpeaker, 1, U"Synthesize", 0, 0, 0);
	praat_addAction2 (classArtword, 1, classSpeaker, 1, U"To Sound...", 0, 0, DO_Artword_Speaker_to_Sound);

	praat_addAction3 (classArtword, 1, classSpeaker, 1, classSound, 1, U"Movie", 0, 0, DO_Artword_Speaker_movie);

	praat_addAction1 (classSpeaker, 0, U"Speaker help", 0, 0, DO_Speaker_help);

	praat_addAction1 (classVocalTract, 0, U"VocalTract help", 0, 0, DO_VocalTract_help);
	praat_addAction1 (classVocalTract, 0, U"Draw", 0, 0, 0);
	praat_addAction1 (classVocalTract, 0, U"Draw", 0, 0, DO_VocalTract_draw);
	praat_addAction1 (classVocalTract, 0, U"Analyse", 0, 0, 0);
	praat_addAction1 (classVocalTract, 0, U"To Spectrum...", 0, 0, DO_VocalTract_to_Spectrum);
	praat_addAction1 (classVocalTract, 0, U"Modify", 0, 0, 0);
	praat_addAction1 (classVocalTract, 0, U"Formula...", 0, 0, DO_VocalTract_formula);
	praat_addAction1 (classVocalTract, 0, U"Hack", 0, 0, 0);
	praat_addAction1 (classVocalTract, 0, U"To Matrix", 0, 0, DO_VocalTract_to_Matrix);

	manual_Artsynth_init (theCurrentPraatApplication -> manPages);
}

/* End of file praat_Artsynth.cpp */
