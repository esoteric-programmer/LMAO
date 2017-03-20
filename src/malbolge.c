/*

	This file is part of LMAO (Low-level Malbolge Assembler, Ooh!), an assembler for Malbolge.
	Copyright (C) 2013-2017 Matthias Lutter

	LMAO is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	LMAO is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.

	E-Mail: matthias@lutter.cc

*/

#include "malbolge.h"

unsigned int crazy(unsigned int a, unsigned int d){
	unsigned int crz[] = {1,0,0,1,0,2,2,2,1};
	int position = 0;
	unsigned int output = 0;
	while (position < 10){
		unsigned int i = a%3;
		unsigned int j = d%3;
		unsigned int out = crz[i+3*j];
		unsigned int multiple = 1;
		int k;
		for (k=0;k<position;k++)
			multiple *= 3;
		output += multiple*out;
		a /= 3;
		d /= 3;
		position++;
	}
	return output;
}


unsigned int rotate_right(unsigned int d){
	unsigned int carry = d%3;
	d /= 3;
	d += 19683 * carry;
	return d;
}

