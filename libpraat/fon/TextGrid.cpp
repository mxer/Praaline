/* TextGrid.cpp
 *
 * Copyright (C) 1992-2012,2014,2015 Paul Boersma
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

#include "TextGrid.h"
#include "longchar.h"

#include "oo_DESTROY.h"
#include "TextGrid_def.h"
#include "oo_COPY.h"
#include "TextGrid_def.h"
#include "oo_EQUAL.h"
#include "TextGrid_def.h"
#include "oo_CAN_WRITE_AS_ENCODING.h"
#include "TextGrid_def.h"
#include "oo_WRITE_TEXT.h"
#include "TextGrid_def.h"
#include "oo_READ_TEXT.h"
#include "TextGrid_def.h"
#include "oo_WRITE_BINARY.h"
#include "TextGrid_def.h"
#include "oo_READ_BINARY.h"
#include "TextGrid_def.h"
#include "oo_DESCRIPTION.h"
#include "TextGrid_def.h"

#include "TextGrid_extensions.h"

Thing_implement (TextPoint, AnyPoint, 0);
#ifdef PRAAT_LIB
const char32 *TextPoint_getText(TextPoint me) {
	return my mark;
}
#endif

TextPoint TextPoint_create (double time, const char32 *mark) {
	try {
		autoTextPoint me = Thing_new (TextPoint);
		my number = time;
		my mark = Melder_dup (mark);
		return me.transfer();
	} catch (MelderError) {
		Melder_throw (U"Text point not created.");
	}
}

void TextPoint_setText (TextPoint me, const char32 *text) {
	try {
		/*
		 * Be fast if the string pointers are equal.
		 */
		if (text == my mark) return;
		/*
		 * Create a temporary variable (i.e. a temporary pointer to the final string),
		 * in order that 'my text' does not change in case of error.
		 */
		autostring32 newText = Melder_dup (text);
		Melder_free (my mark);
		my mark = newText.transfer();
	} catch (MelderError) {
		Melder_throw (me, U": text not set.");
	}
}

Thing_implement (TextInterval, Function, 0);

TextInterval TextInterval_create (double tmin, double tmax, const char32 *text) {
	try {
		autoTextInterval me = Thing_new (TextInterval);
		my xmin = tmin;
		my xmax = tmax;
		my text = Melder_dup (text);
		return me.transfer();
	} catch (MelderError) {
		Melder_throw (U"Text interval not created.");
	}
}

void TextInterval_setText (TextInterval me, const char32 *text) {
	try {
		/*
		 * Be fast if the string pointers are equal.
		 */
		if (text == my text) return;
		/*
		 * Create without change.
		 */
		autostring32 newText = Melder_dup (text);
		/*
		 * Change without error.
		 */
		Melder_free (my text);
		my text = newText.transfer();
	} catch (MelderError) {
		Melder_throw (U"Text interval: text not set.");
	}
}

Thing_implement (TextTier, Function, 0);

#ifdef PRAAT_LIB
const char32 *TextInterval_getText(TextInterval me) {
	return my text;
}

long TextTier_numberOfPoints (I) { iam(TextTier); return my points -> size; }
TextPoint TextTier_point (I, long i) { iam(TextTier); return static_cast <TextPoint> (my points -> item [i]); }

int TextTier_domainQuantity (I) { return MelderQuantity_TIME_SECONDS; }

void TextTier_shiftX (I, double xfrom, double xto) {
	iam(TextTier);
	me -> v_shiftX (xfrom, xto);
}

void TextTier_scaleX (I, double xminfrom, double xmaxfrom, double xminto, double xmaxto) {
	iam(TextTier);
	me -> v_scaleX (xminfrom, xmaxfrom, xminto, xmaxto);
}


#endif

void structTextTier :: v_shiftX (double xfrom, double xto) {
	TextTier_Parent :: v_shiftX (xfrom, xto);
	for (long i = 1; i <= points -> size; i ++) {
		TextPoint point = (TextPoint) points -> item [i];
		NUMshift (& point -> number, xfrom, xto);
	}
}

void structTextTier :: v_scaleX (double xminfrom, double xmaxfrom, double xminto, double xmaxto) {
	TextTier_Parent :: v_scaleX (xminfrom, xmaxfrom, xminto, xmaxto);
	for (long i = 1; i <= points -> size; i ++) {
		TextPoint point = (TextPoint) points -> item [i];
		NUMscale (& point -> number, xminfrom, xmaxfrom, xminto, xmaxto);
	}
}

TextTier TextTier_create (double tmin, double tmax) {
	try {
		autoTextTier me = Thing_new (TextTier);
		my points = SortedSetOfDouble_create ();
		my xmin = tmin;
		my xmax = tmax;
		return me.transfer();
	} catch (MelderError) {
		Melder_throw (U"Point tier not created.");
	}
}

void TextTier_addPoint (TextTier me, double time, const char32 *mark) {
	try {
		Collection_addItem (my points, TextPoint_create (time, mark));
	} catch (MelderError) {
		Melder_throw (U"Point tier: point not added.");
	}
}

Thing_implement (IntervalTier, Function, 0);

#ifdef PRAAT_LIB
long IntervalTier_numberOfIntervals (I) { iam(IntervalTier); return my intervals -> size; }

TextInterval IntervalTier_interval (I, long i) { iam(IntervalTier); return static_cast <TextInterval> (my intervals -> item [i]); }

int IntervalTier_domainQuantity (I) { return MelderQuantity_TIME_SECONDS; }

void IntervalTier_shiftX (I, double xfrom, double xto) {
	iam(IntervalTier);
	me -> v_shiftX(xfrom, xto);
}

void IntervalTier_scaleX (I, double xminfrom, double xmaxfrom, double xminto, double xmaxto) {
	iam(IntervalTier);
	me -> v_scaleX(xminfrom, xmaxfrom, xminto, xmaxto);
}

static void IntervalTier_addInterval_unsafe(IntervalTier me, double tmin, double tmax, const char32 *label);
void IntervalTier_addInterval (IntervalTier me, double tmin, double tmax, const char32 *label) {
	IntervalTier_addInterval_unsafe(me, tmin, tmax, label);
}

void IntervalTier_removeInterval (IntervalTier me, long iinterval) {
	Collection_removeItem(my intervals, iinterval);
}
#endif

void structIntervalTier :: v_shiftX (double xfrom, double xto) {
	IntervalTier_Parent :: v_shiftX (xfrom, xto);
	for (long i = 1; i <= intervals -> size; i ++) {
		TextInterval interval = (TextInterval) intervals -> item [i];
		interval -> v_shiftX (xfrom, xto);
	}
}

void structIntervalTier :: v_scaleX (double xminfrom, double xmaxfrom, double xminto, double xmaxto) {
	IntervalTier_Parent :: v_scaleX (xminfrom, xmaxfrom, xminto, xmaxto);
	for (long i = 1; i <= intervals -> size; i ++) {
		TextInterval interval = (TextInterval) intervals -> item [i];
		interval -> v_scaleX (xminfrom, xmaxfrom, xminto, xmaxto);
	}
}

IntervalTier IntervalTier_create (double tmin, double tmax) {
	try {
		autoIntervalTier me = Thing_new (IntervalTier);
		my intervals = SortedSetOfDouble_create ();
		my xmin = tmin;
		my xmax = tmax;
		autoTextInterval interval = TextInterval_create (tmin, tmax, NULL);
		Collection_addItem (my intervals, interval.transfer());
		return me.transfer();
	} catch (MelderError) {
		Melder_throw (U"Interval tier not created.");
	}
}

long IntervalTier_timeToLowIndex (IntervalTier me, double t) {
	long ileft = 1, iright = my intervals -> size;
	if (iright < 1) return 0;   // empty tier
	TextInterval leftInterval = (TextInterval) my intervals -> item [ileft];
	if (t < leftInterval -> xmin) return 0;   // very small t
	TextInterval rightInterval = (TextInterval) my intervals -> item [iright];
	if (t >= rightInterval -> xmax) return 0;   // very large t
	while (ileft < iright) {
		long imid = (ileft + iright) / 2;
		TextInterval midInterval = (TextInterval) my intervals -> item [imid];
		if (t >= midInterval -> xmax) {
			ileft = imid + 1;
		} else {
			iright = imid;
		}
	}
	return ileft;
}

long IntervalTier_timeToIndex (IntervalTier me, double t) {
	long ileft = 1, iright = my intervals -> size;
	if (iright < 1) return 0;   // empty tier
	TextInterval leftInterval = (TextInterval) my intervals -> item [ileft];
	if (t < leftInterval -> xmin) return 0;   // very small t
	TextInterval rightInterval = (TextInterval) my intervals -> item [iright];
	if (t > rightInterval -> xmax) return 0;   // very large t
	while (ileft < iright) {
		long imid = (ileft + iright) / 2;
		TextInterval midInterval = (TextInterval) my intervals -> item [imid];
		if (t >= midInterval -> xmax) {
			ileft = imid + 1;
		} else {
			iright = imid;
		}
	}
	return ileft;
}

long IntervalTier_timeToHighIndex (IntervalTier me, double t) {
	long ileft = 1, iright = my intervals -> size;
	if (iright < 1) return 0;   // empty tier
	TextInterval leftInterval = (TextInterval) my intervals -> item [ileft];
	if (t <= leftInterval -> xmin) return 0;   // very small t
	TextInterval rightInterval = (TextInterval) my intervals -> item [iright];
	if (t > rightInterval -> xmax) return 0;   // very large t
	while (ileft < iright) {
		long imid = (ileft + iright) / 2;
		TextInterval midInterval = (TextInterval) my intervals -> item [imid];
		if (t > midInterval -> xmax) {
			ileft = imid + 1;
		} else {
			iright = imid;
		}
	}
	return ileft;
}

