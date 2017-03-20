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

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "gen_init.h"
#include "malbolge.h"

/** returns the value needed in the A-Register to turn source into destination by crazy.
 * 0xFFFF if no such value exists. (then source has to be turned into something else e.g. C1)
 * returns a value that contains as less trits as possible different from current_register_a_value
 *
 * \param source TODO
 * \param destination TODO
 * \param current_register_a_value TODO
 * \return TODO
 */
unsigned int get_register_a_value_for_crazy(unsigned int source, unsigned int destination, unsigned int current_register_a_value);

/**
 * TODO
 *
 * \param a TODO
 * \param b TODO
 * \param mask TODO
 * \return TODO
 */
int count_different_trits(unsigned int a, unsigned int b, unsigned int mask);

/** gets a value to transform source to destination that can be generated from current_register_a_value with as less effort as possible.
 * rotations contains the number of rotation operation needed while transforming current_cell_value to the return value of this function. crazy_C2_first indicates whether source should be crazied with C2 before generating destination.
 * returns 0xFFFF if no such value exists.
 *
 * \param source TODO
 * \param destination TODO
 * \param current_register_a_value TODO
 * \param rotations TODO
 * \param crazy_C2_first TODO
 *
 * \return TODO
 */
unsigned int get_best_register_a_value_for_crazy(unsigned int source, unsigned int destination, unsigned int current_register_a_value, int* rotations, int* crazy_C2_first);

/** Adds Malbolge instruction sequence to normalized_init_code and updates cur_state corresponding to the execution of the added Malbolge instructions.
 *
 * \param normalized_init_code Copy Malbolge command into this buffer. Content of the buffer will be overwritten. Won't be terminated by zero.
 * \param current_state (Simplified) state of virtual Malbolge machine. Will be updated corresponding to the added commands.
 * \param add Null terminated string containing normalized Malbolge command sequence that will be copied to normalize_init_code and used to update cur_state.
 * \param max_init_code_length Maximum number of characters that are allowed to be copied to the normalized_init_code buffer.
 * \param no_error_printing TODO
 * \return Number of characters copied to normalized_init_code or -1 on failure. In case of failure partially write operations to normalized_init_code and cur_state may already have taken place. cur_state may be inconsistent in case of failure. */
int add_to_init_code(char* normalized_init_code, State* current_state, const char* add, int max_init_code_length, int no_error_printing);

/** Generates instructions to set the data register to a specific position.
 * Returns the number of commands generated to set the data register or -1 on error.
 *
 * \param normalized_init_code TODO
 * \param current_state TODO
 * \param pos TODO
 * \param max_init_code_length TODO
 * \param no_error_printing TODO
 * \return TODO
 * */
int set_dreg(char* normalized_init_code, State* current_state, DRegPos* pos, int max_init_code_length, int no_error_printing);

/**
 * TODO
 *
 * \param normalized_init_code TODO
 * \param current_state TODO
 * \param module TODO
 * \param position TODO
 * \param max_init_code_length TODO
 * \param no_error_printing TODO
 * \return TODO
 * */
int set_dreg_wrapper(char* normalized_init_code, State* current_state, int module, int position, int max_init_code_length, int no_error_printing);

/**
 * TODO
 *
 * \param target_value TODO
 * \param current_state TODO
 *
 * \return TODO
 */
int get_module_for_constant_generation(int target_value, State* current_state);

/**
 * TODO
 *
 * \param normalized_init_code TODO
 * \param current_state TODO
 * \param constant TODO
 * \param load_from_module TODO
 * \param rotations TODO
 * \param crazy_C2_first TODO
 * \param max_init_code_length TODO
 * \param no_error_printing TODO
 * \return TODO
 * */
int load_constant_to_a_reg(char* normalized_init_code, State* current_state, int constant, int load_from_module, int rotations, int crazy_C2_first, int max_init_code_length, int no_error_printing);

/**
 * TODO
 *
 * \param normalized_init_code TODO
 * \param current_state TODO
 * \param max_init_code_length TODO
 * \param no_error_printing TODO
 * \return TODO
 */
int increment_destination_by_9(char* normalized_init_code, State* current_state, int max_init_code_length, int no_error_printing);


unsigned int get_register_a_value_for_crazy(unsigned int source, unsigned int destination, unsigned int current_register_a_value){

	unsigned int crz[] = {1,0,0,1,0,2,2,2,1};

	unsigned int result = 0;
	unsigned int position = 0;
	unsigned int shift = 1;

	if (crazy(C0, source)==destination)
		return C0;

	if (crazy(C1, source)==destination)
		return C1;

	if (crazy(C2, source)==destination)
		return C2;

	if (crazy(C2-1, source)==destination)
		return C2-1;

	if (crazy(C2-2, source)==destination)
		return C2-2;


	while (position < 10){
		unsigned int k;
		unsigned int i = source%3;
		unsigned int j = destination%3;
		unsigned int cv = current_register_a_value%3;

		unsigned int solution = 4;
		if (crz[cv+3*i]==j) {
			solution = cv;
		}else{
			for (k=0;k<3;k++){
				if (crz[k+3*i]==j) {
					solution = k;
					break;
				}
			}
		}
		if (solution > 2)
			return 0xFFFF;

		result += solution * shift;
		shift *= 3;
		position++;
		source /= 3;
		destination /= 3;
		current_register_a_value /= 3;
	}
	return result;
}


int count_different_trits(unsigned int a, unsigned int b, unsigned int mask){
	int position = 0;
	int result = 0;
	while (position < 10){
		unsigned int i = a%3;
		unsigned int j = b%3;
		unsigned int m = mask%3;
		if (m != 0)
			result += ((i!=j)?1:0);
		a /= 3;
		b /= 3;
		mask /= 3;
		position++;
	}
	return result;
}


