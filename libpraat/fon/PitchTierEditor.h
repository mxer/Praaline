#ifndef _PitchTierEditor_h_
#define _PitchTierEditor_h_
/* PitchTierEditor.h
 *
 * Copyright (C) 1992-2011,2012,2015 Paul Boersma
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

#include "RealTierEditor.h"
#include "PitchTier.h"
#include "Sound.h"

Thing_define (PitchTierEditor, RealTierEditor) {
	void v_createHelpMenuItems (EditorMenu menu)
		override;
	void v_play (double tmin, double tmax)
		override;
	double v_minimumLegalValue ()
		override { return 0.0; }
	const char32 * v_quantityText ()
		override { return U"Frequency (Hz)"; }
	const char32 * v_quantityKey ()
		override { return U"Frequency"; }
	const char32 * v_rightTickUnits ()
		override { return U" Hz"; }
	double v_defaultYmin ()
		override { return 50.0; }
	double v_defaultYmax ()
		override { return 600.0; }
	const char32 * v_setRangeTitle ()
		override { return U"Set frequency range..."; }
	const char32 * v_defaultYminText ()
		override { return U"50.0"; }
	const char32 * v_defaultYmaxText ()
		override { return U"600.0"; }
	const char32 * v_yminText ()
		override { return U"Minimum frequency (Hz)"; }
	const char32 * v_ymaxText ()
		override { return U"Maximum frequency (Hz)"; }
	const char32 * v_yminKey ()
		override { return U"Minimum frequency"; }
	const char32 * v_ymaxKey ()
		override { return U"Maximum frequency"; }
};

PitchTierEditor PitchTierEditor_create (const char32 *title,
	PitchTier pitch,
	Sound sound,   // may be NULL
	bool ownSound);

/* End of file PitchTierEditor.h */
#endif