long IntervalTier_hasTime (IntervalTier me, double t) {
	long ileft = 1, iright = my intervals -> size;
	if (iright < 1) return 0;   // empty tier
	TextInterval leftInterval = (TextInterval) my intervals -> item [ileft];
	if (t < leftInterval -> xmin) return 0;   // very small t
	TextInterval rightInterval = (TextInterval) my intervals -> item [iright];
	if (t > rightInterval -> xmax) return 0;   // very large t
	while (ileft < iright) {
		long imid = (ileft + iright) / 2;
		TextInterval midInterval = (TextInterval) my intervals -> item [imid];
		if (t >= midInterval -> xmax) {
			ileft = imid + 1;
		} else {
			iright = imid;
		}
	}
	/*
	 * We now know that t is within interval ileft.
	 */
	leftInterval = (TextInterval) my intervals -> item [ileft];
	if (t == leftInterval -> xmin || t == leftInterval -> xmax) return ileft;
	return 0;   // not found
}

long IntervalTier_hasBoundary (IntervalTier me, double t) {
	long ileft = 2, iright = my intervals -> size;
	if (iright < 2) return 0;   // tier without inner boundaries
	TextInterval leftInterval = (TextInterval) my intervals -> item [ileft];
	if (t < leftInterval -> xmin) return 0;   // very small t
	TextInterval rightInterval = (TextInterval) my intervals -> item [iright];
	if (t >= rightInterval -> xmax) return 0;   // very large t
	while (ileft < iright) {
		long imid = (ileft + iright) / 2;
		TextInterval midInterval = (TextInterval) my intervals -> item [imid];
		if (t >= midInterval -> xmax) {
			ileft = imid + 1;
		} else {
			iright = imid;
		}
	}
	leftInterval = (TextInterval) my intervals -> item [ileft];
	if (t == leftInterval -> xmin) return ileft;
	return 0;   // not found
}

void structTextGrid :: v_info () {
	long ntier = tiers -> size;
	long numberOfIntervalTiers = 0, numberOfPointTiers = 0, numberOfIntervals = 0, numberOfPoints = 0;
	structData :: v_info ();
	for (long itier = 1; itier <= ntier; itier ++) {
		Function anyTier = (Function) tiers -> item [itier];
		if (anyTier -> classInfo == classIntervalTier) {
			IntervalTier tier = (IntervalTier) anyTier;
			numberOfIntervalTiers += 1;
			numberOfIntervals += tier -> intervals -> size;
		} else {
			TextTier tier = (TextTier) anyTier;
			numberOfPointTiers += 1;
			numberOfPoints += tier -> points -> size;
		}
	}
	MelderInfo_writeLine (U"Number of interval tiers: ", numberOfIntervalTiers);
	MelderInfo_writeLine (U"Number of point tiers: ", numberOfPointTiers);
	MelderInfo_writeLine (U"Number of intervals: ", numberOfIntervals);
	MelderInfo_writeLine (U"Number of points: ", numberOfPoints);
}

static void IntervalTier_addInterval_unsafe (IntervalTier me, double tmin, double tmax, const char32 *label) {
	autoTextInterval interval = TextInterval_create (tmin, tmax, label);
	Collection_addItem (my intervals, interval.transfer());
}

void structTextGrid :: v_repair () {
	for (long itier = 1; itier <= our numberOfTiers (); itier ++) {
		Function anyTier = our tier (itier);
		if (anyTier -> classInfo == classIntervalTier) {
			IntervalTier tier = (IntervalTier) anyTier;
			if (tier -> numberOfIntervals () == 0) {
				IntervalTier_addInterval_unsafe (tier, tier -> xmin, tier -> xmax, U"");
			}
		}
	}
}

void structTextGrid :: v_shiftX (double xfrom, double xto) {
	TextGrid_Parent :: v_shiftX (xfrom, xto);
	for (long i = 1; i <= tiers -> size; i ++) {
		Function tier = (Function) tiers -> item [i];
		tier -> v_shiftX (xfrom, xto);
	}
}

void structTextGrid :: v_scaleX (double xminfrom, double xmaxfrom, double xminto, double xmaxto) {
	TextGrid_Parent :: v_scaleX (xminfrom, xmaxfrom, xminto, xmaxto);
	for (long i = 1; i <= tiers -> size; i ++) {
		Function tier = (Function) tiers -> item [i];
		tier -> v_scaleX (xminfrom, xmaxfrom, xminto, xmaxto);
	}
}

Thing_implement (TextGrid, Function, 0);

#ifdef PRAAT_LIB
long TextGrid_numberOfTiers (I) { iam(TextGrid); return my tiers -> size; }

Function TextGrid_tier (I, long i) { iam(TextGrid); return static_cast <Function> (my tiers -> item [i]); }

void TextGrid_repair (I) {
	iam(TextGrid);
	me -> v_repair();
}

int TextGrid_domainQuantity (I) { return MelderQuantity_TIME_SECONDS; }

void TextGrid_shiftX (I, double xfrom, double xto) {
	iam(TextGrid);
	me -> v_shiftX(xfrom, xto);
}

void TextGrid_scaleX (I, double xminfrom, double xmaxfrom, double xminto, double xmaxto) {
	iam(TextGrid);
	me -> v_scaleX (xminfrom, xmaxfrom, xminto, xmaxto);
}
#endif

TextGrid TextGrid_createWithoutTiers (double tmin, double tmax) {
	try {
		autoTextGrid me = Thing_new (TextGrid);
		my tiers = Ordered_create ();
		my xmin = tmin;
		my xmax = tmax;
		return me.transfer();
	} catch (MelderError) {
		Melder_throw (U"TextGrid not created.");
	}
}

TextGrid TextGrid_create (double tmin, double tmax, const char32 *tierNames, const char32 *pointTiers) {
	try {
		autoTextGrid me = TextGrid_createWithoutTiers (tmin, tmax);
		char32 nameBuffer [400];

		/*
		 * Create a number of IntervalTier objects.
		 */
		if (tierNames && tierNames [0]) {
			str32cpy (nameBuffer, tierNames);
			for (char32 *tierName = Melder_tok (nameBuffer, U" "); tierName != NULL; tierName = Melder_tok (NULL, U" ")) {
				autoIntervalTier tier = IntervalTier_create (tmin, tmax);
				Thing_setName (tier.peek(), tierName);
				Collection_addItem (my tiers, tier.transfer());
			}
		}

		/*
		 * Replace some IntervalTier objects with TextTier objects.
		 */
		if (pointTiers && pointTiers [0]) {
			str32cpy (nameBuffer, pointTiers);
			for (char32 *tierName = Melder_tok (nameBuffer, U" "); tierName != NULL; tierName = Melder_tok (NULL, U" ")) {
				for (long itier = 1; itier <= my tiers -> size; itier ++) {
					if (str32equ (tierName, Thing_getName ((Thing) my tiers -> item [itier]))) {
						autoTextTier tier = TextTier_create (tmin, tmax);
						Thing_setName (tier.peek(), tierName);
						forget (((Function *) my tiers -> item) [itier]);
						my tiers -> item [itier] = tier.transfer();
					}
				}
			}
		}
		if (my tiers -> size == 0)
			Melder_throw (U"Cannot create a TextGrid without tiers. Supply at least one tier name.");
		return me.transfer();
	} catch (MelderError) {
		Melder_throw (U"TextGrid not created.");
	}
}

TextTier TextTier_readFromXwaves (MelderFile file) {
	try {
		char *line;

		autoTextTier me = TextTier_create (0, 100);
		autoMelderFile mfile = MelderFile_open (file);

		/*
		 * Search for a line that starts with '#'.
		 */
		for (;;) {
			line = MelderFile_readLine (file);
			if (line == NULL)
				Melder_throw (U"Missing '#' line.");
			if (line [0] == '#') break;
		}

		/*
		 * Read a mark from every line.
		 */
		for (;;) {
			line = MelderFile_readLine (file);
			if (line == NULL) break;   // normal end-of-file
			double time;
			long colour;
			char mark [300];
			if (sscanf (line, "%lf%ld%s", & time, & colour, mark) < 3)   // BUG: buffer overflow
				Melder_throw (U"Line too short: \"", Melder_peek8to32 (line), U"\".");
			TextTier_addPoint (me.peek(), time, Melder_peek8to32 (mark));
		}

		/*
		 * Fix domain.
		 */
		if (my points -> size) {
			TextPoint point = (TextPoint) my points -> item [1];
			if (point -> number < 0.0) my xmin = point -> number - 1.0;
			point = (TextPoint) my points -> item [my points -> size];
			my xmax = point -> number + 1.0;
		}
		mfile.close ();
		return me.transfer();
	} catch (MelderError) {
		Melder_throw (U"TextTier not read from Xwaves file.");
	}
}

Function TextGrid_checkSpecifiedTierNumberWithinRange (TextGrid me, long tierNumber) {
	if (tierNumber < 1)
		Melder_throw (me, U": the specified tier number is ", tierNumber, U", but should be at least 1.");
	if (tierNumber > my tiers -> size)
		Melder_throw (me, U": the specified tier number (", tierNumber, U") exceeds my number of tiers (", my tiers -> size, U").");
	return my tier (tierNumber);
}

IntervalTier TextGrid_checkSpecifiedTierIsIntervalTier (TextGrid me, long tierNumber) {
	Function tier = TextGrid_checkSpecifiedTierNumberWithinRange (me, tierNumber);
	if (tier -> classInfo != classIntervalTier)
		Melder_throw (U"Tier ", tierNumber, U" is not an interval tier.");
	return static_cast <IntervalTier> (tier);
}

TextTier TextGrid_checkSpecifiedTierIsPointTier (TextGrid me, long tierNumber) {
	Function tier = TextGrid_checkSpecifiedTierNumberWithinRange (me, tierNumber);
	if (tier -> classInfo != classTextTier)
		Melder_throw (U"Tier ", tierNumber, U" is not a point tier.");
	return static_cast <TextTier> (tier);
}