unsigned int get_best_register_a_value_for_crazy(unsigned int source, unsigned int destination, unsigned int current_register_a_value, int* rotations, int* crazy_C2_first){

	unsigned int value;
	int i;
	int not_modifiable_mask = C1-1;
	int min_costs = INT_MAX;
	if ((value = get_register_a_value_for_crazy(source, destination, current_register_a_value))==0xFFFF) {
		return 0xFFFF;
	}

	if (value == C0 || value == C1 || value == C2 || value == C2-1 || value == C2-2) {
		if (rotations != 0)
			*rotations = 10;
		if (crazy_C2_first != 0)
			*crazy_C2_first = 0;
		return value;
	}


	for (i=0;i<10;i++) {
		unsigned int constant = get_register_a_value_for_crazy(source, destination, current_register_a_value);
		int diff = count_different_trits(constant, current_register_a_value, C1);
		int diff2 = count_different_trits(constant, crazy(C2, current_register_a_value), C1);
		/* use mask of modifications. */
		int diff_needs_wrap = count_different_trits(constant, current_register_a_value, not_modifiable_mask);
		int diff2_needs_wrap = count_different_trits(constant, crazy(C2, current_register_a_value), not_modifiable_mask);

		int rots_diff = i+(diff_needs_wrap==0?0:10);
		int costs_diff = diff*9+rots_diff*3;
		int rots_diff2 = i+(diff2_needs_wrap==0?0:10);
		int costs_diff2 = diff2*9+rots_diff2*3+2;

		if (costs_diff == 0) {
			costs_diff = 6; /* Crazy with C2 twice to load unchanged source value */
		}

		if (costs_diff < min_costs) {
			min_costs = costs_diff;
			if (rotations != 0)
				*rotations = rots_diff;
			value = constant;
			if (crazy_C2_first != 0)
				*crazy_C2_first = 0;
		}
		if (costs_diff2 < min_costs && crazy_C2_first != 0) {
			min_costs = costs_diff2;
			if (rotations != 0)
				*rotations = rots_diff2;
			value = constant;
			*crazy_C2_first = 1;
		}		

		current_register_a_value = rotate_right(current_register_a_value);
		if (not_modifiable_mask > 0)
			not_modifiable_mask = rotate_right(not_modifiable_mask) - 1;
	}

	return value;
}

