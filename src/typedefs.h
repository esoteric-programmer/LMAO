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

#ifndef TYPEDEFSINCLUDED
#define TYPEDEFSINCLUDED

typedef struct HeLLCodePosition {
	int first_line;
	int first_column;
	int last_line;
	int last_column;
} HeLLCodePosition;

/**
 * Represents a Malbolge code word as its complete xlat2-cycle.
 * The cycle is stored as a linked list.
 */
typedef struct XlatCycle {
	/**
	 * Should be the numeric representation of one of these Malbolge commands: NOP, MOVD, OPR, JMP, ROT, OUT, IN or HLT.
	 * \sa{malbolge.h}
	 */
	unsigned char cmd;
	/**
	 * Position in .hell file. Used for generating debug informations and error messages.
	 */
	HeLLCodePosition code_position;
	/**
	 * Pointer to the next element in this xlat2 cycle.
	 */
	struct XlatCycle* next;
} XlatCycle;

/**
 * Stores the value of a memory cell in data segment.
 *
 * The value can be given directly or by the name of a label the data cell value points to. It is also possible to mark the DataCell value as don't-care or unused.
 */
typedef struct DataAtom {
	/**
	 * Contains the name of the label where the DataCell is pointing to.
	 * If it is a NULL pointer the value of the DataCell is a constant stored in the number field.
	 */
	const char* destination_label;
	/**
	 * If destination_label is not NULL this field contains a number that is added to the location referenced by destination_label.<br>
	 * If destination_label is NULL and this value is not negative it describes the absolute value of the memory cell.<br>
	 * If destination_label is NULL and this value is -2 it represents a don't-care memory cell.<br>
	 * If destination_label is NULL and this value is -1 it represents an unused memory cell.
	 */
	int number;
	/**
	 * If this is not a NULL pointer then the relative offset of the memory cell referenced by this label has to be subtracted from this cell.
	 * This field can be used to store information about an (unresolved) U_ prefix.
	 */
	const char* operand_label;
	/**
	 * Position in .hell file. Used for generating debug informations and error messages.
	 */
	HeLLCodePosition code_position;
} DataAtom;

#define DATACELL_OPERATOR_LEAF_ELEMENT 0
#define DATACELL_OPERATOR_PLUS 1
#define DATACELL_OPERATOR_MINUS 2
#define DATACELL_OPERATOR_TIMES 3
#define DATACELL_OPERATOR_DIVIDE 4
#define DATACELL_OPERATOR_ROTATE_R 5
#define DATACELL_OPERATOR_ROTATE_L 6
#define DATACELL_OPERATOR_CRAZY 7
#define DATACELL_OPERATOR_DONTCARE 8
#define DATACELL_OPERATOR_NOT_USED 9

/**
 * The value of a Malbolge memory cell that contains an element from the data section can be described by a formula.
 * Such a formula is represented by this struct (tree), so the value of the memory cell can be calculated during opcode generation.
 */
typedef struct DataCell {
	DataAtom* leaf_element;
	int _operator;
	struct DataCell* left_element;
	struct DataCell* right_element;
} DataCell;


/**
 * Double linked list of contiguous DataCells.
 */
typedef struct DataBlock {
	/**
	 * Content of current Malbolge memory cell / DataAtom.
	 */
	DataCell* data;
	/**
	 * \brief Offset of memory cell in final Malbolge program.
	 *
	 * Set to -1 for don't care or unknown
	 */
	int offset;
	/**
	 * Position in .hell file. Used for generating debug informations and error messages.
	 */
	HeLLCodePosition code_position;
	/**
	 * Succeeding element in this linked list.
	 * NULL if the current element is the last element.
	 */
	struct DataBlock* next;
	/**
	 * Previous element in this linked list.
	 * NULL if the current element is the first element.
	 */
	struct DataBlock* prev;
	/**
	 * Number of memory cells until the end of this list, including the current DataCell.
	 *
	 * Set to: 1 for the last cell, 2 for the second last, ..., n for the first.
	 */
	int num_of_blocks;
} DataBlock;

/**
 * Double linked list of contiguous XlatCycles.
 */
typedef struct CodeBlock {
	/**
	 * Current Malbolge command / XlatCycle.
	 */
	XlatCycle* command;
	/**
	 * Succeeding element in this linked list.
	 * NULL if the current element is the last element.
	 */
	int offset;
	/**
	 * Position in .hell file. Used for generating debug informations and error messages.
	 */
	HeLLCodePosition code_position;
	/**
	 * Succeeding element in this linked list.
	 * NULL if the current element is the last element.
	 */
	struct CodeBlock* next;
	/**
	 * Previous element in this linked list.
	 * NULL if the current element is the first element.
	 */
	struct CodeBlock* prev;
	/**
	 * Number of Malbolge commands until the end of this list, including the current XlatCycle.
	 *
	 * Set to: 1 for the last cell, 2 for the second last, ..., n for the first.
	 */
	int num_of_blocks;
} CodeBlock;