long TextGrid_countLabels (TextGrid me, long tierNumber, const char32 *text) {
	try {
		Function anyTier = TextGrid_checkSpecifiedTierNumberWithinRange (me, tierNumber);
		long count = 0;
		if (anyTier -> classInfo == classIntervalTier) {
			IntervalTier tier = (IntervalTier) anyTier;
			for (long i = 1; i <= tier -> numberOfIntervals (); i ++) {
				TextInterval segment = tier -> interval (i);
				if (segment -> text && str32equ (segment -> text, text))
					count ++;
			}
		} else {
			TextTier tier = (TextTier) anyTier;
			for (long i = 1; i <= tier -> numberOfPoints (); i ++) {
				TextPoint point = tier -> point (i);
				if (point -> mark && str32equ (point -> mark, text))
					count ++;
			}
		}
		return count;
	} catch (MelderError) {
		Melder_throw (me, U": labels not counted.");
	}
}

long TextGrid_countIntervalsWhere (TextGrid me, long tierNumber, int which_Melder_STRING, const char32 *criterion) {
	try {
		long count = 0;
		IntervalTier tier = TextGrid_checkSpecifiedTierIsIntervalTier (me, tierNumber);
		for (long iinterval = 1; iinterval <= tier -> numberOfIntervals (); iinterval ++) {
			TextInterval interval = tier -> interval (iinterval);
			if (Melder_stringMatchesCriterion (interval -> text, which_Melder_STRING, criterion)) {
				count ++;
			}
		}
		return count;
	} catch (MelderError) {
		Melder_throw (me, U": intervals not counted.");
	}
}

long TextGrid_countPointsWhere (TextGrid me, long tierNumber, int which_Melder_STRING, const char32 *criterion) {
	try {
		long count = 0;
		TextTier tier = TextGrid_checkSpecifiedTierIsPointTier (me, tierNumber);
		for (long ipoint = 1; ipoint <= tier -> numberOfPoints (); ipoint ++) {
			TextPoint point = tier -> point (ipoint);
			if (Melder_stringMatchesCriterion (point -> mark, which_Melder_STRING, criterion)) {
				count ++;
			}
		}
		return count;
	} catch (MelderError) {
		Melder_throw (me, U": points not counted.");
	}
}

void TextGrid_addTier (TextGrid me, Function anyTier) {
	try {
		autoFunction tier = Data_copy (anyTier);
		if (tier -> xmin < my xmin) my xmin = tier -> xmin;
		if (tier -> xmax > my xmax) my xmax = tier -> xmax;
		Collection_addItem (my tiers, tier.transfer());
	} catch (MelderError) {
		Melder_throw (me, U": tier not added.");
	}
}

TextGrid TextGrid_merge (Collection textGrids) {
	try {
		if (textGrids -> size < 1)
			Melder_throw (U"Cannot merge zero TextGrid objects.");
		autoTextGrid thee = Data_copy ((TextGrid) textGrids -> item [1]);
		for (long igrid = 2; igrid <= textGrids -> size; igrid ++) {
			TextGrid textGrid = (TextGrid) textGrids -> item [igrid];
			for (long itier = 1; itier <= textGrid -> tiers -> size; itier ++) {
				TextGrid_addTier (thee.peek(), (TextGrid) textGrid -> tiers -> item [itier]);
			}
		}
		return thee.transfer();
	} catch (MelderError) {
		Melder_throw (U"TextGrids not merged.");
	}
}

TextGrid TextGrid_extractPart (TextGrid me, double tmin, double tmax, int preserveTimes) {
	try {
		autoTextGrid thee = (TextGrid) Data_copy (me);
		int ntier = my tiers -> size;
		if (tmax <= tmin) return thee.transfer();

		for (long itier = 1; itier <= ntier; itier ++) {
			Function anyTier = (Function) thy tiers -> item [itier];
			if (anyTier -> classInfo == classIntervalTier) {
				IntervalTier tier = (IntervalTier) anyTier;
				long ninterval = tier -> intervals -> size;
				for (long iinterval = ninterval; iinterval >= 1; iinterval --) {
					TextInterval interval = (TextInterval) tier -> intervals -> item [iinterval];
					if (interval -> xmin >= tmax || interval -> xmax <= tmin) {
						Collection_removeItem (tier -> intervals, iinterval);
					} else {
						if (interval -> xmin < tmin) interval -> xmin = tmin;
						if (interval -> xmax > tmax) interval -> xmax = tmax;
					}
				}
			} else {
				TextTier tier = (TextTier) anyTier;
				long n = tier -> points -> size;
				for (long i = n; i >= 1; i --) {
					TextPoint point = (TextPoint) tier -> points -> item [i];
					if (point -> number < tmin || point -> number > tmax) Collection_removeItem (tier -> points, i);
				}
			}
			anyTier -> xmin = tmin;
			anyTier -> xmax = tmax;
		}
		thy xmin = tmin;
		thy xmax = tmax;
		if (! preserveTimes) Function_shiftXTo (thee.peek(), thy xmin, 0.0);
		return thee.transfer();
	} catch (MelderError) {
		Melder_throw (me, U": part not extracted.");
	}
}

static TextGrid _Label_to_TextGrid (Label me, double tmin, double tmax) {
	autoTextGrid thee = TextGrid_createWithoutTiers (tmin, tmax);
	for (long itier = 1; itier <= my size; itier ++) {
		Tier tier = (Tier) my item [itier];
		autoIntervalTier intervalTier = IntervalTier_create (tmin, tmax);
		Collection_addItem (thy tiers, intervalTier.transfer());
		Collection_removeItem (intervalTier -> intervals, 1);
		for (long iinterval = 1; iinterval <= tier -> size; iinterval ++) {
			Autosegment autosegment = (Autosegment) tier -> item [iinterval];
			autoTextInterval textInterval = TextInterval_create (
				iinterval == 1 ? tmin : autosegment -> xmin,
				iinterval == tier -> size ? tmax : autosegment -> xmax,
				autosegment -> name);
			Collection_addItem (intervalTier -> intervals, textInterval.transfer());
		}
	}
	return thee.transfer();
}

TextGrid Label_to_TextGrid (Label me, double duration) {
	try {
		double tmin = 0.0, tmax = duration;
		if (duration == 0.0) Label_suggestDomain (me, & tmin, & tmax);
		trace (duration, U" ", tmin, U" ", tmax);
		return _Label_to_TextGrid (me, tmin, tmax);
	} catch (MelderError) {
		Melder_throw (me, U": not converted to TextGrid.");
	}
}

TextGrid Label_Function_to_TextGrid (Label me, Any function) {
	try {
		return _Label_to_TextGrid (me, ((Function) function) -> xmin, ((Function) function) -> xmax);
	} catch (MelderError) {
		Melder_throw (me, U": not converted to TextGrid.");
	}
}

TextTier PointProcess_upto_TextTier (PointProcess me, const char32 *text) {
	try {
		autoTextTier thee = TextTier_create (my xmin, my xmax);
		for (long i = 1; i <= my nt; i ++) {
			TextTier_addPoint (thee.peek(), my t [i], text);
		}
		return thee.transfer();
	} catch (MelderError) {
		Melder_throw (me, U": not converted to TextTier.");
	}
}

PointProcess TextTier_getPoints (TextTier me, const char32 *text) {
	try {
		autoPointProcess thee = PointProcess_create (my xmin, my xmax, 10);
		for (long i = 1; i <= my points -> size; i ++) {
			TextPoint point = (TextPoint) my points -> item [i];
			if (text && text [0]) {
				if (point -> mark && str32equ (text, point -> mark))
					PointProcess_addPoint (thee.peek(), point -> number);
			} else {
				if (! point -> mark || ! point -> mark [0])
					PointProcess_addPoint (thee.peek(), point -> number);
			}
		}
		return thee.transfer();
	} catch (MelderError) {
		Melder_throw (me, U": points not converted to PointProcess.");
	}
}

PointProcess IntervalTier_getStartingPoints (IntervalTier me, const char32 *text) {
	try {
		autoPointProcess thee = PointProcess_create (my xmin, my xmax, 10);
		for (long i = 1; i <= my intervals -> size; i ++) {
			TextInterval interval = (TextInterval) my intervals -> item [i];
			if (text && text [0]) {
				if (interval -> text && str32equ (text, interval -> text))
					PointProcess_addPoint (thee.peek(), interval -> xmin);
			} else {
				if (! interval -> text || ! interval -> text [0])
					PointProcess_addPoint (thee.peek(), interval -> xmin);
			}
		}
		return thee.transfer();
	} catch (MelderError) {
		Melder_throw (me, U": starting points not gotten.");
	}
}

PointProcess IntervalTier_getEndPoints (IntervalTier me, const char32 *text) {
	try {
		autoPointProcess thee = PointProcess_create (my xmin, my xmax, 10);
		for (long i = 1; i <= my intervals -> size; i ++) {
			TextInterval interval = (TextInterval) my intervals -> item [i];
			if (text && text [0]) {
				if (interval -> text && str32equ (text, interval -> text))
					PointProcess_addPoint (thee.peek(), interval -> xmax);
			} else {
				if (! interval -> text || ! interval -> text [0])
					PointProcess_addPoint (thee.peek(), interval -> xmax);
			}
		}
		return thee.transfer();
	} catch (MelderError) {
		Melder_throw (me, U": end points not gotten.");
	}
}

PointProcess IntervalTier_getCentrePoints (IntervalTier me, const char32 *text) {
	try {
		autoPointProcess thee = PointProcess_create (my xmin, my xmax, 10);
		for (long i = 1; i <= my intervals -> size; i ++) {
			TextInterval interval = (TextInterval) my intervals -> item [i];
			if (text && text [0]) {
				if (interval -> text && str32equ (text, interval -> text))
					PointProcess_addPoint (thee.peek(), 0.5 * (interval -> xmin + interval -> xmax));
			} else {
				if (! interval -> text || ! interval -> text [0])
					PointProcess_addPoint (thee.peek(), 0.5 * (interval -> xmin + interval -> xmax));
			}
		}
		return thee.transfer();
	} catch (MelderError) {
		Melder_throw (me, U": centre points not gotten.");
	}
}