int add_to_init_code(char* normalized_init_code, State* current_state, const char* add, int max_init_code_length, int no_error_printing){
	int added=0;
	int i;
	if (add == 0 || normalized_init_code == 0) {
		if (!no_error_printing) {
			fprintf(stderr, "Internal error: Invalid parameter (Add Malbolge command).\n");
		}
		return -1;
	}

	while (*add != 0 && max_init_code_length>0){
		*normalized_init_code = *add;
		add++;
		normalized_init_code++;
		max_init_code_length--;
		added++;
	}

	if (*add != 0) { /* test if whole string has been copied */
		if (!no_error_printing) {
			fprintf(stderr, "Error: Free space exceeded (Add Malbolge command).\n");
		}
		return -1;
	}

	if (current_state == 0) {
		/* dont save result in cur_state */
		return added;
	}

	if (current_state->d_reg.pos > C2) {
		if (!no_error_printing) {
			fprintf(stderr, "Internal error: Invalid value in D register (Add Malbolge command).\n");
		}
		return -1; /* invalid value */
	}

	add -= added;

	for (i=0;i<added;i++){
		/* modify cur_state */
		switch (add[i]) {
			case 'o':
				if (current_state->d_reg.module > 3) {
					if (!no_error_printing) {
						fprintf(stderr, "Internal error: Stored constant generation module is invalid.\n");
					}
					return -1;
				} else if (current_state->d_reg.module < 0) {
					current_state->d_reg.pos++;
					current_state->d_reg.pos%=(C2+1);
					if (current_state->d_reg.pos == 82){
						/* ran into data module */
						current_state->d_reg.pos = 0;
						current_state->d_reg.module = 0;
					}
				}else{
					if (current_state->d_reg.pos+1 >= current_state->modules[current_state->d_reg.module].num_of_cells) {
						if (!no_error_printing) {
							fprintf(stderr, "Internal error: Stored position in constant generation module is invalid.\n");
						}
						return -1;
					}
					current_state->d_reg.pos++;
				}
				break;
			case 'j':
				if (current_state->d_reg.module >= 0) {
					int new_module;
					if (current_state->d_reg.module > 3){
						if (!no_error_printing) {
							fprintf(stderr, "Internal error: Stored constant generation module is invalid.\n");
						}
						return -1; /* invalid module in cur_state->d_reg.module */
					}
					if (current_state->modules[current_state->d_reg.module].num_of_cells <= current_state->d_reg.pos) {
						if (!no_error_printing) {
							fprintf(stderr, "Internal error: Stored position in constant generation module is invalid.\n");
						}
						return -1; /* invalid position in cur_state->d_reg.pos */
					}
					if (current_state->modules[current_state->d_reg.module].cells[current_state->d_reg.pos].type != CELLTYPE_PTR) {
						/* jmp outside of initialization module */
						current_state->d_reg.pos = current_state->modules[current_state->d_reg.module].cells[current_state->d_reg.pos].value+1;
						current_state->d_reg.pos %= C2+1;
						current_state->d_reg.module = -1; 
					}else{
						new_module = current_state->modules[current_state->d_reg.module].cells[current_state->d_reg.pos].destination.module;
						current_state->d_reg.pos = current_state->modules[current_state->d_reg.module].cells[current_state->d_reg.pos].destination.pos;
						current_state->d_reg.module = new_module;
					}
				}else{
					if (current_state->d_reg.pos >= current_state->last_preinitialized) {
						if ((current_state->last_preinitialized%2) != (current_state->d_reg.pos%2)) {
							if (!no_error_printing) {
								fprintf(stderr, "Internal error: Branch to 0t1111101111 indicates error.\n");
							}
							return -1; /* invalid destination: trying to branch to 0t1111101111 from outside a data module indicates an error in processing... */
						}
						/* branch into data module */
						current_state->d_reg.pos = 0;
						current_state->d_reg.module = 0;
					}else if (current_state->d_reg.pos == 1) {
						/* processing of the top of memory. */
						current_state->d_reg.pos = 'P'+1; /* preinitialized value at cell 1. */
					}else{
						/* unknown value at address. */
						if (!no_error_printing) {
							fprintf(stderr, "Internal error: Branch to unknown destination.\n");
						}
						return -1;
					}
				}
				break;
			case 'p':
				if (current_state->d_reg.module >= 0) {
					if (current_state->d_reg.module>3){
						if (!no_error_printing) {
							fprintf(stderr, "Internal error: Stored constant generation module is invalid.\n");
						}
						return -1; /* invalid module in cur_state->d_reg.module */
					}
					if (current_state->modules[current_state->d_reg.module].num_of_cells <= current_state->d_reg.pos) {
						if (!no_error_printing) {
							fprintf(stderr, "Internal error: Stored position in constant generation module is invalid.\n");
						}
						return -1; /* invalid position in cur_state->d_reg.pos */
					}
					if (current_state->modules[current_state->d_reg.module].cells[current_state->d_reg.pos].type == CELLTYPE_PTR || current_state->modules[current_state->d_reg.module].cells[current_state->d_reg.pos].type == CELLTYPE_CONST) {
						if (!no_error_printing) {
							fprintf(stderr, "Internal error: Invalid modification of constant or pointer cell.\nModule: %d, Cell: %d\n",current_state->d_reg.module,current_state->d_reg.pos);
						}
						return -1; /* invalid use of ptr-cell or const-cell for a crazy */
					}
					if (current_state->modules[current_state->d_reg.module].cells[current_state->d_reg.pos].type == CELLTYPE_C20_OR_C21 && current_state->a_reg != C0 && current_state->a_reg != C1) {
						if (!no_error_printing) {
							fprintf(stderr, "Internal error: Invalid crazy operation on C20/C21 cell.\n");
							fprintf(stderr, "Cell: %d, %d. A register: %d.\n",current_state->d_reg.module,current_state->d_reg.pos,current_state->a_reg);
						}
						return -1; /* invalid use of C20/C21 cell for a crazy */
					}
					/* do the crazy operation */
					current_state->a_reg = (current_state->modules[current_state->d_reg.module].cells[current_state->d_reg.pos].value = crazy(current_state->a_reg, current_state->modules[current_state->d_reg.module].cells[current_state->d_reg.pos].value));
				}else{
					if (current_state->d_reg.pos >= current_state->last_preinitialized) {
						/* save result of crazy in a_reg only */
						current_state->a_reg = crazy(current_state->a_reg, (((current_state->last_preinitialized%2) == (current_state->d_reg.pos%2))?81:(C1-81)));
					}else{
						if (!no_error_printing) {
							fprintf(stderr, "Internal error: Tried initialization of preinitialized cell %d.\n",current_state->d_reg.pos);
						}
						return -1; /*not supported */
					}
				}

				if (current_state->d_reg.module > 3) {
					if (!no_error_printing) {
						fprintf(stderr, "Internal error: Stored constant generation module is invalid.\n");
					}
					return -1;
				}
				else if (current_state->d_reg.module < 0) {
					current_state->d_reg.pos++;
					current_state->d_reg.pos%=(C2+1);
					if (current_state->d_reg.pos == 82){
						/* ran into data module */
						current_state->d_reg.pos = 0;
						current_state->d_reg.module = 0;
					}
				}else{
					if (current_state->d_reg.pos+1 >= current_state->modules[current_state->d_reg.module].num_of_cells) {
						if (!no_error_printing) {
							fprintf(stderr, "Internal error: Stored position in constant generation module is invalid.\n");
						}
						return -1;
					}
					current_state->d_reg.pos++;
				}

				break;
			case '*':
				if (current_state->d_reg.module >= 0) {
					if (4 <= current_state->d_reg.module){
						if (!no_error_printing) {
							fprintf(stderr, "Internal error: Stored constant generation module is invalid.\n");
						}
						return -1; /* invalid module in cur_state->d_reg.module */
					}
					if (current_state->modules[current_state->d_reg.module].num_of_cells <= current_state->d_reg.pos) {
						if (!no_error_printing) {
							fprintf(stderr, "Internal error: Stored position in constant generation module is invalid.\n");
						}
						return -1; /* invalid position in cur_state->d_reg.pos */
					}
					if (current_state->modules[current_state->d_reg.module].cells[current_state->d_reg.pos].type == CELLTYPE_PTR || current_state->modules[current_state->d_reg.module].cells[current_state->d_reg.pos].type == CELLTYPE_C20_OR_C21) {
						if (!no_error_printing) {
							fprintf(stderr, "Internal error: Invalid modification of C20/C21 or pointer cell.\nModule: %d, Cell: %d\n",current_state->d_reg.module,current_state->d_reg.pos);
						}
						return -1; /* invalid use of ptr-cell or const-cell for a rotate */
					}
					if (current_state->modules[current_state->d_reg.module].cells[current_state->d_reg.pos].type == CELLTYPE_CONST) {
						int cellval = current_state->modules[current_state->d_reg.module].cells[current_state->d_reg.pos].value;
						if (cellval != C0 && cellval != C1 && cellval != C2)
							return -1; /* invalid constant manipulation */
					}
					/* do the rotate operation */
					current_state->a_reg = (current_state->modules[current_state->d_reg.module].cells[current_state->d_reg.pos].value = rotate_right(current_state->modules[current_state->d_reg.module].cells[current_state->d_reg.pos].value));
				}else{
					return -1; /*not supported */
				}

				if (current_state->d_reg.module > 3) {
					if (!no_error_printing) {
						fprintf(stderr, "Internal error: Stored constant generation module is invalid.\n");
					}
					return -1;
				}
				else if (current_state->d_reg.module < 0) {
					current_state->d_reg.pos++;
					current_state->d_reg.pos%=(C2+1);
					if (current_state->d_reg.pos == 82){
						/* ran into data module */
						current_state->d_reg.pos = 0;
						current_state->d_reg.module = 0;
					}
				}else{
					if (current_state->d_reg.pos+1 >= current_state->modules[current_state->d_reg.module].num_of_cells) {
						if (!no_error_printing) {
							fprintf(stderr, "Internal error: Stored position in constant generation module is invalid.\n");
						}
						return -1;
					}
					current_state->d_reg.pos++;
				}

				break;
			default:
				return -1; /*invalid or not supported malbolge command */
		}
	}

	return added;

}


