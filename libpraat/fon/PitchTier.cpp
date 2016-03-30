/* PitchTier.cpp
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

#include "PitchTier.h"
#include "Pitch.h"

Thing_implement (PitchTier, RealTier, 0);

void structPitchTier :: v_info () {
	structData :: v_info ();
	MelderInfo_writeLine (U"Time domain:");
	MelderInfo_writeLine (U"   Start time: ", xmin, U" seconds");
	MelderInfo_writeLine (U"   End time: ", xmax, U" seconds");
	MelderInfo_writeLine (U"   Total duration: ", xmax - xmin, U" seconds");
	MelderInfo_writeLine (U"Number of points: ", points -> size);
	MelderInfo_writeLine (U"Minimum pitch value: ", RealTier_getMinimumValue (this), U" Hz");
	MelderInfo_writeLine (U"Maximum pitch value: ", RealTier_getMaximumValue (this), U" Hz");
}

PitchTier PitchTier_create (double tmin, double tmax) {
	try {
		autoPitchTier me = Thing_new (PitchTier);
		RealTier_init (me.peek(), tmin, tmax);
		return me.transfer();
	} catch (MelderError) {
		Melder_throw (U"PitchTier not created.");
	}
}

void PitchTier_draw (PitchTier me, Graphics g, double tmin, double tmax,
	double fmin, double fmax, int garnish, const char32 *method)
{
	RealTier_draw (me, g, tmin, tmax, fmin, fmax, garnish, method, U"Frequency (Hz)");
}

PitchTier PointProcess_upto_PitchTier (PointProcess me, double frequency) {
	try {
		return (PitchTier) PointProcess_upto_RealTier (me, frequency, classPitchTier);
	} catch (MelderError) {
		Melder_throw (me, U": not converted to PitchTier.");
	}
}

void PitchTier_stylize (PitchTier me, double frequencyResolution, int useSemitones) {
	double dfmin;
	for (;;) {
		long i, imin = 0;
		dfmin = 1e300;
		for (i = 2; i <= my points -> size - 1; i ++) {
			RealPoint pm = (RealPoint) my points -> item [i];
			RealPoint pl = (RealPoint) my points -> item [i - 1];
			RealPoint pr = (RealPoint) my points -> item [i + 1];
			double expectedFrequency = pl -> value + (pr -> value - pl -> value) /
				 (pr -> number - pl -> number) * (pm -> number - pl -> number);
			double df = useSemitones ?
				12 * fabs (log (pm -> value / expectedFrequency)) / NUMln2:
				fabs (pm -> value - expectedFrequency);
			if (df < dfmin) {
				imin = i;
				dfmin = df;
			}
		}
		if (imin == 0 || dfmin > frequencyResolution) break;
		Collection_removeItem (my points, imin);
	}
}

static void PitchTier_writeToSpreadsheetFile (PitchTier me, MelderFile file, int hasHeader) {
	autofile f = Melder_fopen (file, "w");
	if (hasHeader)
		fprintf (f, "\"ooTextFile\"\n\"PitchTier\"\n%.17g %.17g %ld\n", my xmin, my xmax, my points -> size);
	for (long i = 1; i <= my points -> size; i ++) {
		RealPoint point = (RealPoint) my points -> item [i];
		fprintf (f, "%.17g\t%.17g\n", point -> number, point -> value);
	}
	f.close (file);
}

void PitchTier_writeToPitchTierSpreadsheetFile (PitchTier me, MelderFile file) {
	try {
		PitchTier_writeToSpreadsheetFile (me, file, TRUE);
	} catch (MelderError) {
		Melder_throw (me, U" not written to tab-separated PitchTier file.");
	}
}

void PitchTier_writeToHeaderlessSpreadsheetFile (PitchTier me, MelderFile file) {
	try {
		PitchTier_writeToSpreadsheetFile (me, file, FALSE);
	} catch (MelderError) {
		Melder_throw (me, U" not written to tab-separated table file.");
	}
}

void PitchTier_shiftFrequencies (PitchTier me, double tmin, double tmax, double shift, int unit) {
	try {
		for (long i = 1; i <= my points -> size; i ++) {
			RealPoint point = (RealPoint) my points -> item [i];
			double frequency = point -> value;
			if (point -> number < tmin || point -> number > tmax) continue;
			switch (unit) {
				case kPitch_unit_HERTZ: {	
					frequency += shift;
					if (frequency <= 0.0)
						Melder_throw (U"The resulting frequency has to be greater than 0 Hz.");
				} break; case kPitch_unit_MEL: {
					frequency = NUMhertzToMel (frequency) + shift;
					if (frequency <= 0.0)
						Melder_throw (U"The resulting frequency has to be greater than 0 mel.");
					frequency = NUMmelToHertz (frequency);
				} break; case kPitch_unit_LOG_HERTZ: {
					frequency = pow (10.0, log10 (frequency) + shift);
				} break; case kPitch_unit_SEMITONES_1: {
					frequency = NUMsemitonesToHertz (NUMhertzToSemitones (frequency) + shift);
				} break; case kPitch_unit_ERB: {
					frequency = NUMhertzToErb (frequency) + shift;
					if (frequency <= 0.0)
						Melder_throw (U"The resulting frequency has to be greater than 0 ERB.");
					frequency = NUMerbToHertz (frequency);
				}
			}
			point -> value = frequency;
		}
	} catch (MelderError) {
		Melder_throw (me, U": not all frequencies were shifted.");
	}
}

void PitchTier_multiplyFrequencies (PitchTier me, double tmin, double tmax, double factor) {
	Melder_assert (factor > 0.0);
	for (long i = 1; i <= my points -> size; i ++) {
		RealPoint point = (RealPoint) my points -> item [i];
		if (point -> number < tmin || point -> number > tmax) continue;
		point -> value *= factor;
	}
}

/* End of file PitchTier.cpp */
