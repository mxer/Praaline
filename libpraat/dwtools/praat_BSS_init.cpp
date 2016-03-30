/* praat_BSS_init.c
 *
 * Copyright (C) 2010-2014 David Weenink
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
	djmw 20101003
	djmw 20110101 Latest modification
*/

#include "praat.h"

#include "EEG_extensions.h"
#include "ICA.h"
#include "Sound_and_PCA.h"

#undef iam
#define iam iam_LOOP

void praat_SSCP_as_TableOfReal_init (ClassInfo klas);
void praat_TableOfReal_init (ClassInfo klas);
void praat_TableOfReal_init3 (ClassInfo klas);


/******************** EEG ********************************************/

FORM (EEG_to_CrossCorrelationTable, U"EEG: To CrossCorrelationTable", U"EEG: To CrossCorrelationTable...")
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"10.0")
	REAL (U"Lag step (s)", U"0.05")
	TEXTFIELD (U"Channel ranges", U"1:64")
	LABEL (U"", U"To supply rising or falling ranges, use e.g. 2:6 or 5:3.")
	OK
DO
	double startTime = GET_REAL (U"left Time range"), endTime = GET_REAL (U"right Time range");
	double lagTime = GET_REAL (U"Lag step");
	const char32 *channelRanges = GET_STRING (U"Channel ranges");
	LOOP {
		iam (EEG);
		autoCrossCorrelationTable cct = EEG_to_CrossCorrelationTable (me, startTime, endTime, lagTime, channelRanges);
		praat_new (cct.transfer(), my name, U"_", (long) floor (lagTime*1000)); // lagTime to ms   ppgb: geeft afrondingsfouten; waarom niet round?
	}
END

FORM (EEG_to_Covariance, U"EEG: To Covariance", U"EEG: To Covariance...")
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"10.0")
	TEXTFIELD (U"Channel ranges", U"1:64")
	LABEL (U"", U"To supply rising or falling ranges, use e.g. 2:6 or 5:3.")
	OK
DO
	double startTime = GET_REAL (U"left Time range"), endTime = GET_REAL (U"right Time range");
	const char32 *channelRanges = GET_STRING (U"Channel ranges");
	LOOP {
		iam (EEG);
		autoCovariance cov = EEG_to_Covariance (me, startTime, endTime, channelRanges);
		praat_new (cov.transfer(), my name);
	}
END

FORM (EEG_to_CrossCorrelationTables, U"EEG: To CrossCorrelationTables", U"EEG: To CrossCorrelationTables...")
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"10.0")
	POSITIVE (U"Lag step (s)", U"0.02")
	NATURAL (U"Number of cross-correlations", U"40")
	LABEL (U"", U"To supply rising or falling ranges, use e.g. 2:6 or 5:3.")
	TEXTFIELD (U"Channel ranges", U"1:64")
	OK
DO
	LOOP {
		iam (EEG);
		autoCrossCorrelationTables thee = EEG_to_CrossCorrelationTables (me, GET_REAL (U"left Time range"), GET_REAL (U"right Time range"),
			GET_REAL (U"Lag step"), GET_INTEGER (U"Number of cross-correlations"), GET_STRING (U"Channel ranges"));
		praat_new (thee.transfer(), my name);
	}
END

FORM (EEG_to_EEG_bss, U"EEG: To EEG (bss)", U"EEG: To EEG (bss)...")
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"10.0")
	NATURAL (U"Number of cross-correlations", U"40")
	POSITIVE (U"Lag step (s)", U"0.002")
	LABEL (U"", U"To supply rising or falling ranges, use e.g. 2:6 or 5:3.")
	TEXTFIELD (U"Channel ranges", U"1:64")
	LABEL (U"", U"Pre-whitening parameters")
	OPTIONMENU (U"Whitening method", 1)
		OPTION (U"No whitening")
		OPTION (U"Covariance")
		OPTION (U"Correlation")
	LABEL (U"", U"Iteration parameters")
	NATURAL (U"Maximum number of iterations", U"100")
	POSITIVE (U"Tolerance", U"0.001")
	OPTIONMENU (U"Diagonalization method", 2)
	OPTION (U"qdiag")
	OPTION (U"ffdiag")
	OK
