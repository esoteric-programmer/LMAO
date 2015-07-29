#ifndef MAININCLUDED
#define MAININCLUDED

#include "typedefs.h"
#include <stdio.h>

/**
 * Searches DataBlock or CodeBlock referenced by a given string in the LabelTree.
 *
 * \param destination_code The pointer to the CodeBlock will be written here. If the label is defined in the data section, NULL will be written here.
 * If the label is expected to be defined in the data section, NULL can be passed for this parameter.
 * If this parameter is NULL and the label is defined in the code section, this function will fail.
 * \param destination_data The pointer to the DataBlock will be written here. If the label is defined in the code section, NULL will be written here.
 * If the label is expected to be defined in the code section, NULL can be passed for this parameter.
 * If this parameter is NULL and the label is defined in the data section, this function will fail.
 * \param label Label string that should be searched in the LabelTree.
 * \param labeltree LabelTree that should be searched.
 * \return One if the Block has been found. Zero otherwise.
 */
int get_label(CodeBlock** destination_code, DataBlock** destination_data, const char* label, LabelTree* labeltree);

/**
 * Finds out if a character can be placed at a specific position in the Malbolge program code or not.
 *
 * \param position Desired position for the character.
 * \param character Desired character.
 * \return If the character value cannot be placed into the Malbolge program code directly and has to be generated during Malbolge program execution, this funciton returns 0.
 *         Otherwise it returns a non-zero value.
 */
int is_valid_initial_character(int position, char character);

/**
 * Gets a single xlat2 cycle and its position. Computates whether the given xlat2 cycle is possible at the given position or not.
 * If start_symbol is not null and the gevien xlat2 cycle exists, the first ASCII value of the xlat2 cycle will be written to *start_symbol.
 *
 * \param cycle Xlat2 cycle that should be checked for existence.
 * \param position Desired position of the xlat2 cycle at the Malbolge virtual machine memory.
 * \param start_symbol An ASCII character that will result in the given xlat2 cycle will be written here if such a cycle exists.
 *                     If this pointer is set to NULL, the ASCII character won't be written.
 * \return If the given xlat2 cycle exists at the given position, this funciton returns a non-zero value. Otherwise zero is returned.
 */
int is_xlatcycle_existant(XlatCycle* cycle, int position, char* start_symbol);

/* debug-informations */
void print_labeltree(FILE* destination, LabelTree* labeltree);
void print_source_positions(FILE* destination, MemoryCell memory[C2+1]);

#endif