PointProcess TextGrid_getStartingPoints (TextGrid me, long tierNumber, int which_Melder_STRING, const char32 *criterion) {
	try {
		IntervalTier tier = TextGrid_checkSpecifiedTierIsIntervalTier (me, tierNumber);
		autoPointProcess thee = PointProcess_create (my xmin, my xmax, 10);
		for (long iinterval = 1; iinterval <= tier -> numberOfIntervals (); iinterval ++) {
			TextInterval interval = tier -> interval (iinterval);
			if (Melder_stringMatchesCriterion (interval -> text, which_Melder_STRING, criterion)) {
				PointProcess_addPoint (thee.peek(), interval -> xmin);
			}
		}
		return thee.transfer();
	} catch (MelderError) {
		Melder_throw (me, U": starting points not converted to PointProcess.");
	}
}

PointProcess TextGrid_getEndPoints (TextGrid me, long tierNumber, int which_Melder_STRING, const char32 *criterion) {
	try {
		IntervalTier tier = TextGrid_checkSpecifiedTierIsIntervalTier (me, tierNumber);
		autoPointProcess thee = PointProcess_create (my xmin, my xmax, 10);
		for (long iinterval = 1; iinterval <= tier -> numberOfIntervals (); iinterval ++) {
			TextInterval interval = tier -> interval (iinterval);
			if (Melder_stringMatchesCriterion (interval -> text, which_Melder_STRING, criterion)) {
				PointProcess_addPoint (thee.peek(), interval -> xmax);
			}
		}
		return thee.transfer();
	} catch (MelderError) {
		Melder_throw (me, U": end points not converted to PointProcess.");
	}
}

PointProcess TextGrid_getCentrePoints (TextGrid me, long tierNumber, int which_Melder_STRING, const char32 *criterion) {
	try {
		IntervalTier tier = TextGrid_checkSpecifiedTierIsIntervalTier (me, tierNumber);
		autoPointProcess thee = PointProcess_create (my xmin, my xmax, 10);
		for (long iinterval = 1; iinterval <= tier -> numberOfIntervals (); iinterval ++) {
			TextInterval interval = tier -> interval (iinterval);
			if (Melder_stringMatchesCriterion (interval -> text, which_Melder_STRING, criterion)) {
				PointProcess_addPoint (thee.peek(), 0.5 * (interval -> xmin + interval -> xmax));
			}
		}
		return thee.transfer();
	} catch (MelderError) {
		Melder_throw (me, U": centre points not converted to PointProcess.");
	}
}

PointProcess TextGrid_getPoints (TextGrid me, long tierNumber, int which_Melder_STRING, const char32 *criterion) {
	try {
		TextTier tier = TextGrid_checkSpecifiedTierIsPointTier (me, tierNumber);
		autoPointProcess thee = PointProcess_create (my xmin, my xmax, 10);
		for (long ipoint = 1; ipoint <= tier -> numberOfPoints (); ipoint ++) {
			TextPoint point = tier -> point (ipoint);
			if (Melder_stringMatchesCriterion (point -> mark, which_Melder_STRING, criterion)) {
				PointProcess_addPoint (thee.peek(), point -> number);
			}
		}
		return thee.transfer();
	} catch (MelderError) {
		Melder_throw (me, U": points not converted to PointProcess.");
	}
}

PointProcess TextGrid_getPoints_preceded (TextGrid me, long tierNumber,
	int which_Melder_STRING, const char32 *criterion,
	int which_Melder_STRING_precededBy, const char32 *criterion_precededBy)
{
	try {
		TextTier tier = TextGrid_checkSpecifiedTierIsPointTier (me, tierNumber);
		autoPointProcess thee = PointProcess_create (my xmin, my xmax, 10);
		for (long ipoint = 1; ipoint <= tier -> numberOfPoints (); ipoint ++) {
			TextPoint point = tier -> point (ipoint);
			if (Melder_stringMatchesCriterion (point -> mark, which_Melder_STRING, criterion)) {
				TextPoint preceding = ipoint <= 1 ? NULL : (TextPoint) tier -> points -> item [ipoint - 1];
				if (Melder_stringMatchesCriterion (preceding -> mark, which_Melder_STRING_precededBy, criterion_precededBy)) {
					PointProcess_addPoint (thee.peek(), point -> number);
				}
			}
		}
		return thee.transfer();
	} catch (MelderError) {
		Melder_throw (me, U": points not converted to PointProcess.");
	}
}

PointProcess TextGrid_getPoints_followed (TextGrid me, long tierNumber,
	int which_Melder_STRING, const char32 *criterion,
	int which_Melder_STRING_followedBy, const char32 *criterion_followedBy)
{
	try {
		TextTier tier = TextGrid_checkSpecifiedTierIsPointTier (me, tierNumber);
		autoPointProcess thee = PointProcess_create (my xmin, my xmax, 10);
		for (long ipoint = 1; ipoint <= tier -> numberOfPoints (); ipoint ++) {
			TextPoint point = tier -> point (ipoint);
			if (Melder_stringMatchesCriterion (point -> mark, which_Melder_STRING, criterion)) {
				TextPoint following = ipoint >= tier -> points -> size ? NULL : (TextPoint) tier -> points -> item [ipoint + 1];
				if (Melder_stringMatchesCriterion (following -> mark, which_Melder_STRING_followedBy, criterion_followedBy)) {
					PointProcess_addPoint (thee.peek(), point -> number);
				}
			}
		}
		return thee.transfer();
	} catch (MelderError) {
		Melder_throw (me, U": points not converted to PointProcess.");
	}
}

PointProcess IntervalTier_PointProcess_startToCentre (IntervalTier tier, PointProcess point, double phase) {
	try {
		autoPointProcess thee = PointProcess_create (tier -> xmin, tier -> xmax, 10);
		for (long i = 1; i <= point -> nt; i ++) {
			double t = point -> t [i];
			long index = IntervalTier_timeToLowIndex (tier, t);
			if (index) {
				TextInterval interval = tier -> interval (index);
				if (interval -> xmin == t)
					PointProcess_addPoint (thee.peek(), (1 - phase) * interval -> xmin + phase * interval -> xmax);
			}
		}
		return thee.transfer();
	} catch (MelderError) {
		Melder_throw (tier, U" & ", point, U": starts of intervals not converted to PointProcess.");
	}
}

PointProcess IntervalTier_PointProcess_endToCentre (IntervalTier tier, PointProcess point, double phase) {
	try {
		autoPointProcess thee = PointProcess_create (tier -> xmin, tier -> xmax, 10);
		for (long i = 1; i <= point -> nt; i ++) {
			double t = point -> t [i];
			long index = IntervalTier_timeToHighIndex (tier, t);
			if (index) {
				TextInterval interval = tier -> interval (index);
				if (interval -> xmax == t)
					PointProcess_addPoint (thee.peek(), (1 - phase) * interval -> xmin + phase * interval -> xmax);
			}
		}
		return thee.transfer();
	} catch (MelderError) {
		Melder_throw (tier, U" & ", point, U": ends of intervals not converted to PointProcess.");
	}
}

TableOfReal IntervalTier_downto_TableOfReal (IntervalTier me, const char32 *label) {
	try {
		long n = 0;
		for (long i = 1; i <= my numberOfIntervals (); i ++) {
			TextInterval interval = my interval (i);
			if (label == NULL || (label [0] == U'\0' && ! interval -> text) || (interval -> text && str32equ (interval -> text, label)))
				n ++;
		}
		autoTableOfReal thee = TableOfReal_create (n, 3);
		TableOfReal_setColumnLabel (thee.peek(), 1, U"Start");
		TableOfReal_setColumnLabel (thee.peek(), 2, U"End");
		TableOfReal_setColumnLabel (thee.peek(), 3, U"Duration");
		n = 0;
		for (long i = 1; i <= my numberOfIntervals (); i ++) {
			TextInterval interval = my interval (i);
			if (label == NULL || (label [0] == U'\0' && ! interval -> text) || (interval -> text && str32equ (interval -> text, label))) {
				n ++;
				TableOfReal_setRowLabel (thee.peek(), n, interval -> text ? interval -> text : U"");
				thy data [n] [1] = interval -> xmin;
				thy data [n] [2] = interval -> xmax;
				thy data [n] [3] = interval -> xmax - interval -> xmin;
			}
		}
		return thee.transfer();
	} catch (MelderError) {
		Melder_throw (me, U": not converted to TableOfReal.");
	}
}

TableOfReal IntervalTier_downto_TableOfReal_any (IntervalTier me) {
	return IntervalTier_downto_TableOfReal (me, NULL);
}

TableOfReal TextTier_downto_TableOfReal (TextTier me, const char32 *label) {
	try {
		long n = 0;
		for (long i = 1; i <= my points -> size; i ++) {
			TextPoint point = (TextPoint) my points -> item [i];
			if (label == NULL || (label [0] == U'\0' && ! point -> mark) || (point -> mark && str32equ (point -> mark, label)))
				n ++;
		}
		autoTableOfReal thee = TableOfReal_create (n, 1);
		TableOfReal_setColumnLabel (thee.peek(), 1, U"Time");
		n = 0;
		for (long i = 1; i <= my numberOfPoints (); i ++) {
			TextPoint point = my point (i);
			if (label == NULL || (label [0] == U'\0' && ! point -> mark) || (point -> mark && str32equ (point -> mark, label))) {
				n ++;
				TableOfReal_setRowLabel (thee.peek(), n, point -> mark ? point -> mark : U"");
				thy data [n] [1] = point -> number;
			}
		}
		return thee.transfer();
	} catch (MelderError) {
		Melder_throw (me, U": not converted to TableOfReal.");
	}
}