int set_dreg(char* normalized_init_code, State* current_state, DRegPos* pos, int max_init_code_length, int no_error_printing){
	int result = 0;
	#ifndef DOXYGEN
	#define ADD_CODE(code) { int res = add_to_init_code(normalized_init_code, current_state, (code), max_init_code_length, no_error_printing); if (res < 0) return -1; result += res; normalized_init_code += res; max_init_code_length -= res;}
	#endif
	if (current_state == 0 || normalized_init_code == 0 || pos == 0 || max_init_code_length < 0)
		return -1;
	if (current_state->d_reg.pos > C2 || pos->pos > C2)
		return -1; /* invalid value */

	if (pos->module < 0)
		return -1; /* not supported */
	if (pos->module > 3 || current_state->d_reg.module > 3)
		return -1; /* invalid value */

	if (current_state->modules[pos->module].num_of_cells <= pos->pos)
		return -1; /* invalid value */

	if (current_state->d_reg.module >= 0)
		if (current_state->modules[current_state->d_reg.module].num_of_cells <= current_state->d_reg.pos)
			return -1; /* invalid value */

	if (current_state->d_reg.module < 0) {
		/* move into module 0 */
		int ret;

		if (current_state->d_reg.pos >= current_state->last_preinitialized) {
			if ((current_state->last_preinitialized%2) != (current_state->d_reg.pos%2)) {
				ADD_CODE("o");
			}
		}
		if (current_state->d_reg.pos >= current_state->last_preinitialized) {
			if ((current_state->last_preinitialized%2) != (current_state->d_reg.pos%2)) {
				return -1; /* internal error! should not occur! */
			}
			ADD_CODE("j");
		}else if (current_state->d_reg.pos <= 1) {
			/* processing of the top of memory. */
			if (current_state->d_reg.pos == 0) {
				ADD_CODE("o");
			}
			ADD_CODE("jo");

		}else{
			return -1; /* not supported! */
		}
		/* call set_dreg again! */
		ret = set_dreg(normalized_init_code, current_state, pos, max_init_code_length, no_error_printing);
		if (ret < 0) {
			return -1; /* error */
		}
		return result+ret;
	}

	if (current_state->d_reg.module == pos->module){
		/* move within the module! */

		do {

			if (current_state->d_reg.pos == pos->pos) {
				/* we did it! */
				return result;
			}

			if (current_state->modules[current_state->d_reg.module].cells[current_state->d_reg.pos].type == CELLTYPE_PTR) {
				if (current_state->modules[current_state->d_reg.module].cells[current_state->d_reg.pos].destination.module == pos->module) {
					if (current_state->modules[current_state->d_reg.module].cells[current_state->d_reg.pos].destination.pos > current_state->d_reg.pos && current_state->modules[current_state->d_reg.module].cells[current_state->d_reg.pos].destination.pos <= pos->pos) {
						/* use abbreviation!!! jmp forward! */
						ADD_CODE("j");
						continue;
					}else if (current_state->modules[current_state->d_reg.module].cells[current_state->d_reg.pos].destination.pos <= pos->pos && current_state->d_reg.pos > pos->pos) {
						/* use backward jmp to reach destination */
						ADD_CODE("j");
						continue;
					}
				}
			}
			ADD_CODE("o");
		}while (current_state->modules[current_state->d_reg.module].num_of_cells >= current_state->d_reg.pos);
		return -1; /* not successful. ran out of module. */

	}else if (current_state->d_reg.module == 0) {
		/* move from module 0 to other module */
		int start_pos = current_state->d_reg.pos;
		int ret;
		do {
			if (current_state->modules[current_state->d_reg.module].cells[current_state->d_reg.pos].type == CELLTYPE_PTR) {
				if (current_state->modules[current_state->d_reg.module].cells[current_state->d_reg.pos].destination.module == pos->module) {
					ADD_CODE("j");
					break;
				}else if (current_state->modules[current_state->d_reg.module].num_of_cells == current_state->d_reg.pos+1) {
					/* last cell of module 0 points back to its beginning */
					ADD_CODE("j");
					continue;
				}
				
			}
			ADD_CODE("o");
		}while (current_state->d_reg.pos != start_pos);
		if (current_state->d_reg.module != pos->module) {
			return -1; /* error! */
		}

		/* move within the module */
		ret = set_dreg(normalized_init_code, current_state, pos, max_init_code_length, no_error_printing);
		if (ret < 0) {
			return -1; /* error */
		}
		return result+ret;

	}else if (pos->module == 0) {
		int ret;
		/* move from other module to module 0: in the current data modules links to modul 0 only exists at their end */
		while (current_state->modules[current_state->d_reg.module].num_of_cells > current_state->d_reg.pos) {
			if (current_state->modules[current_state->d_reg.module].cells[current_state->d_reg.pos].type == CELLTYPE_PTR) {
				if (current_state->modules[current_state->d_reg.module].cells[current_state->d_reg.pos].destination.module == 0) {
					ADD_CODE("j");
					break;
				}
				
			}
			ADD_CODE("o");
		}
		if (current_state->d_reg.module != 0) {
			return -1; /* error! */
		}

		/* move within module 0 */
		ret = set_dreg(normalized_init_code, current_state, pos, max_init_code_length, no_error_printing);
		if (ret < 0) {
			return -1; /* error */
		}
		return result+ret;
	}else{
		/* first move into module 0 */
		int result = set_dreg(normalized_init_code, current_state, pos, max_init_code_length, no_error_printing);
		int result2;
		if (result <= 0) {
			return -1;
		}
		/* then move to destination module */
		result2 = set_dreg(normalized_init_code, current_state, pos, max_init_code_length-result, no_error_printing);
		if (result2 <= 0) {
			return -1;
		}
		return result + result2;
	}
	#undef ADD_CODE
}