DO
	int whiteningMethod = GET_INTEGER (U"Whitening method") - 1;
	LOOP {
		iam (EEG);
		autoEEG thee = EEG_to_EEG_bss (me, GET_REAL (U"left Time range"), GET_REAL (U"right Time range"),
			GET_INTEGER (U"Number of cross-correlations"), GET_REAL (U"Lag step"), GET_STRING (U"Channel ranges"),
			whiteningMethod, GET_INTEGER (U"Diagonalization method"),
			GET_INTEGER (U"Maximum number of iterations"), GET_REAL (U"Tolerance"));
		praat_new (thee.transfer(), my name, U"_bss");
	}
END

FORM (EEG_to_PCA, U"EEG: To PCA", U"EEG: To PCA...")
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"10.0")
	TEXTFIELD (U"Channel ranges", U"1:64")
	LABEL (U"", U"To supply rising or falling ranges, use e.g. 2:6 or 5:3.")
	OPTIONMENU (U"Use", 1)
		OPTION (U"Covariance")
		OPTION (U"Correlation")
	OK
DO
	double startTime = GET_REAL (U"left Time range"), endTime = GET_REAL (U"right Time range");
	const char32 *channelRanges = GET_STRING (U"Channel ranges");
	bool useCorrelation = GET_INTEGER (U"Use") == 2;
	LOOP {
		iam (EEG);
		autoPCA pca = EEG_to_PCA (me, startTime, endTime, channelRanges, useCorrelation);
		praat_new (pca.transfer(), my name);
	}
END

FORM (EEG_and_PCA_to_EEG_principalComponents, U"EEG & PCA: To EEG (principal components)", U"EEG & PCA: To EEG (principal components)...")
	INTEGER (U"Number of components", U"0 (=all)")
	OK
DO
	EEG me = FIRST (EEG);
	PCA thee = FIRST (PCA);
	autoEEG him = EEG_and_PCA_to_EEG_principalComponents (me, thee, GET_INTEGER (U"Number of components"));
	praat_new (him.transfer(), my name, U"_pc");
END

FORM (EEG_and_PCA_to_EEG_whiten, U"EEG & PCA: To EEG (whiten)", U"EEG & PCA: To EEG (whiten)...")
	INTEGER (U"Number of components", U"0 (=all)")
	OK
DO
	EEG me = FIRST (EEG);
	PCA thee = FIRST (PCA);
	autoEEG him = EEG_and_PCA_to_EEG_whiten (me, thee, GET_INTEGER (U"Number of components"));
	praat_new (him.transfer(), my name, U"_white");
END

FORM (EEG_to_Sound_modulated, U"EEG: To Sound (modulated)", 0)
	POSITIVE (U"Start frequency (Hz)", U"100.0")
	POSITIVE (U"Channel bandwidth (Hz)", U"100.0")
	TEXTFIELD (U"Channel ranges", U"1:64")
	LABEL (U"", U"To supply rising or falling ranges, use e.g. 2:6 or 5:3.")
	OK
DO
	LOOP {
		iam (EEG);
		autoSound thee = EEG_to_Sound_modulated (me, GET_REAL (U"Start frequency"), GET_REAL (U"Channel bandwidth"),
			GET_STRING (U"Channel ranges"));
		praat_new (thee.transfer(), my name);
	}
END

FORM (EEG_to_Sound_frequencyShifted, U"EEG: To Sound (frequency shifted)", 0)
	NATURAL (U"Channel", U"1")
	POSITIVE (U"Frequency shift (Hz)", U"100.0")
	POSITIVE (U"Sampling frequecy (Hz)", U"11025.0")
	REAL (U"Maximum amplitude", U"0.99")
	OK