TableOfReal TextTier_downto_TableOfReal_any (TextTier me) {
	return TextTier_downto_TableOfReal (me, NULL);
}

IntervalTier IntervalTier_readFromXwaves (MelderFile file) {
	try {
		char *line;
		double lastTime = 0.0;

		autoIntervalTier me = IntervalTier_create (0, 100);
		autoMelderFile mfile = MelderFile_open (file);

		/*
		 * Search for a line that starts with '#'.
		 */
		for (;;) {
			line = MelderFile_readLine (file);
			if (line == NULL)
				Melder_throw (U"Missing '#' line.");
			if (line [0] == '#') break;
		}

		/*
		 * Read a mark from every line.
		 */
		for (;;) {
			double time;
			long colour, numberOfElements;
			char mark [300];

			line = MelderFile_readLine (file);
			if (line == NULL) break;   // normal end-of-file
			numberOfElements = sscanf (line, "%lf%ld%s", & time, & colour, mark);
			if (numberOfElements == 0) {
				break;   // an empty line, hopefully at the end
			}
			if (numberOfElements == 1)
				Melder_throw (U"Line too short: \"", Melder_peek8to32 (line), U"\".");
			if (numberOfElements == 2)
				mark [0] = '\0';
			if (lastTime == 0.0) {
				TextInterval interval = (TextInterval) my intervals -> item [1];
				interval -> xmax = time;
				TextInterval_setText (interval, Melder_peek8to32 (mark));
			} else {
				IntervalTier_addInterval_unsafe (me.peek(), lastTime, time, Melder_peek8to32 (mark));
			}
			lastTime = time;
		}

		/*
		 * Fix domain.
		 */
		if (lastTime > 0.0) {
			TextInterval lastInterval = (TextInterval) my intervals -> item [my intervals -> size];
			my xmax = lastInterval -> xmax = lastTime;
		}

		mfile.close ();
		return me.transfer();
	} catch (MelderError) {
		Melder_throw (U"IntervalTier not read from file ", file, U".");
	}
}

void IntervalTier_writeToXwaves (IntervalTier me, MelderFile file) {
	try {
		autofile f = Melder_fopen (file, "w");
		fprintf (f, "separator ;\nnfields 1\n#\n");
		for (long iinterval = 1; iinterval <= my intervals -> size; iinterval ++) {
			TextInterval interval = (TextInterval) my intervals -> item [iinterval];
			fprintf (f, "\t%.6f 26\t%s\n", interval -> xmax, Melder_peek32to8 (interval -> text));
		}
		f.close (file);
	} catch (MelderError) {
		Melder_throw (me, U": not written to Xwaves file ", file, U".");
	}
}

TextGrid PointProcess_to_TextGrid_vuv (PointProcess me, double maxT, double meanT) {
	try {
		autoTextGrid thee = TextGrid_create (my xmin, my xmax, U"vuv", NULL);
		Collection_removeItem (((IntervalTier) thy tiers -> item [1]) -> intervals, 1);
		long ipointright;
		double beginVoiceless = my xmin, endVoiceless, halfMeanT = 0.5 * meanT;
		for (long ipointleft = 1; ipointleft <= my nt; ipointleft = ipointright + 1) {
			endVoiceless = my t [ipointleft] - halfMeanT;
			if (endVoiceless <= beginVoiceless) {
				endVoiceless = beginVoiceless;   // we will use for voiced interval
			} else {
				IntervalTier_addInterval_unsafe ((IntervalTier) thy tiers -> item [1], beginVoiceless, endVoiceless, U"U");
			}
			for (ipointright = ipointleft + 1; ipointright <= my nt; ipointright ++)
				if (my t [ipointright] - my t [ipointright - 1] > maxT)
					break;
			ipointright --;
			beginVoiceless = my t [ipointright] + halfMeanT;
			if (beginVoiceless > my xmax)
				beginVoiceless = my xmax;
			IntervalTier_addInterval_unsafe ((IntervalTier) thy tiers -> item [1], endVoiceless, beginVoiceless, U"V");
		}
		endVoiceless = my xmax;
		if (endVoiceless > beginVoiceless) {
			IntervalTier_addInterval_unsafe ((IntervalTier) thy tiers -> item [1], beginVoiceless, endVoiceless, U"U");
		}
		return thee.transfer();
	} catch (MelderError) {
		Melder_throw (me, U": not converted to TextGrid (vuv).");
	}
}

long TextInterval_labelLength (TextInterval me) {
	return my text ? str32len (my text) : 0;
}

long TextPoint_labelLength (TextPoint me) {
	return my mark ? str32len (my mark) : 0;
}

long IntervalTier_maximumLabelLength (IntervalTier me) {
	long maximum = 0, ninterval = my intervals -> size;
	for (long iinterval = 1; iinterval <= ninterval; iinterval ++) {
		long length = TextInterval_labelLength ((TextInterval) my intervals -> item [iinterval]);
		if (length > maximum) maximum = length;
	}
	return maximum;
}

long TextTier_maximumLabelLength (TextTier me) {
	long maximum = 0, npoint = my points -> size;
	for (long ipoint= 1; ipoint <= npoint; ipoint ++) {
		long length = TextPoint_labelLength ((TextPoint) my points -> item [ipoint]);
		if (length > maximum) maximum = length;
	}
	return maximum;
}

long TextGrid_maximumLabelLength (TextGrid me) {
	long maximum = 0, ntier = my tiers -> size;
	for (long itier = 1; itier <= ntier; itier ++) {
		Function anyTier = (Function) my tiers -> item [itier];
		long length = anyTier -> classInfo == classIntervalTier ?
			IntervalTier_maximumLabelLength ((IntervalTier) anyTier) :
			TextTier_maximumLabelLength ((TextTier) anyTier);
		if (length > maximum) maximum = length;
	}
	return maximum;
}

static void genericize (char32 **pstring, char32 *buffer) {
	if (*pstring) {
		const char32 *p = (const char32 *) *pstring;
		while (*p) {
			if (*p > 126) {   // only if necessary
				char32 *newString;
				Longchar_genericize32 (*pstring, buffer);
				newString = Melder_dup (buffer);
				/*
				 * Replace string only if copying was OK.
				 */
				Melder_free (*pstring);
				*pstring = newString;
				break;
			}
			p ++;
		}
	}
}

void TextGrid_genericize (TextGrid me) {
	try {
		autostring32 buffer = Melder_calloc (char32, TextGrid_maximumLabelLength (me) * 3 + 1);
		for (long itier = 1; itier <= my numberOfTiers (); itier ++) {
			Function anyTier = my tier (itier);
			if (anyTier -> classInfo == classIntervalTier) {
				IntervalTier tier = static_cast <IntervalTier> (anyTier);
				long n = tier -> numberOfIntervals ();
				for (long i = 1; i <= n; i ++) {
					TextInterval interval = tier -> interval (i);
					genericize (& interval -> text, buffer.peek());
				}
			} else {
				TextTier tier = (TextTier) anyTier;
				long n = tier -> points -> size;
				for (long i = 1; i <= n; i ++) {
					TextPoint point = tier -> point (i);
					genericize (& point -> mark, buffer.peek());
				}
			}
		}
	} catch (MelderError) {
		Melder_throw (me, U": not converted to backslash trigraphs.");
	}
}

void TextGrid_nativize (TextGrid me) {
	try {
		autostring32 buffer = Melder_calloc (char32, TextGrid_maximumLabelLength (me) + 1);
		for (long itier = 1; itier <= my numberOfTiers (); itier ++) {
			Function anyTier = my tier (itier);
			if (anyTier -> classInfo == classIntervalTier) {
				IntervalTier tier = static_cast <IntervalTier> (anyTier);
				long n = tier -> numberOfIntervals ();
				for (long i = 1; i <= n; i ++) {
					TextInterval interval = tier -> interval (i);
					if (interval -> text) {
						Longchar_nativize32 (interval -> text, buffer.peek(), FALSE);
						str32cpy (interval -> text, buffer.peek());
					}
				}
			} else {
				TextTier tier = static_cast <TextTier> (anyTier);
				long n = tier -> numberOfPoints ();
				for (long i = 1; i <= n; i ++) {
					TextPoint point = tier -> point (i);
					if (point -> mark) {
						Longchar_nativize32 (point -> mark, buffer.peek(), FALSE);
						str32cpy (point -> mark, buffer.peek());
					}
				}
			}
		}
	} catch (MelderError) {
		Melder_throw (me, U": backslash trigraphs not converted to Unicode.");
	}
}

void TextInterval_removeText (TextInterval me) {
	Melder_free (my text);
}

void TextPoint_removeText (TextPoint me) {
	Melder_free (my mark);
}

void IntervalTier_removeText (IntervalTier me) {
	long ninterval = my numberOfIntervals ();
	for (long iinterval = 1; iinterval <= ninterval; iinterval ++)
		TextInterval_removeText (my interval (iinterval));
}

void TextTier_removeText (TextTier me) {
	long npoint = my numberOfPoints ();
	for (long ipoint = 1; ipoint <= npoint; ipoint ++)
		TextPoint_removeText (my point (ipoint));
}

void TextGrid_insertBoundary (TextGrid me, int tierNumber, double t) {
	try {
		Function anyTier = TextGrid_checkSpecifiedTierNumberWithinRange (me, tierNumber);
		if (anyTier -> classInfo != classIntervalTier)
			Melder_throw (U"Cannot add a boundary on tier ", tierNumber, U", because that tier is not an interval tier.");
		IntervalTier intervalTier = static_cast <IntervalTier> (anyTier);
		if (IntervalTier_hasTime (intervalTier, t))
			Melder_throw (U"Cannot add a boundary at ", Melder_fixed (t, 6), U" seconds, because there is already a boundary there.");
		long intervalNumber = IntervalTier_timeToIndex (intervalTier, t);
		if (intervalNumber == 0)
			Melder_throw (U"Cannot add a boundary at ", Melder_fixed (t, 6), U" seconds, because this is outside the time domain of the intervals.");
		TextInterval interval = intervalTier -> interval (intervalNumber);
		/*
		 * Move the text to the left of the boundary.
		 */
		autoTextInterval newInterval = TextInterval_create (t, interval -> xmax, U"");
		interval -> xmax = t;
		Collection_addItem (intervalTier -> intervals, newInterval.transfer());
	} catch (MelderError) {
		Melder_throw (me, U": boundary not inserted.");
	}
}