int set_dreg_wrapper(char* normalized_init_code, State* current_state, int module, int position, int max_init_code_length, int no_error_printing){
	DRegPos pos;
	DRegPos old_pos;
	int result;
	pos.module = module;
	pos.pos = position;
	old_pos.module = current_state->d_reg.module;
	old_pos.pos = current_state->d_reg.pos;
	result = set_dreg(normalized_init_code, current_state, &pos, max_init_code_length, no_error_printing);
	if (result >= 0){
		if (current_state->d_reg.module != module || current_state->d_reg.pos != position) {
			if (!no_error_printing) {
				fprintf(stderr,"Internal error: Tried to move D register to %d, %d but it is now at %d, %d.\n",module,position,current_state->d_reg.module,current_state->d_reg.pos);
				fprintf(stderr, "Previous position was %d, %d.\n",old_pos.module,old_pos.pos);
			}
			return -1;
		}
	}
	return result;
}


int get_module_for_constant_generation(int target_value, State* current_state){
	int load_from_module;
	/* select module to generate constant in: */
	if (target_value <= 161)
		load_from_module = 3;
	else if (target_value <= current_state->last_preinitialized)
		load_from_module = 1;
	else
		load_from_module = 2;
	return load_from_module;
}


int load_constant_to_a_reg(char* normalized_init_code, State* current_state, int constant, int load_from_module, int rotations, int crazy_C2_first, int max_init_code_length, int no_error_printing){

	int result = 0;
	#ifndef DOXYGEN
	#define ADD_CODE(code) { int res = add_to_init_code(normalized_init_code, current_state, (code), max_init_code_length, no_error_printing); if (res < 0) return -1; result += res; normalized_init_code += res; max_init_code_length -= res;}
	#define SET_DREG(module, position) {  int res = set_dreg_wrapper(normalized_init_code, current_state, (module), (position), max_init_code_length, no_error_printing); if (res < 0) return -1; result += res; normalized_init_code += res; max_init_code_length -= res;}
	#endif

	int old_val;

	/* load constant into A (maybee generate it first) */ \
	if ((constant) == C1){
		SET_DREG(1,1);
		ADD_CODE("*");
		return result;
	}else if ((constant) == C2) {
		SET_DREG(1,4);
		ADD_CODE("*");
		return result;
	}else if ((constant) == C0) {
		SET_DREG(0,0);
		ADD_CODE("*");
		return result;
	}else if ((constant) == C2-1 || (constant) == C2-2){
		if ((constant) == C2-1) {
			SET_DREG(0,0);
		}else{
			SET_DREG(1,1);
		}
		ADD_CODE("*");
		SET_DREG(1,2);
		ADD_CODE("p");
		return result;
	}

	/* read current constant: stored in cell 3. // TODO: check if cell 3 exists; check type of cell 3! */
	old_val = current_state->modules[load_from_module].cells[3].value;
		
	if (constant == old_val && rotations == 0){
		/* reuse generated constant */
		SET_DREG(load_from_module, 4);
		ADD_CODE("*");
		SET_DREG(load_from_module, 3);
		ADD_CODE("p");
		SET_DREG(load_from_module, 4);
		ADD_CODE("*");
		SET_DREG(load_from_module, 3);
		ADD_CODE("p");
	}else{
		int i;
		/* generate constant! */
		/* 0->1: C2, C20 */
		/* 0->2: C20, C2 */
		/* 1->0: C21, C2 */
		/* 1->2: C20, C2 / C2, C20 */
		/* 2->0: C2, C21 */
		/* 2->1: C2, C20 / C20, C2 */
		int c2_outstanding = crazy_C2_first;
		int currentConstant = constant;

		for (;rotations>10;){
			/* rotate! */
			SET_DREG(load_from_module, 3);
			ADD_CODE("*");
			rotations--;
		}

		for (i=rotations;i<10;i++) {
			currentConstant = rotate_right(currentConstant);
		}

		for (i=10-rotations;i<11;i++){
			unsigned int curBit;
			unsigned int genBit;
			unsigned int useP;
			unsigned int C2_at_beginning = 0;
			int currentCell = current_state->modules[load_from_module].cells[3].value;
			if (c2_outstanding) {
				currentCell = crazy(C2, currentCell);
			}
			curBit = currentCell%3;
			genBit = currentConstant%3;
			currentConstant = rotate_right(currentConstant);
			useP = 2; /* 2: none! */
			if ((curBit == 0 && (genBit == 1 || genBit == 2)) || (curBit == 1 && genBit == 2)  || (curBit == 2 && genBit == 1) )
				useP = 0;
			else if ((curBit == 1 && genBit == 0) || (curBit == 2 && genBit == 0))
				useP = 1;
			if (useP != 2) {
				/* do crazy with C2 if necessary */
				if ((curBit == 0 && genBit == 1) || (curBit == 2 && (genBit == 0))){
					C2_at_beginning = 1;
				}
				if ((curBit == 2 && genBit == 1) || (curBit == 1 && genBit == 2)) {
					C2_at_beginning = c2_outstanding; /* C2 can be done whenever we want. */
					/* we dont want to do it right now. */
				}
				if (C2_at_beginning) {
					if (c2_outstanding) {
						c2_outstanding = 0;
					}else{
						c2_outstanding = 1;
					}
				}
				if (c2_outstanding){
					/* crazy with C2 */
					SET_DREG(load_from_module, 4);
					ADD_CODE("*");
					SET_DREG(load_from_module, 3);
					ADD_CODE("p");
					c2_outstanding = 0;
				}
				/* crazy with C2_useP (C20/C21)! */
				if (useP == 1) {
					SET_DREG(load_from_module, 0);
				}else{
					SET_DREG(load_from_module, 1);
				}
				ADD_CODE("*");
				SET_DREG(load_from_module, 2);
				ADD_CODE("p");
				SET_DREG(load_from_module, 3);
				ADD_CODE("p");
			}
			/* save that a crazy with c2 could be neccessary */ \
			if (curBit != genBit && !C2_at_beginning)
				c2_outstanding = 1;
			/* rotate! */
			if (i < 10) {
				SET_DREG(load_from_module, 3);
				ADD_CODE("*");
			}else
				break;
		}
		if (c2_outstanding) {
			/* crazy with C2 */
			SET_DREG(load_from_module, 4);
			ADD_CODE("*");
			SET_DREG(load_from_module, 3);
			ADD_CODE("p");
			c2_outstanding = 0;
		}
		/**dmod_VALUE = (constant); */
	}
	#undef SET_DREG
	#undef ADD_CODE
	return result;
}