DO
	long channel = GET_INTEGER (U"Channel");
	LOOP {
		iam (EEG);
		autoSound thee = EEG_to_Sound_frequencyShifted (me, channel, GET_REAL (U"Frequency shift"),
			GET_REAL (U"Sampling frequecy"), GET_REAL (U"Maximum amplitude"));
		praat_new (thee.transfer(), my name, U"_ch", channel);
	}
END

/********************** CrossCorrelationTable(s) ******************/

FORM (CrossCorrelationTables_createTestSet, U"CrossCorrelationTables: Create test set", U"CrossCorrelationTables: Create test set...")
	WORD (U"Name", U"5x5")
	NATURAL (U"Matrix dimension", U"5")
	NATURAL (U"Number of matrices", U"20")
	BOOLEAN (U"First is positive definite", 1)
	REAL (U"Sigma", U"0.02")
	OK
DO
	praat_new (CrossCorrelationTables_createTestSet (GET_INTEGER (U"Matrix dimension"),
		GET_INTEGER (U"Number of matrices"), GET_INTEGER (U"First is positive definite"), GET_REAL (U"Sigma")), GET_STRING (U"Name"));
END

FORM (CrossCorrelationTable_createSimple, U"Create simple CrossCorrelationTable", 0)
	WORD (U"Name", U"ct")
	SENTENCE (U"Cross correlations", U"1.0 0.0 1.0")
	SENTENCE (U"Centroid", U"0.0 0.0")
	POSITIVE (U"Number of samples", U"100.0")   // ppgb: wat is het punt van dit een real te laten zijn, als je het meteen omzet in een integer?
	OK
DO
	praat_new (CrossCorrelationTable_createSimple (GET_STRING (U"Cross correlations"), GET_STRING (U"Centroid"),
		(long) floor (GET_REAL (U"Number of samples"))), GET_STRING (U"Name"));
END

FORM (MixingMatrix_createSimple, U"Create simple MixingMatrix", 0)
	WORD (U"Name", U"mm")
	NATURAL (U"Number of channels", U"2")
	NATURAL (U"Number of components", U"2")
	SENTENCE (U"Mixing coefficients", U"1.0 1.0 1.0 1.0")
	OK
DO
	praat_new (MixingMatrix_createSimple (GET_INTEGER (U"Number of channels"), GET_INTEGER (U"Number of components"),
		GET_STRING (U"Mixing coefficients")), GET_STRING (U"Name"));
END

DIRECT (CrossCorrelationTable_help)
	Melder_help (U"CrossCorrelationTable");
END

FORM (Sound_and_PCA_principalComponents, U"Sound & PCA: To Sound (principal components)", 0)
	NATURAL (U"Number of components", U"10")
	OK
DO
	Sound me = FIRST (Sound);
	PCA thee = FIRST (PCA);
	praat_new (Sound_and_PCA_principalComponents (me, thee, GET_INTEGER (U"Number of components")), Thing_getName (me), U"_pc");
END

FORM (Sound_and_PCA_whitenChannels, U"Sound & PCA: To Sound (white channels)", 0)
	NATURAL (U"Number of components", U"10")
	OK
DO
	Sound me = FIRST (Sound);
	PCA thee = FIRST (PCA);
	praat_new (Sound_and_PCA_whitenChannels (me, thee, GET_INTEGER (U"Number of components")), Thing_getName (me), U"_white");
END

DIRECT (CrossCorrelationTable_to_CrossCorrelationTables)
	autoCrossCorrelationTables thee = CrossCorrelationTables_create ();
	long nrows = 0, ncols = 0, nselected = 0;
	LOOP {
		iam (CrossCorrelationTable); nselected++;
		if (nselected == 1) {
			nrows = my numberOfRows;
			ncols = my numberOfColumns;
		}
		if (my numberOfRows != nrows || my numberOfColumns != ncols) Melder_throw (U"Dimensions of table ",
			IOBJECT, U" differs from the rest.");
		autoCrossCorrelationTable myc = Data_copy (me);
		Collection_addItem (thee.peek(), myc.transfer());
	}
	praat_new (thee.transfer(), U"ct_", nselected);
