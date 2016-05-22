/*

	This file is part of LMAO (Low-level Malbolge Assembler, Ooh!), an assembler for Malbolge.
	Copyright (C) 2013 Matthias Ernst

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

	E-Mail: info@matthias-ernst.eu

*/

#ifndef INITIALIZEINCLUDED
#define INITIALIZEINCLUDED

#include "typedefs.h"

/**
 * This function sets the offsets stored in the codeblocks and datablocks to fit to the given memory layout.
 *
 * \param memory Array of GeneralMemoryCells corresponding to current memory layout. This array must contain exactly C2+1=59049 elements. The offset of each codeblock and datablock referenced by the memory cells will be updated.
 */
void update_offsets(MemoryCell* memory);

/**
 * \brief Generates opcodes for memory cells of the final initialized Malbolge program.
 *
 * After that step the initialization code can be generated to get the final Malbolge code.
 *
 * \param memory_layout Memory layout that should be converted into opcodes. The size of this array must be C2+1=59049.
 * \param last_preinitialized Address of last memory cell that will be initialized directly after the Malbolge interpreter read in the source file.
 * \param opcodes The generated opcodes will be written into this array. If no opcode is generated for a memory cell, it will be set to -1. The size of this array must be C2+1=59049.
 * \param no_error_printing If set to non-zero, error messages won't be printed to stdout.
 * \return If an error occurs, zero will be returned. Otherwise, a non-zero value will be returned.
 */
int generate_opcodes_from_memory_layout(MemoryCell* memory_layout, int last_preinitialized, int* opcodes, LabelTree* labeltree, int no_error_printing, int ignore_fixed_offsets_in_preinitialized_section);

/**
 * \brief This function creates a Malbolge program from given memory cells.
 *
 * Most cell values won't be valid initial values for Malbolge, so this function generates code to initiate that cells to their values.
 * \param program TODO
 * \param last_preinitialized TODO
 * \param labeltree TODO
 * \param entrypoint TODO
 * \param malbolge_code TODO
 * \param no_error_printing TODO
 * \return  -1 on error; otherwise: size needed for pure initialization code (without NOPs used for filling up; without preinitialized code words.)
 */
int generate_malbolge_initialization_code(int program[], int last_preinitialized, int entrypoint, char malbolge_code[], int no_error_printing, int* execution_steps_until_entry_point, int ignore_wrong_size);

#endif