int increment_destination_by_9(char* normalized_init_code, State* current_state, int max_init_code_length, int no_error_printing){

	int result = 0;
	#ifndef DOXYGEN
	#define ADD_CODE(code) { int res = add_to_init_code(normalized_init_code, current_state, (code), max_init_code_length, no_error_printing); if (res < 0) return -1; result += res; normalized_init_code += res; max_init_code_length -= res; }
	#define SET_DREG(module, position) {  int res = set_dreg_wrapper(normalized_init_code, current_state, (module), (position), max_init_code_length, no_error_printing); if (res < 0) return -1; result += res; normalized_init_code += res; max_init_code_length -= res; }
	#endif

	/* read current destination: stored in cell 12. // TODO: check if cell 12 exists; check type of cell 12! */
	int old_dest_val = current_state->modules[0].cells[12].value;

	unsigned int overflow = 0;
	/* clr tmp: ROT C1, CRAZY tmp */
	SET_DREG(0,5);
	ADD_CODE("*");
	SET_DREG(0,6);
	ADD_CODE("p");
	/* ROTR C2 */
	SET_DREG(0,11);
	ADD_CODE("*");
	/* CRAZY destination */
	SET_DREG(0,12);
	ADD_CODE("p");
	/* CRAZY tmp */ \
	SET_DREG(0,6);
	ADD_CODE("p");
	/* CRAZY carry */
	/* get trinary overflow! */
	if ((old_dest_val+9)/27 > old_dest_val/27){
		/* overflow level 1 */
		overflow = 1;
		if ((old_dest_val+9)/81 > old_dest_val/81){
			/* overflow level 2 */
			overflow = 2;
			if ((old_dest_val+9)/(81*3) > old_dest_val/(81*3)){
				/* overflow level 3 */
				overflow = 3;
				if ((old_dest_val+9)/(81*9) > old_dest_val/(81*9)){
					/* overflow level 4 */
					if (!no_error_printing) {
						fprintf(stderr,"Internal error: Carry has not been detected correctly.\nRan into wrong execution path...\n");
					}
					return -1;
				}
			}
		}
	}
	SET_DREG(0,7+3-overflow);
	ADD_CODE("p");
	/* CRAZY destination */
	SET_DREG(0,12);
	ADD_CODE("p");
	#undef SET_DREG
	#undef ADD_CODE
	return result;
}


int denormalize_malbolge(char* normalized_code, int normalized_code_offset, int code_len, int no_error_printing) {
	unsigned int c=normalized_code_offset;
	int i = 0;
	for (i=0;i<code_len;i++) {
		unsigned char in=(unsigned char)(*normalized_code);

		unsigned int instruction = 0;
		switch(in){
			case 'o': instruction = MALBOLGE_COMMAND_NOP; break;
			case 'j': instruction = MALBOLGE_COMMAND_MOVED; break;
			case 'p': instruction = MALBOLGE_COMMAND_OPR; break;
			case '*': instruction = MALBOLGE_COMMAND_ROT; break;
			case 'i': instruction = MALBOLGE_COMMAND_JMP; break;
			case '<': instruction = MALBOLGE_COMMAND_OUT; break;
			case '/': instruction = MALBOLGE_COMMAND_IN; break;
			case 'v': instruction = MALBOLGE_COMMAND_HALT; break;
		}
		if (instruction != 0){
			unsigned int code = ((instruction+94-(c%94))%94);
			if (code < 33)
				code+=94;
			*normalized_code = (char)code;
			c++;
		}else{
			/* blanks, non-normalized symbols and other are not allowed. */
			if (!no_error_printing) {
				fprintf(stderr,"Internal error: Found character: '%c' (%d) at %d during code denormalization.\n",*normalized_code,in,c);
			}
			return 0;
		}
		normalized_code++;
	}
	return 1;
}