END

FORM (Sound_to_Covariance_channels, U"Sound: To Covariance (channels)", U"Sound: To Covariance (channels)...")
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"10.0")
	OK
DO
	LOOP {
		iam (Sound);
		praat_new (Sound_to_Covariance_channels (me, GET_REAL (U"left Time range"), GET_REAL (U"right Time range")), my name);
	}
END

FORM (Sound_to_CrossCorrelationTable, U"Sound: To CrossCorrelationTable", U"Sound: To CrossCorrelationTable...")
    REAL (U"left Time range (s)", U"0.0")
    REAL (U"right Time range (s)", U"10.0")
    REAL (U"Lag step (s)", U"0.0")
    OK
DO
	double lagTime = fabs (GET_REAL (U"Lag step"));
    LOOP {
        iam (Sound);
        praat_new (Sound_to_CrossCorrelationTable (me, GET_REAL (U"left Time range"),
        GET_REAL (U"right Time range"), lagTime), my name);
    }
END

FORM (Sounds_to_CrossCorrelationTable_combined, U"Sound: To CrossCorrelationTable (combined)", 0)
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"10.0")
	REAL (U"Lag step (s)", U"0.0")
	OK
DO
	Sound s1 = NULL, s2 = NULL;
	LOOP {
		iam (Sound);
		(s1 ? s2 : s1) = me;
	}
	Melder_assert (s1 != NULL && s2 != NULL);
	autoCrossCorrelationTable thee = Sounds_to_CrossCorrelationTable_combined (s1, s2, GET_REAL (U"left Time range"),
		GET_REAL (U"right Time range"), GET_REAL (U"Lag step"));
	praat_new (thee.transfer(), s1 -> name, U"_", s2 -> name, U"_cc");
END

DIRECT (CrossCorrelationTables_help)
Melder_help (U"CrossCorrelationTables");
	END

FORM (CrossCorrelationTables_getDiagonalityMeasure, U"CrossCorrelationTables: Get diagonality measure", U"CrossCorrelationTables: Get diagonality measure...")
	NATURAL (U"First table", U"1")
	NATURAL (U"Last table", U"100")
	OK
DO
	LOOP {
		iam (CrossCorrelationTables);
		double dm = CrossCorrelationTables_getDiagonalityMeasure (me, 0, GET_INTEGER (U"First table"),
		GET_INTEGER (U"Last table"));
		Melder_information (dm, U" (= average sum of squared off-diagonal elements)");
	}
END

FORM (CrossCorrelationTables_extractCrossCorrelationTable, U"CrossCorrelationTables: Extract one CrossCorrelationTable", 0)
	NATURAL (U"Index", U"1")
	OK
DO
	long index = GET_INTEGER (U"Index");
	LOOP {
		iam (CrossCorrelationTables);
		if (index > my size) {
			Melder_throw (U"Index too large.");
		}
		autoCrossCorrelationTable thee = Data_copy ( (CrossCorrelationTable) my item[index]);
		praat_new (thee.transfer(), Thing_getName (me), U"_", index);
	}
END

FORM (CrossCorrelationTables_to_Diagonalizer, U"CrossCorrelationTables: To Diagonalizer", 0)
	NATURAL (U"Maximum number of iterations", U"100")
	POSITIVE (U"Tolerance", U"0.001")
	OPTIONMENU (U"Diagonalization method", 2)
	OPTION (U"qdiag")
	OPTION (U"ffdiag")
	OK
DO
	LOOP {
		iam (CrossCorrelationTables);
		praat_new (CrossCorrelationTables_to_Diagonalizer (me, GET_INTEGER (U"Maximum number of iterations"),
		GET_REAL (U"Tolerance"), GET_INTEGER (U"Diagonalization method")), my name);
	}