void IntervalTier_removeLeftBoundary (IntervalTier me, long intervalNumber) {
	try {
		Melder_assert (intervalNumber > 1);
		Melder_assert (intervalNumber <= my numberOfIntervals ());
		TextInterval left = my interval (intervalNumber - 1);
		TextInterval right = my interval (intervalNumber);
		/*
		 * Move the text to the left of the boundary.
		 */
		left -> xmax = right -> xmax;   // collapse left and right intervals into left interval
		if (right -> text == NULL) {
			;
		} else if (left -> text == NULL) {
			TextInterval_setText (left, right -> text);
		} else {
			TextInterval_setText (left, Melder_cat (left -> text, right -> text));
		}
		Collection_removeItem (my intervals, intervalNumber);   // remove right interval
	} catch (MelderError) {
		Melder_throw (me, U": left boundary not removed.");
	}
}

void TextGrid_removeBoundaryAtTime (TextGrid me, int tierNumber, double t) {
	try {
		IntervalTier intervalTier = TextGrid_checkSpecifiedTierIsIntervalTier (me, tierNumber);
		if (! IntervalTier_hasTime (intervalTier, t))
			Melder_throw (U"There is no boundary at ", t, U" seconds.");
		long intervalNumber = IntervalTier_timeToIndex (intervalTier, t);
		if (intervalNumber == 0)
			Melder_throw (U"The time of ", t, U" seconds is outside the time domain of the intervals.");
		if (intervalNumber == 1)
			Melder_throw (U"The time of ", t, U" seconds is at the left edge of the tier.");
		IntervalTier_removeLeftBoundary (intervalTier, intervalNumber);
	} catch (MelderError) {
		Melder_throw (me, U": boundary not removed.");
	}
}

void TextGrid_setIntervalText (TextGrid me, int tierNumber, long intervalNumber, const char32 *text) {
	try {
		IntervalTier intervalTier = TextGrid_checkSpecifiedTierIsIntervalTier (me, tierNumber);
		if (intervalNumber < 1 || intervalNumber > intervalTier -> numberOfIntervals ())
			Melder_throw (U"Interval ", intervalNumber, U" does not exist on tier ", tierNumber, U".");
		TextInterval interval = intervalTier -> interval (intervalNumber);
		TextInterval_setText (interval, text);
	} catch (MelderError) {
		Melder_throw (me, U": interval text not set.");
	}
}

void TextGrid_insertPoint (TextGrid me, int tierNumber, double t, const char32 *mark) {
	try {
		TextTier textTier = TextGrid_checkSpecifiedTierIsPointTier (me, tierNumber);
		if (AnyTier_hasPoint (textTier, t))
			Melder_throw (U"There is already a point at ", t, U" seconds.");
		autoTextPoint newPoint = TextPoint_create (t, mark);
		Collection_addItem (textTier -> points, newPoint.transfer());
	} catch (MelderError) {
		Melder_throw (me, U": point not inserted.");
	}
}

void TextTier_removePoint (TextTier me, long ipoint) {
	Melder_assert (ipoint <= my points -> size);
	Collection_removeItem (my points, ipoint);
}

void TextTier_removePoints (TextTier me, int which_Melder_STRING, const char32 *criterion) {
	for (long i = my numberOfPoints (); i > 0; i --)
		if (Melder_stringMatchesCriterion (my point (i) -> mark, which_Melder_STRING, criterion))
			Collection_removeItem (my points, i);
}

void TextGrid_removePoints (TextGrid me, long tierNumber, int which_Melder_STRING, const char32 *criterion) {
	try {
		TextTier tier = TextGrid_checkSpecifiedTierIsPointTier (me, tierNumber);
		TextTier_removePoints (tier, which_Melder_STRING, criterion);
	} catch (MelderError) {
		Melder_throw (me, U": points not removed.");
	}
}

void TextGrid_setPointText (TextGrid me, int tierNumber, long pointNumber, const char32 *text) {
	try {
		TextTier textTier = TextGrid_checkSpecifiedTierIsPointTier (me, tierNumber);
		if (pointNumber < 1 || pointNumber > textTier -> numberOfPoints ())
			Melder_throw (U"Point ", pointNumber, U" does not exist on tier ", tierNumber, U".");
		TextPoint point = textTier -> point (pointNumber);
		TextPoint_setText (point, text);
	} catch (MelderError) {
		Melder_throw (me, U": point text not set.");
	}
}

static void sgmlToPraat (char *text) {
	char *sgml = text, *praat = text;
	for (;;) {
		if (*sgml == '\0') break;
		if (*sgml == '&') {
			static struct { const char *sgml, *praat; } translations [] = {
				{ "auml", "\\a\"" }, { "euml", "\\e\"" }, { "iuml", "\\i\"" },
				{ "ouml", "\\o\"" }, { "ouml", "\\o\"" },
				{ "Auml", "\\A\"" }, { "Euml", "\\E\"" }, { "Iuml", "\\I\"" },
				{ "Ouml", "\\O\"" }, { "Uuml", "\\U\"" },
				{ "aacute", "\\a'" }, { "eacute", "\\e'" }, { "iacute", "\\i'" },
				{ "oacute", "\\o'" }, { "oacute", "\\o'" },
				{ "Aacute", "\\A'" }, { "Eacute", "\\E'" }, { "Iacute", "\\I'" },
				{ "Oacute", "\\O'" }, { "Uacute", "\\U'" },
				{ "agrave", "\\a`" }, { "egrave", "\\e`" }, { "igrave", "\\i`" },
				{ "ograve", "\\o`" }, { "ograve", "\\o`" },
				{ "Agrave", "\\A`" }, { "Egrave", "\\E`" }, { "Igrave", "\\I`" },
				{ "Ograve", "\\O`" }, { "Ugrave", "\\U`" },
				{ "acirc", "\\a^" }, { "ecirc", "\\e^" }, { "icirc", "\\i^" },
				{ "ocirc", "\\o^" }, { "ocirc", "\\o^" },
				{ "Acirc", "\\A^" }, { "Ecirc", "\\E^" }, { "Icirc", "\\I^" },
				{ "Ocirc", "\\O^" }, { "Ucirc", "\\U^" },
				{ NULL, NULL } };
			char sgmlCode [201];
			int i = 0;
			++ sgml;
			for (i = 0; i < 200; i ++) {
				char sgmlChar = sgml [i];
				if (sgmlChar == ';') {
					if (i == 0) Melder_throw (U"Empty SGML code.");
					sgml += i + 1;
					break;
				}
				sgmlCode [i] = sgmlChar;
			}
			if (i >= 200) Melder_throw (U"Unfinished SGML code.");
			sgmlCode [i] = '\0';
			for (i = 0; translations [i]. sgml != NULL; i ++) {
				if (strequ (sgmlCode, translations [i]. sgml)) {
					memcpy (praat, translations [i]. praat, strlen (translations [i]. praat));
					praat += strlen (translations [i]. praat);
					break;
				}
			}
			if (translations [i]. sgml == NULL) Melder_throw (U"Unknown SGML code &", Melder_peek8to32 (sgmlCode), U";.");
		} else {
			* praat ++ = * sgml ++;
		}
	}
	*praat = '\0';
}

TextGrid TextGrid_readFromChronologicalTextFile (MelderFile file) {
	try {
		Thing_version = 0;
		autoMelderReadText text = MelderReadText_createFromFile (file);
		autostring32 tag = texgetw2 (text.peek());
		if (! str32equ (tag.peek(), U"Praat chronological TextGrid text file"))
			Melder_throw (U"This is not a chronological TextGrid text file.");
		autoTextGrid me = Thing_new (TextGrid);
		my structFunction :: v_readText (text.peek());
		my tiers = Ordered_create ();
		long numberOfTiers = texgeti4 (text.peek());
		for (long itier = 1; itier <= numberOfTiers; itier ++) {
			autostring32 klas = texgetw2 (text.peek());
			if (str32equ (klas.peek(), U"IntervalTier")) {
				autoIntervalTier tier = Thing_new (IntervalTier);
				tier -> name = texgetw2 (text.peek());
				tier -> structFunction :: v_readText (text.peek());
				tier -> intervals = SortedSetOfDouble_create ();
				Collection_addItem (my tiers, tier.transfer());
			} else if (str32equ (klas.peek(), U"TextTier")) {
				autoTextTier tier = Thing_new (TextTier);
				tier -> name = texgetw2 (text.peek());
				tier -> structFunction :: v_readText (text.peek());
				tier -> points = SortedSetOfDouble_create ();
				Collection_addItem (my tiers, tier.transfer());
			} else {
				Melder_throw (U"Unknown tier class \"", klas.peek(), U"\".");
			}
		}
		for (;;) {
			long tierNumber;
			try {
				tierNumber = texgeti4 (text.peek());
			} catch (MelderError) {
				if (str32str (Melder_getError (), U"Early end of text")) {
					Melder_clearError ();
					break;
				} else {
					throw;
				}
			}
			Function anyTier = TextGrid_checkSpecifiedTierNumberWithinRange (me.peek(), tierNumber);
			if (anyTier -> classInfo == classIntervalTier) {
				IntervalTier tier = static_cast <IntervalTier> (anyTier);
				autoTextInterval interval = Thing_new (TextInterval);
				interval -> v_readText (text.peek());
				Collection_addItem (tier -> intervals, interval.transfer());   // not earlier: sorting depends on contents of interval
			} else {
				TextTier tier = static_cast <TextTier> (anyTier);
				autoTextPoint point = Thing_new (TextPoint);
				point -> v_readText (text.peek());
				Collection_addItem (tier -> points, point.transfer());   // not earlier: sorting depends on contents of point
			}
		}
		return me.transfer();
	} catch (MelderError) {
		Melder_throw (U"TextGrid not read from chronological text file ", file, U".");
	}
}

