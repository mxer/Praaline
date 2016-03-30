/* Table_def.h
 *
 * Copyright (C) 2002-2012,2015 Paul Boersma
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


#define ooSTRUCT TableCell
oo_DEFINE_STRUCT (TableCell)

	oo_STRING (string)
	#if oo_DECLARING || oo_COPYING
		oo_DOUBLE (number)
	#endif

oo_END_STRUCT (TableCell)
#undef ooSTRUCT


#define ooSTRUCT TableRow
oo_DEFINE_CLASS (TableRow, Data)

	#if oo_DECLARING || oo_COPYING
		oo_LONG (sortingIndex)
	#endif
	oo_LONG (numberOfColumns)
	oo_STRUCT_VECTOR (TableCell, cells, numberOfColumns)

oo_END_CLASS (TableRow)
#undef ooSTRUCT


#define ooSTRUCT TableColumnHeader
oo_DEFINE_STRUCT (TableColumnHeader)

	oo_STRING (label)
	#if oo_DECLARING || oo_COPYING
		oo_INT (numericized)
	#endif

oo_END_STRUCT (TableColumnHeader)
#undef ooSTRUCT


#define ooSTRUCT Table
oo_DEFINE_CLASS (Table, Data)

	oo_LONG (numberOfColumns)
	oo_STRUCT_VECTOR (TableColumnHeader, columnHeaders, numberOfColumns)
	oo_COLLECTION (Ordered, rows, TableRow, 0)

	#if oo_DECLARING
		TableRow row (long i) // accessor
			{ return static_cast <TableRow> (rows -> item [i]); }

		void v_info ()
			override;
		bool v_hasGetNrow ()
			override { return true; }
		double v_getNrow ()
			override { return rows -> size; }
		bool v_hasGetNcol ()
			override { return true; }
		double v_getNcol ()
			override { return numberOfColumns; }
		bool v_hasGetColStr ()
			override { return true; }
		const char32 * v_getColStr (long columnNumber)
			override;
		bool v_hasGetMatrix ()
			override { return true; }
		double v_getMatrix (long rowNumber, long columnNumber)
			override;
		bool v_hasGetMatrixStr ()
			override { return true; }
		const char32 * v_getMatrixStr (long rowNumber, long columnNumber)
			override;
		bool v_hasGetColIndex ()
			override { return true; }
		double v_getColIndex (const char32 *columnLabel)
			override;
	#endif

oo_END_CLASS (Table)
#undef ooSTRUCT


/* End of file Table_def.h */