END

FORM (Diagonalizer_and_CrossCorrelationTables_improveDiagonality, U"Diagonalizer & CrossCorrelationTables: Improve diagonality", 0)
	NATURAL (U"Maximum number of iterations", U"100")
	POSITIVE (U"Tolerance", U"0.001")
	OPTIONMENU (U"Diagonalization method", 2)
	OPTION (U"qdiag")
	OPTION (U"ffdiag")
	OK
DO
	Diagonalizer d = FIRST (Diagonalizer);
	CrossCorrelationTables ccts = FIRST (CrossCorrelationTables);
	Diagonalizer_and_CrossCorrelationTables_improveDiagonality (d, ccts, GET_INTEGER (U"Maximum number of iterations"),
		GET_REAL (U"Tolerance"), GET_INTEGER (U"Diagonalization method"));
END

FORM (CrossCorrelationTables_and_Diagonalizer_getDiagonalityMeasure, U"CrossCorrelationTables & Diagonalizer: Get diagonality measure", 0)
	NATURAL (U"First table", U"1")
	NATURAL (U"Last table", U"100")
	OK
DO
	CrossCorrelationTables ccts = FIRST (CrossCorrelationTables);
	Diagonalizer d = FIRST (Diagonalizer);
	double dm = CrossCorrelationTables_and_Diagonalizer_getDiagonalityMeasure (ccts, d, 0, GET_INTEGER (U"First table"),
		GET_INTEGER (U"Last table"));
	Melder_information (dm, U" (= average sum of squared off-diagonal elements)");
END

DIRECT (CrossCorrelationTable_and_Diagonalizer_diagonalize)
	CrossCorrelationTable cct = FIRST (CrossCorrelationTable);
	Diagonalizer d = FIRST (Diagonalizer);
	praat_new (CrossCorrelationTable_and_Diagonalizer_diagonalize (cct, d), cct->name, U"_", d->name);
END

DIRECT (CrossCorrelationTables_and_Diagonalizer_diagonalize)
	CrossCorrelationTables ccts = FIRST (CrossCorrelationTables);
	Diagonalizer d = FIRST (Diagonalizer);
	praat_new (CrossCorrelationTables_and_Diagonalizer_diagonalize (ccts, d), ccts->name, U"_", d->name);
END

FORM (CrossCorrelationTables_and_MixingMatrix_improveUnmixing, U"", 0)
	LABEL (U"", U"Iteration parameters")
	NATURAL (U"Maximum number of iterations", U"100")
	POSITIVE (U"Tolerance", U"0.001")
	OPTIONMENU (U"Diagonalization method", 2)
	OPTION (U"qdiag")
	OPTION (U"ffdiag")
	OK
DO
	MixingMatrix mm = FIRST (MixingMatrix);
	CrossCorrelationTables ccts = FIRST (CrossCorrelationTables);
	MixingMatrix_and_CrossCorrelationTables_improveUnmixing (mm, ccts,
		GET_INTEGER (U"Maximum number of iterations"), GET_REAL (U"Tolerance"), GET_INTEGER (U"Diagonalization method"));
END

DIRECT (Diagonalizer_to_MixingMatrix)
	LOOP {
		iam (Diagonalizer);
		praat_new (Diagonalizer_to_MixingMatrix (me), my name);
	}
END

FORM (Sound_to_MixingMatrix, U"Sound: To MixingMatrix", 0)
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"10.0")
	NATURAL (U"Number of cross-correlations", U"40")
	POSITIVE (U"Lag step (s)", U"0.002")
	LABEL (U"", U"Iteration parameters")
	NATURAL (U"Maximum number of iterations", U"100")
	POSITIVE (U"Tolerance", U"0.001")
	OPTIONMENU (U"Diagonalization method", 2)
	OPTION (U"qdiag")
	OPTION (U"ffdiag")
	OK
