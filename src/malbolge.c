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