static void writeQuotedString (MelderFile file, const char32 *string) {
	MelderFile_writeCharacter (file, U'\"');
	if (string) {
		char32 kar;
		while ((kar = *string ++) != U'\0') {
			MelderFile_writeCharacter (file, kar);
			if (kar == '\"') MelderFile_writeCharacter (file, kar);
		}
	}   // BUG
	MelderFile_writeCharacter (file, U'\"');
}

void TextGrid_writeToChronologicalTextFile (TextGrid me, MelderFile file) {
	try {
		Data_createTextFile (me, file, false);
		autoMelderFile mfile = file;
		/*
		 * The "elements" (intervals and points) are sorted primarily by time and secondarily by tier.
		 */
		double sortingTime = -1e308;
		long sortingTier = 0;
		file -> verbose = false;
		texindent (file);
		MelderFile_write (file, U"\"Praat chronological TextGrid text file\"\n", my xmin, U" ", my xmax,
			U"   ! Time domain.\n", my tiers -> size, U"   ! Number of tiers.");
		for (long itier = 1; itier <= my tiers -> size; itier ++) {
			Function anyTier = (Function) my tiers -> item [itier];
			MelderFile_write (file, U"\n");
			writeQuotedString (file, Thing_className (anyTier));
			MelderFile_write (file, U" ");
			writeQuotedString (file, anyTier -> name);
			MelderFile_write (file, U" ", anyTier -> xmin, U" ", anyTier -> xmax);
		}
		for (;;) {
			double firstRemainingTime = +1e308;
			long firstRemainingTier = 2000000000, firstRemainingElement = 0;
			for (long itier = 1; itier <= my tiers -> size; itier ++) {
				Function anyTier = (Function) my tiers -> item [itier];
				if (anyTier -> classInfo == classIntervalTier) {
					IntervalTier tier = (IntervalTier) anyTier;
					for (long ielement = 1; ielement <= tier -> intervals -> size; ielement ++) {
						TextInterval interval = (TextInterval) tier -> intervals -> item [ielement];
						if ((interval -> xmin > sortingTime ||   // sort primarily by time
							 (interval -> xmin == sortingTime && itier > sortingTier)) &&   // sort secondarily by tier number
							(interval -> xmin < firstRemainingTime ||   // sort primarily by time
							 (interval -> xmin == firstRemainingTime && itier < firstRemainingTier)))   // sort secondarily by tier number
						{
							firstRemainingTime = interval -> xmin;
							firstRemainingTier = itier;
							firstRemainingElement = ielement;
						}
					}
				} else {
					TextTier tier = (TextTier) anyTier;
					for (long ielement = 1; ielement <= tier -> points -> size; ielement ++) {
						TextPoint point = (TextPoint) tier -> points -> item [ielement];
						if ((point -> number > sortingTime ||   // sort primarily by time
							 (point -> number == sortingTime && itier > sortingTier)) &&   // sort secondarily by tier number
							(point -> number < firstRemainingTime ||   // sort primarily by time
							 (point -> number == firstRemainingTime && itier < firstRemainingTier)))   // sort secondarily by tier number
						{
							firstRemainingTime = point -> number;
							firstRemainingTier = itier;
							firstRemainingElement = ielement;
						}
					}
				}
			}
			if (firstRemainingElement == 0) {
				break;
			} else {
				Function anyTier = (Function) my tiers -> item [firstRemainingTier];
				if (anyTier -> classInfo == classIntervalTier) {
					IntervalTier tier = (IntervalTier) anyTier;
					TextInterval interval = (TextInterval) tier -> intervals -> item [firstRemainingElement];
					if (tier -> name) MelderFile_write (file, U"\n\n! ", tier -> name, U":");
					MelderFile_write (file, U"\n", firstRemainingTier, U" ", interval -> xmin, U" ", interval -> xmax);
					texputw4 (file, interval -> text, U"", 0,0,0,0,0);
				} else {
					TextTier tier = (TextTier) anyTier;
					TextPoint point = (TextPoint) tier -> points -> item [firstRemainingElement];
					if (tier -> name) MelderFile_write (file, U"\n\n! ", tier -> name, U":");
					MelderFile_write (file, U"\n", firstRemainingTier, U" ", point -> number, U" ");
					texputw4 (file, point -> mark, U"", 0,0,0,0,0);
				}
				sortingTime = firstRemainingTime;
				sortingTier = firstRemainingTier;
			}
		}
		texexdent (file);
		mfile.close ();
	} catch (MelderError) {
		Melder_throw (me, U": not written to chronological text file ", file, U".");
	}
}

TextGrid TextGrid_readFromCgnSyntaxFile (MelderFile file) {
	try {
		autoTextGrid me = Thing_new (TextGrid);
		long sentenceNumber = 0;
		double phraseBegin = 0.0, phraseEnd = 0.0;
		IntervalTier sentenceTier = NULL, phraseTier = NULL;
		TextInterval lastInterval = NULL;
		static char phrase [1000];
		my tiers = Ordered_create ();
		autoMelderFile mfile = MelderFile_open (file);
		char *line = MelderFile_readLine (file);
		if (! strequ (line, "<?xml version=\"1.0\"?>"))
			Melder_throw (U"This is not a CGN syntax file.");
		line = MelderFile_readLine (file);
		if (! strequ (line, "<!DOCTYPE ttext SYSTEM \"ttext.dtd\">"))
			Melder_throw (U"This is not a CGN syntax file.");
		line = MelderFile_readLine (file);
		long startOfData = MelderFile_tell (file);
		/*
		 * Get duration.
		 */
		my xmin = 0.0;
		char arg1 [41], arg2 [41], arg3 [41], arg4 [41], arg5 [41], arg6 [41], arg7 [201];
		for (;;) {
			line = MelderFile_readLine (file);
			if (! line) break;
			if (strnequ (line, "  <tau ref=\"", 12)) {
				if (sscanf (line, "%40s%40s%40s%40s%40s%40s%200s", arg1, arg2, arg3, arg4, arg5, arg6, arg7) < 7)
					Melder_throw (U"Too few strings in tau line.");
				my xmax = atof (arg5 + 4);
			}
		}
		if (my xmax <= 0.0) Melder_throw (U"Duration (", my xmax, U" seconds) should be greater than zero.");
		/*
		 * Get number and names of tiers.
		 */
		MelderFile_seek (file, startOfData, SEEK_SET);
		for (;;) {
			line = MelderFile_readLine (file);
			if (! line) break;
			if (strnequ (line, "  <tau ref=\"", 12)) {
				char *speakerName;
				int length, speakerTier = 0;
				double tb, te;
				if (sscanf (line, "%40s%40s%40s%40s%40s%40s%200s", arg1, arg2, arg3, arg4, arg5, arg6, arg7) < 7)
					Melder_throw (U"Too few strings in tau line.");
				length = strlen (arg3);
				if (length < 5 || ! strnequ (arg3, "s=\"", 3))
					Melder_throw (U"Missing speaker name.");
				arg3 [length - 1] = '\0';   // truncate at double quote
				speakerName = arg3 + 3;   // truncate leading s="
				/*
				 * Does this speaker name occur in the tiers?
				 */
				for (long itier = 1; itier <= my tiers -> size; itier ++) {
					IntervalTier tier = (IntervalTier) my tiers -> item [itier];
					if (str32equ (tier -> name, Melder_peek8to32 (speakerName))) {
						speakerTier = itier;
						break;
					}
				}
				if (speakerTier == 0) {
					/*
					 * Create two new tiers.
					 */
					autoIntervalTier newSentenceTier = Thing_new (IntervalTier);
					newSentenceTier -> intervals = SortedSetOfDouble_create ();
					newSentenceTier -> xmin = 0.0;
					newSentenceTier -> xmax = my xmax;
					Thing_setName (newSentenceTier.peek(), Melder_peek8to32 (speakerName));
					sentenceTier = newSentenceTier.peek();   // for later use; this seems safe
					Collection_addItem (my tiers, newSentenceTier.transfer());
					autoIntervalTier newPhraseTier = Thing_new (IntervalTier);
					newPhraseTier -> intervals = SortedSetOfDouble_create ();
					newPhraseTier -> xmin = 0.0;
					newPhraseTier -> xmax = my xmax;
					Thing_setName (newPhraseTier.peek(), Melder_peek8to32 (speakerName));
					phraseTier = newPhraseTier.peek();
					Collection_addItem (my tiers, newPhraseTier.transfer());
				} else {
					sentenceTier = (IntervalTier) my tiers -> item [speakerTier];
					phraseTier = (IntervalTier) my tiers -> item [speakerTier + 1];
				}
				tb = atof (arg4 + 4), te = atof (arg5 + 4);
				if (te <= tb)
					Melder_throw (U"Zero duration for sentence.");
				/*
				 * We are going to add one or two intervals to the sentence tier.
				 */
				if (sentenceTier -> intervals -> size > 0) {
					TextInterval latestInterval = (TextInterval) sentenceTier -> intervals -> item [sentenceTier -> intervals -> size];
					if (tb > latestInterval -> xmax) {
						autoTextInterval interval = TextInterval_create (latestInterval -> xmax, tb, U"");
						Collection_addItem (sentenceTier -> intervals, interval.transfer());
					} else if (tb < latestInterval -> xmax) {
						Melder_throw (U"Overlap on tier not allowed.");
					}
				} else {
					if (tb > 0.0) {
						TextInterval interval = TextInterval_create (0.0, tb, U"");
						Collection_addItem (sentenceTier -> intervals, interval);
					} else if (tb < 0.0) {
						Melder_throw (U"Negative times not allowed.");
					}
				}
				autoTextInterval interval = TextInterval_create (tb, te, Melder_integer (++ sentenceNumber));
				Collection_addItem (sentenceTier -> intervals, interval.transfer());
			} else if (strnequ (line, "    <tw ref=\"", 13)) {
				int length;
				double tb, te;
				if (sscanf (line, "%40s%40s%40s%40s%40s%40s%200s", arg1, arg2, arg3, arg4, arg5, arg6, arg7) < 7)
					Melder_throw (U"Too few strings in tw line.");
				length = strlen (arg3);
				if (length < 6 || ! strnequ (arg3, "tb=\"", 4))
					Melder_throw (U"Missing tb.");
				tb = atof (arg3 + 4);
				length = strlen (arg4);
				if (length < 6 || ! strnequ (arg4, "te=\"", 4))
					Melder_throw (U"Missing te.");
				te = atof (arg4 + 4);
				if (te <= tb)
					Melder_throw (U"Zero duration for phrase.");
				if (tb == phraseBegin && te == phraseEnd) {
					/* Append a word. */
					strcat (phrase, " ");
					length = strlen (arg7);
					if (length < 6 || ! strnequ (arg7, "w=\"", 3))
						Melder_throw (U"Missing word.");
					arg7 [length - 3] = '\0';   // truncate "/>
					strcat (phrase, arg7 + 3);
				} else {
					/* Begin a phrase. */
					if (lastInterval) {
						sgmlToPraat (phrase);
						TextInterval_setText (lastInterval, Melder_peek8to32 (phrase));
					}
					phrase [0] = '\0';
					length = strlen (arg7);
					if (length < 6 || ! strnequ (arg7, "w=\"", 3))
						Melder_throw (U"Missing word.");
					arg7 [length - 3] = '\0';   // truncate "/>
					strcat (phrase, arg7 + 3);
					if (phraseTier -> intervals -> size > 0) {
						TextInterval latestInterval = (TextInterval) phraseTier -> intervals -> item [phraseTier -> intervals -> size];
						if (tb > latestInterval -> xmax) {
							autoTextInterval interval = TextInterval_create (latestInterval -> xmax, tb, U"");
							Collection_addItem (phraseTier -> intervals, interval.transfer());
						} else if (tb < latestInterval -> xmax) {
							Melder_throw (U"Overlap on tier not allowed.");
						}
					} else {
						if (tb > 0.0) {
							autoTextInterval interval = TextInterval_create (0.0, tb, U"");
							Collection_addItem (phraseTier -> intervals, interval.transfer());
						} else if (tb < 0.0) {
							Melder_throw (U"Negative times not allowed.");
						}
					}
					if (! phraseTier)
						Melder_throw (U"Phrase outside sentence.");
					autoTextInterval newLastInterval = TextInterval_create (tb, te, U"");
					lastInterval = newLastInterval.peek();
					Collection_addItem (phraseTier -> intervals, newLastInterval.transfer());
					phraseBegin = tb;
					phraseEnd = te;
				}
			}
		}
		if (lastInterval) {
			sgmlToPraat (phrase);
			TextInterval_setText (lastInterval, Melder_peek8to32 (phrase));
		}
		for (long itier = 1; itier <= my tiers -> size; itier ++) {
			IntervalTier tier = (IntervalTier) my tiers -> item [itier];
			if (tier -> intervals -> size > 0) {
				TextInterval latestInterval = (TextInterval) tier -> intervals -> item [tier -> intervals -> size];
				if (my xmax > latestInterval -> xmax) {
					autoTextInterval interval = TextInterval_create (latestInterval -> xmax, my xmax, U"");
					Collection_addItem (tier -> intervals, interval.transfer());
				}
			} else {
				autoTextInterval interval = TextInterval_create (my xmin, my xmax, U"");
				Collection_addItem (tier -> intervals, interval.transfer());
			}
		}
		mfile.close ();
		return me.transfer();
	} catch (MelderError) {
		Melder_throw (U"TextGrid not read from CGN syntax file ", file, U".");
	}
}