DO
	LOOP {
		iam (Sound);
		praat_new (Sound_to_MixingMatrix (me,
			GET_REAL (U"left Time range"), GET_REAL (U"right Time range"), GET_INTEGER (U"Number of cross-correlations"),
			GET_REAL (U"Lag step"), GET_INTEGER (U"Maximum number of iterations"), GET_REAL (U"Tolerance"),
			GET_INTEGER (U"Diagonalization method")), my name);
	}
END

FORM (Sound_to_CrossCorrelationTables, U"Sound: To CrossCorrelationTables", 0)
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"10.0")
	NATURAL (U"Number of cross-correlations", U"40")
	POSITIVE (U"Lag step (s)", U"0.002")
	OK
DO
	LOOP {
		iam (Sound);
		praat_new (Sound_to_CrossCorrelationTables (me, GET_REAL (U"left Time range"), GET_REAL (U"right Time range"), GET_REAL (U"Lag step"), GET_INTEGER (U"Number of cross-correlations")), my name);
	}
END

FORM (Sound_to_Sound_bss, U"Sound: To Sound (blind source separation)", U"Sound: To Sound (blind source separation)...")
	REAL (U"left Time range (s)", U"0.0")
	REAL (U"right Time range (s)", U"10.0")
	NATURAL (U"Number of cross-correlations", U"40")
	POSITIVE (U"Lag step (s)", U"0.002")
	LABEL (U"", U"Iteration parameters")
	NATURAL (U"Maximum number of iterations", U"100")
	POSITIVE (U"Tolerance", U"0.001")
	OPTIONMENU (U"Diagonalization method", 2)
	OPTION (U"qdiag")
	OPTION (U"ffdiag")
	OK
DO
	LOOP {
		iam (Sound);
		praat_new (Sound_to_Sound_BSS (me, GET_REAL (U"left Time range"), GET_REAL (U"right Time range"),
			GET_INTEGER (U"Number of cross-correlations"), GET_REAL (U"Lag step"),
			GET_INTEGER (U"Maximum number of iterations"), GET_REAL (U"Tolerance"),
			GET_INTEGER (U"Diagonalization method")), my name, U"_bss");
	}
END

FORM (Sound_to_Sound_whiteChannels, U"Sound: To Sound (white channels)", U"Sound: To Sound (white channels)...")
    POSITIVE (U"Variance fraction to keep", U"0.99")
    OK
DO
    double varianceFraction = GET_REAL (U"Variance fraction to keep");
    if (varianceFraction > 1) varianceFraction = 1;
    long permille = (long) floor (varianceFraction * 1000.0);
    LOOP {
        iam (Sound);
        praat_new (Sound_whitenChannels (me, varianceFraction), my name, U"_", permille);
    }
END

DIRECT (Sound_and_MixingMatrix_mix)
	Sound s = FIRST (Sound);
	MixingMatrix mm = FIRST (MixingMatrix);
	praat_new (Sound_and_MixingMatrix_mix (s, mm), Thing_getName (s), U"_mixed");
END

DIRECT (Sound_and_MixingMatrix_unmix)
	Sound s = FIRST (Sound);
	MixingMatrix mm = FIRST (MixingMatrix);
	praat_new (Sound_and_MixingMatrix_unmix (s, mm), Thing_getName (s), U"_unmixed");
END

DIRECT (TableOfReal_to_MixingMatrix)
	LOOP {
		iam (TableOfReal);
		praat_new (TableOfReal_to_MixingMatrix (me), my name);
	}
END

FORM (TableOfReal_and_TableOfReal_crossCorrelations, U"TableOfReal & TableOfReal: Cross-correlations", 0)
	OPTIONMENU (U"Correlations between", 1)
	OPTION (U"Rows")
	OPTION (U"Columns")
	BOOLEAN (U"Center", 0)
	BOOLEAN (U"Normalize", 0)
	OK