int generate_normalized_init_code_for_word_with_module_system(int init_position, int init_value, char* normalized_init_code, State* current_state, int max_init_code_length, int no_error_printing){

	int result = 0;
	#ifndef DOXYGEN
	#define ADD_CODE(code) { int res = add_to_init_code(normalized_init_code, current_state, (code), max_init_code_length, no_error_printing); if (res < 0) return -1; result += res; normalized_init_code += res; max_init_code_length -= res;}
	#define SET_DREG(module, position) {  int res = set_dreg_wrapper(normalized_init_code, current_state, (module), (position), max_init_code_length, no_error_printing); if (res < 0) return -1; result += res; normalized_init_code += res; max_init_code_length -= res; }
	#define LOAD_CONSTANT(constant, module, rotations, crazy_C2_first) {  int res = load_constant_to_a_reg(normalized_init_code, current_state, (constant), (module), (rotations), (crazy_C2_first), max_init_code_length, no_error_printing); if (res < 0) return -1; result += res; normalized_init_code += res; max_init_code_length -= res; }
	#define INCREMENT_BY_9() {  int res = increment_destination_by_9(normalized_init_code, current_state, max_init_code_length, no_error_printing); if (res < 0) return -1; result += res; normalized_init_code += res; max_init_code_length -= res; }
	#endif

	int old_mem_val;
	int constant;
	int rotations=10;
	int crazy_C2_first=0;

	if (current_state == 0 || normalized_init_code == 0 || max_init_code_length < 0 || init_value < 0 || init_value > C2 || init_position <= current_state->last_preinitialized || current_state->last_preinitialized < 0 || current_state->last_preinitialized > C2 || init_position > C2) {
		/* invalid function parameter(s) */
		if (!no_error_printing) {
			fprintf(stderr,"Internal error: Invalid call of generate_normalized_init_code_for_word function.\n");
		}
		return -1;
	}

	old_mem_val = (((init_position%2)==(current_state->last_preinitialized%2))?81:(C1-81));

	if (old_mem_val == init_value) {
		/* nothing to do */
		return 0;
	}

	/* - Generate destination address if necessary */
	/* - Test if a constant exists that can be crazied into the destination cell to reach the required value (call getCrazyAValue) */
	/*   - If it does: Generate the constant */
	/*   - Otherwise: Clear the destination memory cell to C1 (by crazying C0 into it), then call getCrazyAValue again and generate the constant */
	/* - Crazy the constant into the destination address */

	/* Test if a completely new destination address has to be generated: */
	if (current_state->modules[0].cells[12].value + 1 > init_position || current_state->modules[0].cells[12].value + 1 + 9 + 9 <= init_position || (current_state->modules[0].cells[12].value)/(3*3*3*3*3*3) < (current_state->modules[0].cells[12].value+9)/(3*3*3*3*3*3)){
		unsigned int constant;
		/* Generate new destination address */
		rotations = 10;
		crazy_C2_first = 0;
		constant = get_best_register_a_value_for_crazy(current_state->modules[0].cells[12].value, init_position-1, current_state->modules[get_module_for_constant_generation(init_position-1, current_state)].cells[3].value,&rotations,&crazy_C2_first);
		if (constant == 0xFFFF){
			/* bring to C1. */
			SET_DREG(1,1);
			ADD_CODE("*");
			SET_DREG(0,12);
			ADD_CODE("p");
			SET_DREG(0,12);
			ADD_CODE("p");
			rotations = 10;
			crazy_C2_first = 0;
			constant = get_best_register_a_value_for_crazy(current_state->modules[0].cells[12].value, init_position-1, current_state->modules[get_module_for_constant_generation(init_position-1, current_state)].cells[3].value,&rotations,&crazy_C2_first);
			if (constant == 0xFFFF){
				/* constant is C1, so this could not occur. */
				if (!no_error_printing) {
					fprintf(stderr,"Internal error: Error calculating a value.\n");
				}
				return -1;
			}
		}
		/* use constant generation module to generate value of constant in A-reg */
		LOAD_CONSTANT(constant, get_module_for_constant_generation(init_position-1, current_state),rotations,crazy_C2_first);
		/* crazy A-reg into DESTINATION to get destination address (init_position-1) */
		SET_DREG(0,12);
		ADD_CODE("p");

	}else if (current_state->modules[0].cells[12].value + 1 + 9 <= init_position) {
		/* incement DESTINATION by 9. */
		INCREMENT_BY_9();
	}

	/* check if data can be stored directly */
	rotations = 10;
	crazy_C2_first = 0;
	constant = get_best_register_a_value_for_crazy(old_mem_val, init_value, current_state->modules[get_module_for_constant_generation(init_value, current_state)].cells[3].value,&rotations,&crazy_C2_first);
	if (constant == 0xFFFF){
		/* this is not the case -> bring mem[DESTINATION] 2 C1 by crazying with C0. (mem[init_position] does not contain trits with a value of 2.) */
		/* laod C0 */
		SET_DREG(0,0);
		ADD_CODE("*");
		/* crazy it into memory cell */
		SET_DREG(0,12);
		ADD_CODE("j");
		if (current_state->d_reg.module >= 0) {
			if (!no_error_printing) {
				fprintf(stderr,"Internal error: Not at memoy cell!\n");
			}
			return -1;
		}
		while(current_state->d_reg.pos != init_position) {
			ADD_CODE("o");
		}
		ADD_CODE("p");
		old_mem_val = C1;
		rotations = 10;
		crazy_C2_first = 0;
		constant = get_best_register_a_value_for_crazy(old_mem_val, init_value, current_state->modules[get_module_for_constant_generation(init_value, current_state)].cells[3].value,&rotations,&crazy_C2_first);
		if (constant == 0xFFFF){
			/* destination memory cell contains C1, so this could not occur. */
			if (!no_error_printing) {
				fprintf(stderr,"Internal error: Error calculating a value.\n");
			}
			return -1;
		}
	}

	/* generate or reuse constant */
	LOAD_CONSTANT(constant, get_module_for_constant_generation(init_value, current_state), rotations, crazy_C2_first);
	/* crazy constant into memory cell */
	SET_DREG(0,12);
	ADD_CODE("j");
	if (current_state->d_reg.module >= 0) {
		if (!no_error_printing) {
			fprintf(stderr,"Internal error: Not at memoy cell!\n");
		}
		return -1;
	}
	while(current_state->d_reg.pos != init_position) {
		ADD_CODE("o");
	}
	ADD_CODE("p");

	return result;

	#undef INCREMENT_BY_9
	#undef LOAD_CONSTANT
	#undef SET_DREG
	#undef ADD_CODE
}