Table TextGrid_downto_Table (TextGrid me, bool includeLineNumbers, int timeDecimals, bool includeTierNames, bool includeEmptyIntervals) {
	long numberOfRows = 0;
	for (long itier = 1; itier <= my tiers -> size; itier ++) {
		Function anyTier = (Function) my tiers -> item [itier];
		if (anyTier -> classInfo == classIntervalTier) {
			IntervalTier tier = (IntervalTier) anyTier;
			if (includeEmptyIntervals) {
				numberOfRows += tier -> intervals -> size;
			} else {
				for (long iinterval = 1; iinterval <= tier -> intervals -> size; iinterval ++) {
					TextInterval interval = (TextInterval) tier -> intervals -> item [iinterval];
					if (interval -> text != NULL && interval -> text [0] != U'\0') {
						numberOfRows ++;
					}
				}
			}
		} else {
			TextTier tier = (TextTier) anyTier;
			numberOfRows += tier -> points -> size;
		}
	}
	autoTable thee = Table_createWithoutColumnNames (numberOfRows, 3 + includeLineNumbers + includeTierNames);
	long icol = 0;
	if (includeLineNumbers)
		Table_setColumnLabel (thee.peek(), ++ icol, U"line");
	Table_setColumnLabel (thee.peek(), ++ icol, U"tmin");
	if (includeTierNames)
		Table_setColumnLabel (thee.peek(), ++ icol, U"tier");
	Table_setColumnLabel (thee.peek(), ++ icol, U"text");
	Table_setColumnLabel (thee.peek(), ++ icol, U"tmax");
	long irow = 0;
	for (long itier = 1; itier <= my tiers -> size; itier ++) {
		Function anyTier = (Function) my tiers -> item [itier];
		if (anyTier -> classInfo == classIntervalTier) {
			IntervalTier tier = (IntervalTier) anyTier;
			for (long iinterval = 1; iinterval <= tier -> intervals -> size; iinterval ++) {
				TextInterval interval = (TextInterval) tier -> intervals -> item [iinterval];
				if (includeEmptyIntervals || (interval -> text != NULL && interval -> text [0] != U'\0')) {
					++ irow;
					icol = 0;
					if (includeLineNumbers)
						Table_setNumericValue (thee.peek(), irow, ++ icol, irow);
					Table_setStringValue (thee.peek(), irow, ++ icol, Melder_fixed (interval -> xmin, timeDecimals));
					if (includeTierNames)
						Table_setStringValue (thee.peek(), irow, ++ icol, tier -> name);
					Table_setStringValue (thee.peek(), irow, ++ icol, interval -> text);
					Table_setStringValue (thee.peek(), irow, ++ icol, Melder_fixed (interval -> xmax, timeDecimals));
				}
			}
		} else {
			TextTier tier = (TextTier) anyTier;
			for (long ipoint = 1; ipoint <= tier -> points -> size; ipoint ++) {
				TextPoint point = (TextPoint) tier -> points -> item [ipoint];
				++ irow;
				icol = 0;
				if (includeLineNumbers)
					Table_setNumericValue (thee.peek(), irow, ++ icol, irow);
				Table_setStringValue (thee.peek(), irow, ++ icol, Melder_fixed (point -> number, timeDecimals));
				if (includeTierNames)
					Table_setStringValue (thee.peek(), irow, ++ icol, tier -> name);
				Table_setStringValue (thee.peek(), irow, ++ icol, point -> mark);
				Table_setStringValue (thee.peek(), irow, ++ icol, Melder_fixed (point -> number, timeDecimals));
			}
		}
	}
	long columns [1+2] = { 0, 1 + includeLineNumbers, 3 + includeLineNumbers + includeTierNames };   // sort by tmin and tmax
	Table_sortRows_Assert (thee.peek(), columns, 2);
	return thee.transfer();
}

void TextGrid_list (TextGrid me, bool includeLineNumbers, int timeDecimals, bool includeTierNames, bool includeEmptyIntervals) {
	try {
		autoTable table = TextGrid_downto_Table (me, includeLineNumbers, timeDecimals, includeTierNames, includeEmptyIntervals);
		Table_list (table.peek(), false);
	} catch (MelderError) {
		Melder_throw (me, U": not listed.");
	}
}

void TextGrid_correctRoundingErrors (TextGrid me) {
	for (long itier = 1; itier <= my tiers -> size; itier ++) {
		Function anyTier = (Function) my tiers -> item [itier];
		if (anyTier -> classInfo == classIntervalTier) {
			IntervalTier tier = (IntervalTier) anyTier;
			TextInterval first = (TextInterval) tier -> intervals -> item [1];
			first -> xmin = my xmin;
			Melder_assert (first -> xmin < first -> xmax);
			for (long iinterval = 1; iinterval < tier -> intervals -> size; iinterval ++) {
				TextInterval left = (TextInterval) tier -> intervals -> item [iinterval];
				TextInterval right = (TextInterval) tier -> intervals -> item [iinterval + 1];
				right -> xmin = left -> xmax;
				trace (U"tier ", itier, U", interval ", iinterval, U", ", right -> xmin, U" ", right -> xmax);
				Melder_assert (right -> xmin < right -> xmax);
			}
			TextInterval last = (TextInterval) tier -> intervals -> item [tier -> intervals -> size];
			trace (tier -> intervals -> size, U" ", last -> xmax, U" ", my xmax);
			last -> xmax = my xmax;
			Melder_assert (last -> xmax > last -> xmin);
		}
		anyTier -> xmin = my xmin;
		anyTier -> xmax = my xmax;
	}
}

TextGrid TextGrids_concatenate (Collection me) {
	try {
		autoTextGrid thee = TextGrids_to_TextGrid_appendContinuous (me, false);
		TextGrid_correctRoundingErrors (thee.peek());
		return thee.transfer();
	} catch (MelderError) {
		Melder_throw (U"TextGrids not concatenated.");
	}
}

/* End of file TextGrid.cpp */