/**
 * TODO
 *
 * Binary search tree containing all label strings defined in the HeLL program as nodes.
 * Each node is associated to a DataBlock or a CodeBlock corresponding to its label string.
 */
typedef struct LabelTree {
	/**
	 * TODO
	 *
	 * pointer to datablock or NULL, if label is defined in code section.
	 */
	DataBlock* destination_data;
	/**
	 * TODO
	 *
	 * pointer to codeblock or NULL, if label is defined in data section.
	 */
	CodeBlock* destination_code;
	/**
	 * TODO
	 *
	 * name
	 */
	const char* label;
	/**
	 * Position in .hell file. Used for generating debug informations and error messages.
	 */
	HeLLCodePosition code_position;
	/**
	 * TODO
	 *
	 * left sub tree
	 */
	struct LabelTree* left;
	/**
	 * TODO
	 *
	 * right sub tree
	 */
	struct LabelTree* right;
} LabelTree;

/**
 * TODO
 *
 * Array containing each DataBlock defined in the HeLL program.
 */
typedef struct DataBlocks {
	/**
	 * TODO
	 */
	DataBlock** datafield;
	/**
	 * TODO
	 */
	unsigned int size;
} DataBlocks;

/**
 * TODO
 *
 * Array containing each CodeBlock defined in the HeLL program.
 */
typedef struct CodeBlocks {
	/**
	 * TODO
	 */
	CodeBlock** codefield;
	/**
	 * TODO
	 */
	unsigned int size;
} CodeBlocks;

/**
 * Value for the usage field of MemoryCell.
 *
 * This value indicates that the memory cell is not used in the initialized Malbolge program.
 */
#define UNUSED 0
/**
 * Value for the usage field of MemoryCell.
 *
 * This value indicates that the memory is used for the code section. It should be initialized directly in the generated Malbolge code whithout the need of Malbolge runtime initialization.
 */
#define PREINITIALIZED_CODE 1
/**
 * Value for the usage field of MemoryCell.
 *
 * This value indicates that the memory is used for the code section. It should be initialized during execution of the Malbolge program by using the constant generation data module.
 */
#define CODE 2
/**
 * Value for the usage field of MemoryCell.
 *
 * This value indicates that the memory is used for the data section. It should be initialized during execution of the Malbolge program by using the constant generation data module.
 */
#define DATA 3
/**
 * Value for the usage field of MemoryCell.
 *
 * This value indicates that the memory is used for the code section. Its initial value does not matter, but it must be in the range from 33-126 to prevent crashes of the Malbolge interpreter.
 */
#define RESERVED_CODE 4
/**
 * Value for the usage field of MemoryCell.
 *
 * This value indicates that the memory is used for the data section. Its initial value does not matter.
 */
#define RESERVED_DATA 5

/**
 * \brief Links to an element in the double linked lists of XlatCycles or DataCells and stores the initialization type of the memory cell.
 *
 * Create an array of these elements to calculate and store a possible memory layout of the final initialized Malbolge program.
 */
typedef struct MemoryCell {
	/**
	 * Pointer to element of the double linked list of XlatCycles that is located at this memory cell.
	 */
	CodeBlock* code;
	/**
	 * Pointer to element of the double linked list of DataCells that is located at this memory cell.
	 */
	DataBlock* data;
	/**
	 * Way of initialization and usage of this memory cell.<br>
	 * 0: unused<br>
	 * 1: preinitialized code<br>
	 * 2: code<br>
	 * 3: data<br>
	 * 4: reserved code (ASCII: 33-126)<br>
	 * 5: reserved data
	 */
	unsigned char usage;
	/**
	 * Position in .hell file. Used for generating debug informations and error messages.
	 */
	HeLLCodePosition code_position;
} MemoryCell;

/**
 * Root element of binary search tree containing all label strings defined in the HeLL program as nodes.
 * Each node is associated to a DataBlock or a CodeBlock corresponding to its label string.
 */
extern LabelTree* labeltree;

/**
 * Array containing each DataBlock defined in the HeLL program.
 */
extern DataBlocks datablocks;

/**
 * Array containing each CodeBlock defined in the HeLL program.
 */
extern CodeBlocks codeblocks;

#endif