int generate_jump_to_entrypoint_with_module_system(int entry_point, char* normalized_init_code, State* cur_state, int max_init_code_length, int no_error_printing){

	int init_position = entry_point;
	int result = 0;
	#ifndef DOXYGEN
	#define ADD_CODE(code) { int res = add_to_init_code(normalized_init_code, cur_state, (code), max_init_code_length, no_error_printing); if (res < 0) return -1; result += res; normalized_init_code += res; max_init_code_length -= res;}
	#define SET_DREG(module, position) {  int res = set_dreg_wrapper(normalized_init_code, cur_state, (module), (position), max_init_code_length, no_error_printing); if (res < 0) return -1; result += res; normalized_init_code += res; max_init_code_length -= res; }
	#define LOAD_CONSTANT(constant, module, rotations, crazy_C2_first) {  int res = load_constant_to_a_reg(normalized_init_code, cur_state, (constant), (module), (rotations), (crazy_C2_first), max_init_code_length, no_error_printing); if (res < 0) return -1; result += res; normalized_init_code += res; max_init_code_length -= res; }
	#define INCREMENT_BY_9() {  int res = increment_destination_by_9(normalized_init_code, cur_state, max_init_code_length, no_error_printing); if (res < 0) return -1; result += res; normalized_init_code += res; max_init_code_length -= res; }
	#endif

	int rotations=10;
	int crazy_C2_first=0;

	if (cur_state == 0 || normalized_init_code == 0 || max_init_code_length < 0 || cur_state->last_preinitialized < 0 || cur_state->last_preinitialized > C2 || init_position > C2) {
		/* invalid function parameter(s) */
		if (!no_error_printing) {
			fprintf(stderr,"Internal error: Invalid call of generate_normalized_init_code_for_word function.\n");
		}
		return -1;
	}


	/* - Generate destination address */
	/* - Jump to destination address */

	/* Test if a completely new destination address has to be generated: */
	if (cur_state->modules[0].cells[12].value + 1 > init_position || cur_state->modules[0].cells[12].value + 1 + 9 + 9 <= init_position || (cur_state->modules[0].cells[12].value)/(3*3*3*3*3*3) < (cur_state->modules[0].cells[12].value+9)/(3*3*3*3*3*3)){
		/* Generate new destination address */
		unsigned int constant = get_best_register_a_value_for_crazy(cur_state->modules[0].cells[12].value, init_position-1, cur_state->modules[get_module_for_constant_generation(init_position-1, cur_state)].cells[3].value,&rotations,&crazy_C2_first);
		if (constant == 0xFFFF){
			/* bring to C1. */
			SET_DREG(1,1);
			ADD_CODE("*");
			SET_DREG(0,12);
			ADD_CODE("p");
			SET_DREG(0,12);
			ADD_CODE("p");
			rotations=10;
			crazy_C2_first=0;
			constant = get_best_register_a_value_for_crazy(cur_state->modules[0].cells[12].value, init_position-1, cur_state->modules[get_module_for_constant_generation(init_position-1, cur_state)].cells[3].value,&rotations,&crazy_C2_first);
			if (constant == 0xFFFF){
				/* constant is C1, so this could not occur. */
				if (!no_error_printing) {
					fprintf(stderr,"Internal error: Error calculating a value.\n");
				}
				return -1;
			}
		}
		/* use constant generation module to generate value of constant in A-reg */
		LOAD_CONSTANT(constant, get_module_for_constant_generation(init_position-1, cur_state), rotations, crazy_C2_first);

		/* crazy A-reg into DESTINATION to get destination address (init_position-1) */
		SET_DREG(0,12);
		ADD_CODE("p");
	}else if (cur_state->modules[0].cells[12].value + 1 + 9 <= init_position) {
		/* incement DESTINATION by 9. */
		INCREMENT_BY_9();
	}

	/* go to destination address */
	SET_DREG(0,12);
	ADD_CODE("j");
	if (cur_state->d_reg.module >= 0) {
		if (!no_error_printing) {
			fprintf(stderr,"Internal error: Not at memoy cell!\n");
		}
		return -1;
	}
	while(cur_state->d_reg.pos != init_position) {
		ADD_CODE("o");
	}

	return result;

	#undef INCREMENT_BY_9
	#undef LOAD_CONSTANT
	#undef SET_DREG
	#undef ADD_CODE
}