DO
	TableOfReal t1 = 0, t2 = 0;
	LOOP {
		iam (TableOfReal);
		(t1 ? t2 : t1) = me;
	}
	Melder_assert (t1 != NULL && t2 != NULL);
	int by_columns = GET_INTEGER (U"Correlations between") - 1;
	praat_new (TableOfReal_and_TableOfReal_crossCorrelations (t1, t2, by_columns,
		GET_INTEGER (U"Center"), GET_INTEGER (U"Normalize")),
		(by_columns ? U"by_columns" : U"by_rows"));
END

void praat_TableOfReal_init3 (ClassInfo klas) {
	praat_TableOfReal_init (klas);
	praat_addAction1 (klas, 2, U"To TableOfReal (cross-correlations)...", 0, 0, DO_TableOfReal_and_TableOfReal_crossCorrelations);
}

void praat_BSS_init (void);
void praat_BSS_init (void) {
	Thing_recognizeClassesByName (classDiagonalizer, classMixingMatrix, classCrossCorrelationTable, classCrossCorrelationTables, NULL);

	praat_addMenuCommand (U"Objects", U"New", U"Create simple CrossCorrelationTable...", U"Create simple Covariance...", praat_HIDDEN + praat_DEPTH_1, DO_CrossCorrelationTable_createSimple);
	praat_addMenuCommand (U"Objects", U"New", U"Create test CrossCorrelationTables...", U"Create simple CrossCorrelationTable...", praat_HIDDEN + praat_DEPTH_1, DO_CrossCorrelationTables_createTestSet);
	praat_addMenuCommand (U"Objects", U"New", U"Create simple MixingMatrix...", U"Create test CrossCorrelationTables...", praat_HIDDEN + praat_DEPTH_1, DO_MixingMatrix_createSimple);

	praat_addAction1 (classCrossCorrelationTable, 0, U"CrossCorrelationTable help", 0, 0, DO_CrossCorrelationTable_help);
	praat_SSCP_as_TableOfReal_init (classCrossCorrelationTable);

	praat_addAction1 (classCrossCorrelationTable, 0, U"To CrossCorrelationTables", 0, 0, DO_CrossCorrelationTable_to_CrossCorrelationTables);

	praat_addAction1 (classCrossCorrelationTables, 0, U"CrossCorrelationTables help", 0, 0, DO_CrossCorrelationTables_help);
	praat_addAction1 (classCrossCorrelationTables, 1, U"Extract CrossCorrelationTable...", 0, 0, DO_CrossCorrelationTables_extractCrossCorrelationTable);
	praat_addAction1 (classCrossCorrelationTables, 1, U"Get diagonality measure...", 0, 0, DO_CrossCorrelationTables_getDiagonalityMeasure);
	praat_addAction1 (classCrossCorrelationTables, 0, U"To Diagonalizer...", 0, 0, DO_CrossCorrelationTables_to_Diagonalizer);

	praat_TableOfReal_init3 (classDiagonalizer);
	praat_addAction1 (classDiagonalizer, 0, U"To MixingMatrix", 0, 0, DO_Diagonalizer_to_MixingMatrix);

	praat_addAction1 (classEEG, 0, U"To Sound (mc modulated)...", U"To ERPTier...", praat_HIDDEN, DO_EEG_to_Sound_modulated);
	praat_addAction1 (classEEG, 0, U"To Sound (frequency shifted)...", U"To ERPTier...", 0, DO_EEG_to_Sound_frequencyShifted);
	praat_addAction1 (classEEG, 0, U"To PCA...", U"To ERPTier...", 0, DO_EEG_to_PCA);
	praat_addAction1 (classEEG, 0, U"To CrossCorrelationTable...", U"To PCA...", praat_HIDDEN, DO_EEG_to_CrossCorrelationTable);
	praat_addAction1 (classEEG, 0, U"To CrossCorrelationTables...", U"To PCA...", praat_HIDDEN, DO_EEG_to_CrossCorrelationTables);

	praat_addAction1 (classEEG, 0, U"To Covariance...", U"To CrossCorrelationTable...", praat_HIDDEN, DO_EEG_to_Covariance);
	praat_addAction1 (classEEG, 0, U"To EEG (bss)...", U"To CrossCorrelationTable...", praat_HIDDEN, DO_EEG_to_EEG_bss);

	praat_addAction2 (classEEG, 1, classPCA, 1, U"To EEG (principal components)...", 0, 0, DO_EEG_and_PCA_to_EEG_principalComponents);
	praat_addAction2 (classEEG, 1, classPCA, 1, U"To EEG (whiten)...", 0, 0, DO_EEG_and_PCA_to_EEG_whiten);


	praat_TableOfReal_init3 (classMixingMatrix);

	praat_addAction1 (classSound, 0, U"To MixingMatrix...",  U"Resample...", praat_HIDDEN + praat_DEPTH_1, DO_Sound_to_MixingMatrix);
    praat_addAction1 (classSound, 0, U"To CrossCorrelationTable...",  U"Resample...", 1, DO_Sound_to_CrossCorrelationTable);
    praat_addAction1 (classSound, 0, U"To Covariance (channels)...",  U"Resample...", praat_HIDDEN + praat_DEPTH_1, DO_Sound_to_Covariance_channels);
	praat_addAction1 (classSound, 0, U"To CrossCorrelationTables...",  U"Resample...", praat_HIDDEN + praat_DEPTH_1, DO_Sound_to_CrossCorrelationTables);

	praat_addAction1 (classSound, 0, U"To Sound (bss)...", U"Resample...", 1, DO_Sound_to_Sound_bss);
    praat_addAction1 (classSound, 0, U"To Sound (white channels)...", U"Resample...", 1, DO_Sound_to_Sound_whiteChannels);
    praat_addAction1 (classSound, 2, U"To CrossCorrelationTable (combined)...",  U"Cross-correlate...", 1, DO_Sounds_to_CrossCorrelationTable_combined);

	praat_addAction1 (classTableOfReal, 0, U"To MixingMatrix", U"To Configuration", praat_HIDDEN, DO_TableOfReal_to_MixingMatrix);

	praat_addAction2 (classSound, 1, classMixingMatrix, 1, U"Mix", 0, 0, DO_Sound_and_MixingMatrix_mix);
	praat_addAction2 (classSound, 1, classMixingMatrix, 1, U"Unmix", 0, 0, DO_Sound_and_MixingMatrix_unmix);

	praat_addAction2 (classSound, 1, classPCA, 1, U"To Sound (white channels)...", 0 , 0, DO_Sound_and_PCA_whitenChannels);
	praat_addAction2 (classSound, 1, classPCA, 1, U"To Sound (principal components)...", 0 , 0, DO_Sound_and_PCA_principalComponents);

	praat_addAction2 (classCrossCorrelationTable, 1, classDiagonalizer, 1, U"Diagonalize", 0 , 0, DO_CrossCorrelationTable_and_Diagonalizer_diagonalize);

	praat_addAction2 (classCrossCorrelationTables, 1, classDiagonalizer, 1, U"Get diagonality measure...", 0 , 0, DO_CrossCorrelationTables_and_Diagonalizer_getDiagonalityMeasure);
	praat_addAction2 (classCrossCorrelationTables, 1, classDiagonalizer, 1, U"Diagonalize", 0 , 0, DO_CrossCorrelationTables_and_Diagonalizer_diagonalize);
	praat_addAction2 (classCrossCorrelationTables, 1, classDiagonalizer, 1, U"Improve diagonality...", 0 , 0, DO_Diagonalizer_and_CrossCorrelationTables_improveDiagonality);

	praat_addAction2 (classCrossCorrelationTables, 1, classMixingMatrix, 1, U"Improve unmixing...", 0 , 0, DO_CrossCorrelationTables_and_MixingMatrix_improveUnmixing);

	INCLUDE_MANPAGES (manual_BSS)
}

/* End of file praat_BSS_init.c */
