/* EEG_def.h
 *
 * Copyright (C) 2011,2014,2015 Paul Boersma
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


#define ooSTRUCT EEG
oo_DEFINE_CLASS (EEG, Function)

	oo_LONG (numberOfChannels)
	oo_STRING_VECTOR (channelNames, numberOfChannels)
	oo_OBJECT (Sound, 2, sound)
	oo_OBJECT (TextGrid, 0, textgrid)

	#if oo_DECLARING
		void v_info ()
			override;
		int v_domainQuantity ()
			override { return MelderQuantity_TIME_SECONDS; }
		void v_shiftX (double xfrom, double xto)
			override;
		void v_scaleX (double xminfrom, double xmaxfrom, double xminto, double xmaxto)
			override;
	#endif

oo_END_CLASS (EEG)
#undef ooSTRUCT

/* End of file EEG_def.h */
