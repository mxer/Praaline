#ifndef _Matrix_extensions_h_
#define _Matrix_extensions_h_
/* Matrix_extensions.h
 *
 * Copyright (C) 1993-2011 David Weenink
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
 djmw 20020813 GPL header
 djmw 20121110 Latest modification
*/

#include "Matrix.h"
#include "Graphics.h"

void Matrix_scatterPlot (Matrix me, Graphics g, long icx, long icy,
    double xmin, double xmax, double ymin, double ymax,
    double size_mm, const char32 *mark, int garnish);
/* Draw my columns ix and iy as a scatterplot (with squares)				*/

void Matrix_drawAsSquares (Matrix me, Graphics graphics, double xmin, double xmax, double ymin, double ymax, int garnished);
/* Draw a Matrix as small squares whose area correspond to the matrix element */
/* The square is filled with black if the weights are negative					*/

void Matrix_drawRowsAsLineSegments (Matrix me, Graphics g, double xmin, double xmax, double ymin, double ymax,
	double minimum, double maximum, int connect);
/* draw a row as segments */

void Matrix_scale (Matrix me, int choice);
/* choice = 1 :divide each elmnt by the maximum (abs) */
/* choice = 2 :rows, divide each row elmnt by the maximum (abs) of that row	*/
/* choice = 3 :columns, divide each col elmnt by the maximum of that col	*/

Matrix Matrix_transpose (Matrix me);

void Matrix_drawDistribution (Matrix me, Graphics g, double xmin, double xmax,
	double ymin, double ymax, double minimum, double maximum, long nBins,
	double freqMin, double freqmax, int cumulative, int garnish);

void Matrix_drawSliceY (Matrix me, Graphics g, double x, double ymin, double ymax,
	double min, double max);

int Matrix_fitPolynomial (Matrix me, long maxDegree);

Matrix Matrix_solveEquation (Matrix me, double tolerance);

double Matrix_getMean (Matrix me, double xmin, double xmax, double ymin, double ymax);
double Matrix_getStandardDeviation (Matrix me, double xmin, double xmax, double ymin, double ymax);


#endif /* _Matrix_extensions_h_ */
