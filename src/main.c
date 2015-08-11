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

/*
Debuginformationen:

Labeltree+Zieladressen
Sourcecode-Verweise für Malbolge-Zellen
Entrypoint bzw. Anzahl der Operationen bis zum Erreichen des Selbigen

Ggf. MD5-Hash / CRC-Hash der Malbolge-Datei sowie ggf. auch der Source-Code-Datei...
Ggf. Filenames von Source-Code und Malbolge-Code...

=> -d-flag erzeugt automatisch .dbg-file, passend zu .mb-file.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "typedefs.h"
#include "initialize.h"
#include "malbolge.h"
#include "main.h"
#include "globals.h"

/**
 * File extension that is added to the input file if no output file name is given as command line argument.
 */
#define MALBOLGE_FILE_EXTENSION "mb"
#define MALBOLGE_DEBUG_FILE_EXTENSION "dbg"


#define COPY_CODE_POSITION(to, from_start, from_end) {(to).first_line = (from_start).first_line;(to).first_column = (from_start).first_column;(to).last_line = (from_end).last_line;(to).last_column = (from_end).last_column;};


LabelTree* labeltree;
DataBlocks datablocks;
CodeBlocks codeblocks;
int debug_mode;

/**
 *  Bison-generated parser function.
 *
 *  \return TODO
 */
int yyparse();

/**
 * Determines if a given Malbolge command will be executed as a NOP.
 *
 * \param command Malbolge command.
 * \return Non-zero if the Malbolge command is equivalent to a NOP instruction. Zero otherwise.
 */
int is_nop(int command);


/**
 * This function checks if R_ prefixes are only used with identifiers that point to the code section.
 * It substitutes U_ prefixes by not-prefixed labels and the corresponding offset adjustments, also.
 *
 * \param datablocks TODO
 * \param codeblocks TODO
 * \param labeltree TODO
 * \return TODO
 */
int hande_U_and_R_prefixes(DataBlocks* datablocks, CodeBlocks* codeblocks, LabelTree* labeltree);

/**
 * Adds a CodeBlock at a relative offset to the memory layout. The usage field of the affected memory cells will be set to code, preinitialized code, or reserved code. This function chooses the lowest possible offset for the CodeBlock.
 * \param block The CodeBlock that should be added.
 * \param memory_layout The memory layout the CodeBlock should be added to.
 * \param possible_positions_mod_94 An array of size 94 indicating valid start positions for the given CodeBlock modulo 94. Valid start positions are indicated by non-zero entries. Invalid start positions are indicated by zero entries.
 * \param must_be_initialized Indicates whether the given CodeBlock must be initialized or can be written directly into the preinitialized memory region. This will be written into the usage field of the affected memory cells.
 * \return A non-zero value is returned in the case that this function succeeds. Otherwise, zero is returned.
 */
int add_codeblock_to_memory_layout(CodeBlock* block, MemoryCell memory_layout[], unsigned char possible_positions_mod_94[], int must_be_initialized);

/**
 * Adds a DataBlock at a relative offset to the memory layout. The usage field of the affected memory cells will be set to data, or reserved data. This function chooses the lowest possible offset for the DataBlock.
 * \param block The DataBlock that should be added.
 * \param memory_layout The memory layout the DataBlock should be added to.
 * \return A non-zero value is returned in the case that this function succeeds. Otherwise, zero is returned.
 */
int add_datablock_to_memory_layout(DataBlock* block, MemoryCell memory_layout[]);

/**
 * \brief Generates final memory layout.
 *
 * Real memory offsets are calculated from the relative memory offsets for the CodeBlocks and DataBlocks.
 * Blocks with fixed offset are also added an stored into a single MemoryCell array.
 *
 * \param fixedoffset Memory layout containing DataBlock and CodeBlock elements with fixed offset. These blocks will keep their positions in the final memory layout.
 * \param toinitial Memory layout with DataBlock and CodeBlock elements at relative adresses. These blocks have to be initialized by the Malbolge initialization code that will be generated later. These memory cells will be positioned behind the generated Malbolge program in the free space area.
 * \param preinitial Memory layout with DataBlock and CodeBlock elements at relative adresses. These blocks can be set directly in the Malbolge code without initialization sequenze. These memory cells will be positioned at the end of the generated Malbolge program.
 * \param memory_layout The final memory layout generated by this function will be written into this array.
 * \param end_of_initialization_code Position of last Malbolge initialization code word. This value is generally unknown but may be guessed to decrease the size of the output file.
 * If this value is too big the space between preinitialized CodeBlocks and DataBlocks will be filled with NOP instructions. If this value is too small the generation of the Malbolge initialization code sequece will fail due to conflicts.
 * Set this parameter to a value of 59049 or higher to put the preinitialized DataBlock and CodeBlock elements backwards as far as possible. This will result in a very big Malbolge program containing many NOP instructions but it should prevent failing of code generation due to conflicts.
 * \param last_preinitialized_cell_offset This value will be set to the last memory cell that must be initialized and indicates the size of the Malbolge code that has to be generated later.
 * \param no_error_printing Disables printing error messages to stderr on failure.
 * \return A non-zero value is returned in the case that this function succeeds. Otherwise, zero is returned.
 */
int put_all_memcells_together(MemoryCell fixedoffset[], MemoryCell toinitial[], MemoryCell preinitial[], MemoryCell memory_layout[], int* last_preinitialized_cell_offset, int end_of_initialization_code, int no_error_printing);

/**
 * Prints a usage message to stdin.
 *
 * \param executable_name Command used to run this program. If set to NULL, "./lmao" will be printed instead.
 */
void print_usage_message(char* executable_name) {
	printf("Usage: %s [options] <input file name>\n",executable_name!=0?executable_name:"./lmao");
	printf("Options:\n");
	printf("  -o <file>           Write output to <file>\n");
	printf("  -f                  Fast mode; big output size\n");
	printf("  -l <number>         Line break in output file every <number> characters\n");
	printf("  -d                  Write debugging information\n");
}

/**
 * Parses command line arguments.
 *
 * \param argc Number of command line arguments.
 * \param argv Array containing argc command line arguments.
 * \param line_length The number of characters per line in the output file will be written here.
 *                    If no valid value is given at the command line line_length will be set to 0.
 * \param fast_mode TODO
 * \param output_filename The name of the output file will be written here. Therefore new memory is allocated and should be freed later.
 *                        If no output file name is given at the command line the file extension of input_file is replaced by MALBOLGE_FILE_EXTENSION.
 * \param input_filename The name of the input file. The pointer is copied from the argv array.
 * \return A non-zero value will be returned in case of success. Otherwise, zero will be returned.
 */
int parse_input_args(int argc, char** argv, int* line_length, int* fast_mode, char** output_filename, const char** input_filename, char** debug_filename) {
	int i;
	int debug_mode = 0;
	if (argc<2 || argv == 0 || line_length == 0 || output_filename == 0 || input_filename == 0 || fast_mode == 0 || debug_filename == 0) {
		return 0;
	}
	*line_length = -1;
	*output_filename = 0;
	*input_filename = 0;
	*fast_mode = 0;
	for (i=1;i<argc;i++) {
		if (argv[i][0] == '-') {
			/* read parameter */
			switch (argv[i][1]) {
				long int tmp;
				case 'l':
					i++;
					if (*line_length != -1) {
						return 0; /* double parameter: -l */
					}
					if (i>=argc) {
						return 0; /* missing argument for parameter: -l */
					}
					tmp = strtol(argv[i], 0, 10);
					if (tmp > C2+1)
						tmp = C2+1;
					if (tmp < 0)
						tmp = 0;
					*line_length = (int)tmp;
					break;
				case 'o':
					i++;
					if (*output_filename != 0) {
						return 0; /* double parameter: -o */
					}
					if (i>=argc) {
						return 0; /* missing argument for parameter: -l */
					}
					*output_filename = (char*)malloc(strlen(argv[i])+1);
					memcpy(*output_filename,argv[i],strlen(argv[i])+1);
					break;
				case 'f':
					if (*fast_mode != 0) {
						return 0; /* double parameter: -l */
					}
					*fast_mode = 1;
					break;
				case 'd':
					if (debug_mode != 0) {
						return 0; /* double parameter: -l */
					}
					debug_mode = 1;
					break;
				default:
					return 0; /* unknown parameter */
			}
		}else{
			/* read input file name */
			if (*input_filename != 0) {
				return 0; /* more than one input file given */
			}
			*input_filename = argv[i];
		}
	}
	if (*input_filename == 0) {
		return 0; /* no input file name given */
	}
	if (*line_length == -1)
		*line_length = 0;
	if (*output_filename == 0) {
		char* file_extension;
		size_t input_file_name_length;
		/* get file extension and overwrite it - or append it. */
		file_extension = strrchr((char*)*input_filename,'.');
		if (file_extension == 0 || strrchr(*input_filename,'\\')>file_extension || strrchr(*input_filename,'/')>file_extension) {
			input_file_name_length = strlen(*input_filename);
		}else{
			if (strcmp(file_extension+1,MALBOLGE_FILE_EXTENSION)==0) {
				input_file_name_length = strlen(*input_filename);
			}else{
				input_file_name_length = file_extension - *input_filename;
			}
		}
		/* add extension MALBOLGE_FILE_EXTENSION to file name. */
		*output_filename = (char*)malloc(input_file_name_length+1+strlen(MALBOLGE_FILE_EXTENSION)+1);
		memcpy(*output_filename,*input_filename,input_file_name_length);
		(*output_filename)[input_file_name_length] = '.';
		memcpy(*output_filename+input_file_name_length+1,MALBOLGE_FILE_EXTENSION,strlen(MALBOLGE_FILE_EXTENSION)+1);
	}
	if (debug_mode) {
		char* file_extension;
		size_t output_file_name_length;
		/* get file extension and overwrite it - or append it. */
		file_extension = strrchr(*output_filename,'.');
		if (file_extension == 0 || strrchr(*output_filename,'\\')>file_extension || strrchr(*output_filename,'/')>file_extension) {
			output_file_name_length = strlen(*output_filename);
		}else{
			if (strcmp(file_extension+1,MALBOLGE_DEBUG_FILE_EXTENSION)==0) {
				output_file_name_length = strlen(*output_filename);
			}else{
				output_file_name_length = file_extension - *output_filename;
			}
		}
		/* add extension MALBOLGE_FILE_EXTENSION to file name. */
		*debug_filename = (char*)malloc(output_file_name_length+1+strlen(MALBOLGE_DEBUG_FILE_EXTENSION)+1);
		memcpy(*debug_filename,*output_filename,output_file_name_length);
		(*debug_filename)[output_file_name_length] = '.';
		memcpy(*debug_filename+output_file_name_length+1,MALBOLGE_DEBUG_FILE_EXTENSION,strlen(MALBOLGE_DEBUG_FILE_EXTENSION)+1);
	}
	return 1; /* success */
}

/**
 * Main routine of LMAO.
 * Parses command line arguments, reads and parses input file, generates Malbolge code from HeLL program, and writes generated Malbolge code to output file.
 *
 * \param argc Number of command line arguments.
 * \param argv Array with command line arguments.
 * \return Zero will be returned in case of success. Otherwise, a non-zero error code will be returned.
 */
int main(int argc, char **argv) {
	int line_length=0;
	int execution_steps_until_entry_point = -1;
	char* output_filename = 0;
	char* debug_filename = 0;
	const char* input_filename = 0;
	/* Store relative memory positions modulo 94. */
	MemoryCell* preinitialized_section = (MemoryCell*)malloc(59049*sizeof(MemoryCell));
	MemoryCell* to_be_initialized_section = (MemoryCell*)malloc(59049*sizeof(MemoryCell));
	MemoryCell* fixed_offsets = (MemoryCell*)malloc(59049*sizeof(MemoryCell));

	/* Save final memory layout here (this will be the same as stored in the dump file). */
	MemoryCell* memory_layout = (MemoryCell*)malloc(59049*sizeof(MemoryCell));

	DataBlock* entrypoint;

	unsigned int i; /* loop variable */
	int tmp; /* temporary variable */
	int last_preinitialized_position;
	char program[C2+2];
	int opcodes[C2+1];
	char smaller_program[59050];
	int size;
	char* use_program;
	int j;
	int initialize_code_size = 0;
	int fast_mode = 0;
	int smaller_program_success;
	debug_mode = 0;
	FILE* outputfile;
	HeLLCodePosition unset_position;

	printf("This is LMAO v0.5.1 (Low-level Malbolge Assembler, Ooh!) by Matthias Ernst.\n");

	if (!parse_input_args(argc, argv, &line_length, &fast_mode, &output_filename, &input_filename, &debug_filename)){
		print_usage_message(argc>0?argv[0]:0);
		return 0;
	}

	if (freopen(input_filename, "r", stdin) == NULL) {
		fprintf(stderr,"Error: Cannot open file %s\n", input_filename);
		return 1;
	}

	labeltree = 0;
	datablocks.datafield = 0;
	datablocks.size = 0;
	codeblocks.codefield = 0;
	codeblocks.size = 0;
	entrypoint = 0;
	
	if (debug_filename != 0) {
		debug_mode = 1;
	}

	/* Parse input; exit on error. */
	tmp = yyparse();
	if (tmp) {
		return tmp;
 	}

	/* search entry point */
	if (!get_label(NULL, &entrypoint, "ENTRY", labeltree)){
		fprintf(stderr,"Error: Cannot find entry point (label ENTRY) in data section.\n");
		return 1;
	}
	if (entrypoint == 0) {
		fprintf(stderr,"Error: Internal error while looking for entry point.\n");
		return 1;
	}


	if (!hande_U_and_R_prefixes(&datablocks, &codeblocks, labeltree)) {
		/* TODO: print error message?? */
		return 1;
	}

	unset_position.first_line = -1;
	unset_position.first_column = -1;
	unset_position.last_line = -1;
	unset_position.last_column = -1;
	/* initialize memory layout arrays. */
	for (i=0;i<=C2;i++){
		preinitialized_section[i].usage = UNUSED;
		to_be_initialized_section[i].usage = UNUSED;
		fixed_offsets[i].usage = UNUSED;
		COPY_CODE_POSITION(preinitialized_section[i].code_position, unset_position, unset_position);
		COPY_CODE_POSITION(to_be_initialized_section[i].code_position, unset_position, unset_position);
		COPY_CODE_POSITION(fixed_offsets[i].code_position, unset_position, unset_position);
	}

	/* find positions for code blocks.
	 * TODO: maybe source out this for-loop into a function */
	for (i=0;i<codeblocks.size;i++){
		CodeBlock* current_block = codeblocks.codefield[i];
		unsigned char possible_positions[94];
		int current_block_needs_initialization = 0;
		int j;
		int pos = 0;
		for (j=0;j<94;j++)
			if (current_block->offset < 0)
				possible_positions[j] = 2;
			else
				possible_positions[j] = (current_block->offset%94==j?2:0);

		if (codeblocks.codefield[i]->num_of_blocks < 1)
			continue;

		/* treat every xlat-cycle in the current code block */
		while (current_block != 0){
			int positionsLeft = 0;
			int preinitialized_positions_left = 0;
			XlatCycle* current_command = current_block->command;

			/* check for every position that is still possible if the current xlat-cycle is allowed there. remove not-allowed positions.
			 * break with error if no position is allowed any more. */
			for (j=0;j<94;j++){
				char symbol = 0;
				if (possible_positions[j]!=0 && !is_xlatcycle_existant(current_command, (j+pos)%94, &symbol))
					possible_positions[j] = 0;
				if (possible_positions[j]==2 && !is_valid_initial_character((j+pos)%94, symbol))
					possible_positions[j] = 1;
				if (possible_positions[j]!=0) {
					positionsLeft = 1;
				}
				if (possible_positions[j]==2) {
					preinitialized_positions_left = 1;
				}
			}
			pos++;

			if (!positionsLeft){
				fprintf(stderr,"Error: Forced xlat cycle doesn't exist at line %d column %d.\n", current_block->code_position.first_line, current_block->code_position.first_column);
				return 1;
			}

			if (!preinitialized_positions_left){
				current_block_needs_initialization = 1;
			}

			/* get next xlat-cycle in code block */
			current_block = current_block->next;

		}

		if (!current_block_needs_initialization) {
			/* remove all positions that need initialization */
			for (j=0;j<94;j++) {
				if (possible_positions[j]==1) {
					possible_positions[j] = 0;
				}
			}
		}


		/* put current code block into appropriate memory-layout-array! */
		if (!add_codeblock_to_memory_layout(codeblocks.codefield[i], (codeblocks.codefield[i]->offset>=0?fixed_offsets:(current_block_needs_initialization?to_be_initialized_section:preinitialized_section)), possible_positions, current_block_needs_initialization)) {
			if (codeblocks.codefield[i]->offset>=0)
				fprintf(stderr,"Error: Overlapping offsets in code section.\n");
			else
				fprintf(stderr,"Error: Code section too big: Exceeds maximum size of Malbolge program.\n");
			return 1;
		}

	}


	/* TODO: maybe source out this for-loop into a function */
	for (i=0;i<datablocks.size;i++){
		if (datablocks.datafield[i]->num_of_blocks < 1)
			continue;
		/* put current code block into appropriate memory-layout-array! */
		if (!add_datablock_to_memory_layout(datablocks.datafield[i], (datablocks.datafield[i]->offset>=0?fixed_offsets:to_be_initialized_section))) {
			if (datablocks.datafield[i]->offset>=0)
				fprintf(stderr,"Error: Overlapping offsets in data section or between code and data section.\n");
			else
				fprintf(stderr,"Error: Code/Data sections too big: Exceeds maximum size of Malbolge program.\n");
			return 1;
		}

	}

	/* put preinitialized_section, to_be_initialized_section and fixed_offsets together... */
	last_preinitialized_position = 0;
	if (!put_all_memcells_together(fixed_offsets, to_be_initialized_section, preinitialized_section, memory_layout, &last_preinitialized_position, 59049, 0)){
		fprintf(stderr,"Error: Not enough memory in virtual Malbolge machine or fixed offsets in reserved area.\n");
		return 1;
	}

	update_offsets(memory_layout);
	srand(time(NULL)); /* seed */
	if (generate_opcodes_from_memory_layout(memory_layout, last_preinitialized_position, opcodes, labeltree, 0)!=0)
		initialize_code_size = generate_malbolge_initialization_code(opcodes, last_preinitialized_position, entrypoint->offset, program, 0, &execution_steps_until_entry_point);
	else
		program[0] = 0;

	if (initialize_code_size > 0){
		initialize_code_size = (initialize_code_size*2)/3; /* start with small programs. */
	}else{
		initialize_code_size = (C2*2)/3; /* it may fail, but start with small programs... */
	}

	/* try to generate a malbolge program with smaller code */
	smaller_program_success = 0;
	while (initialize_code_size<59049){
		execution_steps_until_entry_point = -1;
		if (fast_mode) {
			initialize_code_size = 59049;
		}
		for (j=0;j<=C2;j++){
			memory_layout[i].usage = UNUSED;
			COPY_CODE_POSITION(memory_layout[i].code_position, unset_position, unset_position);
		}
		last_preinitialized_position = 0;
		if (!put_all_memcells_together(fixed_offsets, to_be_initialized_section, preinitialized_section, memory_layout, &last_preinitialized_position, initialize_code_size, 1)){
			initialize_code_size += 32;
			continue;
		}
		update_offsets(memory_layout);
		if (!generate_opcodes_from_memory_layout(memory_layout, last_preinitialized_position, opcodes, labeltree, 1)) {
			initialize_code_size += 32;
			continue;
		}
		if (generate_malbolge_initialization_code(opcodes, last_preinitialized_position, entrypoint->offset, smaller_program, 1, &execution_steps_until_entry_point) < 0){
			initialize_code_size += 32;
			continue;
		}
		smaller_program_success = 1;
		break;
	}

	if (smaller_program_success != 1){
		fprintf(stderr,"Error: Memory usage conflict while generating initial code.\nThe program may be too big or .OFFSET may cause problems.\n");
		return 1;
	}

	use_program = smaller_program; /* (smaller_program_success==1?smaller_program:program); */
	/* Write result to file. */
	size = 0;
	while (use_program[size] != 0)
		size++;
	outputfile = fopen(output_filename, "wt");
	if (outputfile == 0) {
		fprintf(stderr,"Error: Cannot write to file %s\n", output_filename);
		return 1;
	}
	if (line_length > 0 && line_length < size){
		int remaining = size;
		while(remaining>0) {
			fwrite(use_program+size-remaining,1,(remaining>line_length?line_length:remaining),outputfile);
			fwrite("\n",1,1,outputfile);
			remaining-=line_length;
		}
	}else{
		fwrite(use_program,1,size,outputfile);
		fwrite("\n",1,1,outputfile);
	}
	fclose(outputfile);
	printf("Malbolge code written to %s\n", output_filename);



	if (debug_filename != 0) {
		FILE* debugfile = fopen(debug_filename, "wt");
		if (debugfile == 0) {
			fprintf(stderr,"Error: Cannot write debugging information to file %s\n", debug_filename);
			return 1;
		}
		fprintf(debugfile,":LABELS:\n");
		print_labeltree(debugfile, labeltree);
		fprintf(debugfile,":SOURCEPOSITIONS:\n");
		print_source_positions(debugfile, memory_layout);
		fprintf(debugfile,":EXECUTION_STEPS_UNTIL_ENTRY_POINT:\n");
		fprintf(debugfile,"%d\n", execution_steps_until_entry_point);
		fprintf(debugfile,":SOURCE_FILE:\n");
		fprintf(debugfile,"%s\n",input_filename);
		fprintf(debugfile,":MALBOLGE_FILE:\n");
		fprintf(debugfile,"%s\n",output_filename);
		fclose(debugfile);
		printf("Debugging information written to %s\n", debug_filename);
	}

	return 0;
}

int unused_datacell_crash_warning_displayed = 0;
int get_label(CodeBlock** destination_code, DataBlock** destination_data, const char* label, LabelTree* labeltree){

	if (labeltree == 0 || label == 0 || (destination_code == 0 && destination_data == 0))
		return 0; /* ERROR */

	while(labeltree != 0) {
		/* select sub tree */
		int cmp = strncmp(label,labeltree->label,101);
		if (cmp > 0){
			labeltree = labeltree->left;
		}else if (cmp < 0){
			labeltree = labeltree->right;
		}else{
			if ((labeltree->destination_code != 0) && (labeltree->destination_data != 0)) {
				return 0; /* ERROR: entry for code and data section in label tree */
			}
			if (labeltree->destination_code != 0) {
				if (destination_code == 0) {
					return 0; /* ERROR: label to data section expected during function call, but label to code section found. */
				}
				*destination_code = labeltree->destination_code;
				if (destination_data != 0) {
					*destination_data = 0;
				}
			}else if (labeltree->destination_data != 0) {
				if (destination_data == 0) {
					return 0; /* ERROR: label to code section expected during function call, but label to data section found. */
				}
				*destination_data = labeltree->destination_data;
				if (destination_code != 0) {
					*destination_code = 0;
				}
			}else{
				return 0; /* ERROR: found link to code AND data section for this label */
			}

			if (labeltree->destination_data != 0) {
				DataCell* block = labeltree->destination_data->data;
				if (block->_operator == DATACELL_OPERATOR_NOT_USED){
					if (unused_datacell_crash_warning_displayed == 0) {
						unused_datacell_crash_warning_displayed = 1;
						fprintf(stderr,"Warning: The label %s at line %d column %d points to an unused data cell.\n"
								"Labels pointing to unused data cells are not supported yet.\n"
								"This may cause a crash or faulty error messages.\n"
								"Further warnings of this type will be suppressed.\n",
								label, labeltree->code_position.first_line,
								labeltree->code_position.first_column);
					}
				}
			}

			return 1;
		}
	}

	return 0; /* Not found */
}


void print_labeltree(FILE* destination, LabelTree* labeltree) {
	if (labeltree == 0 || destination == 0)
		return;

	if (labeltree->label != 0) {
		if ((labeltree->destination_code != 0) && (labeltree->destination_data != 0)) {
		} else if (labeltree->destination_code != 0) {
			fprintf(destination, "%s: CODE %d\n",labeltree->label,labeltree->destination_code->offset);
		} else if (labeltree->destination_data != 0) {
			fprintf(destination, "%s: DATA %d\n",labeltree->label,labeltree->destination_data->offset);
		}else{
		}
	}
	print_labeltree(destination, labeltree->left);
	print_labeltree(destination, labeltree->right);
}


void print_source_positions(FILE* destination, MemoryCell memory[C2+1]){
	int i;
	if (destination == 0)
		return;
	/* save all offsets. */
	for (i=0;i<C2+1;i++){
		if (memory[i].usage == CODE || memory[i].usage == PREINITIALIZED_CODE) {
			fprintf(destination,"%d: CODE %d:%d - %d:%d\n",i, memory[i].code->code_position.first_line, memory[i].code->code_position.first_column, memory[i].code->code_position.last_line, memory[i].code->code_position.last_column);
		} else if (memory[i].usage == DATA || memory[i].usage == RESERVED_DATA)
			fprintf(destination,"%d: DATA %d:%d - %d:%d\n",i, memory[i].data->code_position.first_line, memory[i].data->code_position.first_column, memory[i].data->code_position.last_line, memory[i].data->code_position.last_column);
	}
}


int is_nop(int command){
	if (command != MALBOLGE_COMMAND_OPR
			&& command != MALBOLGE_COMMAND_MOVED
			&& command != MALBOLGE_COMMAND_ROT
			&& command != MALBOLGE_COMMAND_JMP
			&& command != MALBOLGE_COMMAND_OUT
			&& command != MALBOLGE_COMMAND_IN
			&& command != MALBOLGE_COMMAND_HALT)
		return 1;
	return 0;
}

int is_valid_initial_character(int position, char character){
	int cmd;
	unsigned char u_c = ((unsigned char)character);
	if (position < 0 || position >= C2)
		return 0; /* invalid parameter */
	position %= 94;
	if (u_c < 33 || u_c > 126)
		return 0;
	cmd = (((int)position) + ((int)u_c))%94;
	if (cmd == MALBOLGE_COMMAND_OPR
			|| cmd == MALBOLGE_COMMAND_MOVED
			|| cmd == MALBOLGE_COMMAND_ROT
			|| cmd == MALBOLGE_COMMAND_JMP
			|| cmd == MALBOLGE_COMMAND_OUT
			|| cmd == MALBOLGE_COMMAND_IN
			|| cmd == MALBOLGE_COMMAND_HALT
			|| cmd == MALBOLGE_COMMAND_NOP)
		return 1;
	return 0;
}

int is_xlatcycle_existant(XlatCycle* cycle, int position, char* start_symbol){
	/* find first NON-NOP in cycle, count NOPs until it.
	 * append NOPs before NON-NOP virtually at the end; start with NON-NOP
	 * set character to the NON-OP-character, then check if xlat2 does exactly what it should.
	 *
	 * if an xlat2 cycle is found AND startSymbol is not a NULL pointer, the ASCII character for the cycle will be written to *startSymbol. */

	unsigned int prefixed_nops = 0;
	unsigned char first_non_nop_char_in_cycle;
	unsigned char current_char;
	unsigned int i; /* loop variable */

	char start_sym_tmp = 0;

	if (position < 0 || position >= C2)
		return 0; /* invalid parameter */
	position %= 94;

	if (cycle == 0)
		return 0; /* invalid argument */
	if (position >= 94)
		return 0; /* invalid argument */

	if (cycle->next == 0) {
		/* no xlat cycle given -> non-loop-resistant command. */
		if (start_symbol != 0) {
			*start_symbol = (char)((cycle->cmd+94-position)%94);
			if (*start_symbol < 33)
				*start_symbol += 94;
		}
		return 1;
	}

	while (cycle->cmd == MALBOLGE_COMMAND_NOP && cycle->next != 0 && cycle->next != cycle){
		cycle = cycle->next;
		prefixed_nops++;
	}

	if (cycle->cmd == MALBOLGE_COMMAND_NOP) {
		/* is a RNop -> Lou Scheffer showed that immutable NOPs exists for every position.
		 * one immutable nop for each position is stored in immutable_nops.
		 * but there may be a better NOP that can be initialized easier.
		 * So this may be optimized in the future. */
		if (start_symbol != 0) {
			const char* const immutable_nops = "FFFFFF>><<::FFFFF3FFFFFF***)))FFFFFFF}FFFFxxFFFrroooFFFFFFF**FF**FFFFFFFFFFFFFFFFPPF**LJJFFFFF";
			*start_symbol = immutable_nops[position];
		}
		return 1;
	}

	/* calc character; store it. */
	first_non_nop_char_in_cycle = (unsigned char)((cycle->cmd+94-position)%94);
	if (first_non_nop_char_in_cycle < 33)
		first_non_nop_char_in_cycle += 94;
	current_char = first_non_nop_char_in_cycle;

	/* while cycle->next: xlat; cmp */
	while (cycle->next != 0){
		unsigned char cur_cmd;
		cycle = cycle->next;
		current_char = (unsigned char)(XLAT2[(int)current_char-33]);
		cur_cmd = (unsigned char)((current_char + position)%94);
		/* TEST: cur_cmd == cycle->cmd */
		if (is_nop(cur_cmd) != is_nop(cycle->cmd))
			return 0;
		if (!is_nop(cur_cmd))
			if (cur_cmd != cycle->cmd)
				return 0;
	}

	/* if an xlat2 cycle exists, then this will be the first ASCII character of the cycle: */
	start_sym_tmp = XLAT2[current_char-33];

	/* for prefixed_nops: xlat; test for NOP */
	for (i=0;i<prefixed_nops;i++) {
		unsigned char curCmd;
		current_char = (unsigned char)XLAT2[current_char-33];
		curCmd = (unsigned char)((current_char + position)%94);
		/* TEST: curCmd is a NOP */
		if (!is_nop(curCmd))
			return 0;
	}

	/* compare with calculated character */
	if (first_non_nop_char_in_cycle != (unsigned char)XLAT2[current_char-33])
		return 0; /* different! */

	if (start_symbol != 0)
		*start_symbol = start_sym_tmp;

	/* cycle exists. */
	return 1;
}

int handle_U_and_R_prefix_for_data_atom(DataAtom* data, DataBlock* following_block, LabelTree* labeltree) {
	if (data->destination_label != 0){
		if (data->operand_label != 0 && data->number == 0){
			/* find relative offset of operand_label and replace it by defining an offset. */
			DataBlock* iterator = following_block;
			DataBlock* destination = 0;
			CodeBlock* destination_c = 0;
			int offset_counter = 0;
			if (!get_label(0, &destination, data->operand_label, labeltree)){
				fprintf(stderr,"Error: Cannot find label %s in data section at line %d column %d.\n",data->operand_label, data->code_position.first_line,data->code_position.first_column);
				return 0;
			}
			if (destination == 0) {
				fprintf(stderr,"Error: Internal error while looking for label %s at line %d column %d.\n",data->operand_label, data->code_position.first_line,data->code_position.first_column);
				return 0;
			}
			while (iterator != 0 && iterator != destination) {
				iterator = iterator->next;
				offset_counter--;
			}
			if (iterator == 0){
				fprintf(stderr,"Error: Label %s used by U_ prefixed command is not in same data block at line %d column %d.\n",data->operand_label, data->code_position.first_line,data->code_position.first_column);
				return 0;
			}
			data->number = offset_counter;
			free((void*)data->operand_label);
			data->operand_label = 0;
			/* search data->destination_label and create the needed preceeding NOPs. */
			if (!get_label(&destination_c, 0, data->destination_label, labeltree)){
				fprintf(stderr,"Error: Cannot find label %s in code section at line %d column %d.\n",data->destination_label, data->code_position.first_line,data->code_position.first_column);
				return 0;
			}
			if (destination_c == 0) {
				fprintf(stderr,"Error: Internal error while looking for label %s at line %d column %d.\n",data->destination_label, data->code_position.first_line,data->code_position.first_column);
				return 0;
			}
			while (offset_counter < 0) {
				if (destination_c->prev != 0){
					XlatCycle* cycle;
					destination_c = destination_c->prev;
					/* check destination_c for immutable NOP! */
					cycle = destination_c->command;
					while (cycle->cmd == MALBOLGE_COMMAND_NOP && cycle->next != 0 && cycle->next != cycle){
						cycle = cycle->next;
					}

					if (cycle->cmd != MALBOLGE_COMMAND_NOP) {
						fprintf(stderr,"Error: Cannot construct nop chain for label %s at line %d column %d.\n", data->destination_label, data->code_position.first_line,data->code_position.first_column);
						fprintf(stderr,"Found non-nop command at line %d column %d.\n", cycle->code_position.first_line,cycle->code_position.first_column);
						return 0;
					}
				}else{
					/* create preceding immutable NOP! */
					CodeBlock* code;
					XlatCycle* cycle = (XlatCycle*)malloc(sizeof(XlatCycle));
					cycle->next = cycle;
					cycle->cmd = MALBOLGE_COMMAND_NOP;
					memcpy(&(cycle->code_position), &(destination_c->command->code_position), sizeof(HeLLCodePosition));
					code = (CodeBlock*)malloc(sizeof(CodeBlock));
					code->command = cycle;
					code->prev = 0;
					code->next = destination_c;
					destination_c->prev = code;
					code->num_of_blocks = destination_c->num_of_blocks+1;
					code->offset = (destination_c->offset==-1?-1:destination_c->offset-1);
					memcpy(&(code->code_position), &(cycle->code_position), sizeof(HeLLCodePosition));
					destination_c = code;
				}
				offset_counter++;
			}

		}else if (data->operand_label == 0 && data->number == 1){
			/* R_ prefix: check that the destination of the JMP has a successor! */
			CodeBlock* destination = 0;
			if (!get_label(&destination, 0, data->destination_label, labeltree)){
				fprintf(stderr,"Error: Cannot find label %s in code block at line %d column %d.\n",data->destination_label, data->code_position.first_line,data->code_position.first_column);
				return 0;
			}
			if (destination == 0) {
				fprintf(stderr,"Error: Internal error while looking for label %s at line %d column %d.\n",data->destination_label, data->code_position.first_line,data->code_position.first_column);
				return 0;
			}
			if (destination->next == 0) {
				fprintf(stderr,"Error: Invalid use of R_ prefix for label %s at line %d column %d.\n",data->destination_label, data->code_position.first_line,data->code_position.first_column);
				return 0;
			}
		}else if (data->operand_label == 0 && data->number == 0){
			/* label without prefix => check if label exists */
			CodeBlock* destination_c = 0;
			DataBlock* destination_d = 0;
			if (!get_label(&destination_c, &destination_d, data->destination_label, labeltree)){
				fprintf(stderr,"Error: Cannot find label %s at line %d column %d.\n",data->destination_label, data->code_position.first_line,data->code_position.first_column);
				return 0;
			}
		}else if (data->number > 1){
			/* Cannot occur. -> ERROR! */
			fprintf(stderr,"Internal error.\n");
			return 0;
		}
	}
	return 1;
}

int hande_U_and_R_prefixes_recursive(DataCell* data, DataBlock* following_block, LabelTree* labeltree){
	if (data->_operator == DATACELL_OPERATOR_LEAF_ELEMENT) {
		/* handle leaf element */
		return handle_U_and_R_prefix_for_data_atom(data->leaf_element, following_block, labeltree);
	}
	if (data->left_element != 0)
		if (!hande_U_and_R_prefixes_recursive(data->left_element, following_block, labeltree))
			return 0;

	if (data->right_element != 0)
		if (!hande_U_and_R_prefixes_recursive(data->right_element, following_block, labeltree))
			return 0;
	return 1;
}


int hande_U_and_R_prefixes(DataBlocks* datablocks, CodeBlocks* codeblocks, LabelTree* labeltree){
	unsigned int i;

	if (datablocks == 0 || labeltree == 0)
		return 0;

	/* For all datablocks: Handle U_ prefix and check R_ prefix. */
	for (i=0;i<datablocks->size;i++){
		DataBlock* current_block = datablocks->datafield[i];
		if (datablocks->datafield[i]->num_of_blocks < 1)
			continue;
		/* Search for U_ prefixed DATA words and calculate offset for them */
		while (current_block != 0){
			if (!hande_U_and_R_prefixes_recursive(current_block->data, current_block->next, labeltree))
				return 0;
			current_block = current_block->next;
		}
	}
	/* iterate code section and check if prev pointer of first element is zero. If it is not zero, correct entry in codeblocks. */
	for (i=0;i<codeblocks->size;i++){
		CodeBlock* curblock = codeblocks->codefield[i];
		while (curblock->prev != 0){
			codeblocks->codefield[i] = (curblock = curblock->prev);
		}
	}

	/* successful */
	return 1;
}

int add_codeblock_to_memory_layout(CodeBlock* block, MemoryCell memory_layout[], unsigned char possible_positions_mod_94[], int must_be_initialized){
	if (block == 0 || memory_layout == 0 || possible_positions_mod_94 == 0)
		return 0; /* invalid parameters */
	if (block->offset>=0){
		/* insert reserved space at the front of this block! */
		int offset = (block->offset>0?block->offset-1:C2);
		if (block->offset > C2)
			return 0; /* invalid offset! */
		if (memory_layout[offset].usage != UNUSED)
			return 0; /* already used! */
		memory_layout[offset].usage = RESERVED_CODE;
		COPY_CODE_POSITION(memory_layout[offset].code_position, block->code_position, block->code_position);
		memory_layout[offset].code_position.last_line = memory_layout[offset].code_position.first_line;
		memory_layout[offset].code_position.last_column = memory_layout[offset].code_position.first_column;
		/* insert at given offset! */
		while (block != 0){
			offset++;
			offset %= 59049;
			if (memory_layout[offset].usage != UNUSED)
				return 0; /* already used! */
			memory_layout[offset].usage = (must_be_initialized?CODE:PREINITIALIZED_CODE);
			memory_layout[offset].code = block;
			COPY_CODE_POSITION(memory_layout[offset].code_position, block->code_position, block->code_position);
			/* get next XlatCycle in code block */
			block = block->next;
		}
		return 1; /* successful */
	}else{
		/* insert at first possible position => at first we have to find the first possible position. */
		int try_position = 1; /* don't use position 0, because RESERVED_CODE has to be inserted in front of the first command. */
		int current_position;
		int pre_offset;
		CodeBlock* current_block;
		/* try each starting position. */
		do {
			int matches_here;
			while (try_position < C2 && !possible_positions_mod_94[try_position%94])
				try_position++;
			if (!possible_positions_mod_94[try_position%94])
				return 0; /* no possible position found */
			matches_here = 1;
			current_position = try_position;
			if (memory_layout[current_position>0?current_position-1:C2].usage != UNUSED) {
				try_position++;
				continue; /* doesn't match here (memory cell in front of block cannot be reserved, it's already used) */
			}
			current_block = block;

			while (current_block != 0){
				if (memory_layout[current_position].usage != UNUSED) {
					matches_here = 0;
					break;
				}
				current_position++;
				current_position%=59049;
				current_block = current_block->next;
			}
			if (matches_here)
				break; /* done */
			try_position++;
			if (try_position == C2)
				return 0; /* reached end. */
		}while(1);
		/* try_position now contains a position that is possible.
		 * now reserve memory! */
		current_position = try_position;
		pre_offset = (current_position>0?current_position-1:C2);
		memory_layout[pre_offset].usage = RESERVED_CODE;
		current_block = block;
		COPY_CODE_POSITION(memory_layout[pre_offset].code_position, current_block->code_position, current_block->code_position);
		memory_layout[pre_offset].code_position.last_line = memory_layout[pre_offset].code_position.first_line;
		memory_layout[pre_offset].code_position.last_column = memory_layout[pre_offset].code_position.first_column;
		while (current_block != 0){
			memory_layout[current_position].usage = (must_be_initialized?CODE:PREINITIALIZED_CODE);
			memory_layout[current_position].code = current_block;
			COPY_CODE_POSITION(memory_layout[current_position].code_position, current_block->code_position, current_block->code_position);
			current_position++;
			current_position%=59049;
			current_block = current_block->next;
		}
		/* memory has been reserved. -> done. */
		return 1;
	}
}

int add_datablock_to_memory_layout(DataBlock* block, MemoryCell memory_layout[]){
	if (block == 0 || memory_layout == 0)
		return 0; /* invalid parameters */
	if (block->offset>=0){
		/* insert reserved space at the front of this block! */
		int offset = block->offset;
		/* insert at given offset! */
		while (block != 0){
			if (block->data->_operator != DATACELL_OPERATOR_NOT_USED) {
				if (memory_layout[offset].usage != UNUSED)
					return 0; /* already used! */
				if (block->data->_operator != DATACELL_OPERATOR_DONTCARE)
					memory_layout[offset].usage = DATA;
				else
					memory_layout[offset].usage = RESERVED_DATA;
				memory_layout[offset].data = block;
				COPY_CODE_POSITION(memory_layout[offset].code_position, block->code_position, block->code_position);
			}
				/* get next value in data block */
			block = block->next;
			offset++;
			offset %= 59049;
		}
		return 1; /* successful */
	}else{
		/* insert at first possible position => at first we have to find the first possible position. */
		int try_position = 0;
		int cur_pos;
		DataBlock* cur_block;
		/* try each starting position. */
		do {
			int matches_here = 1;
			int cur_pos = try_position;
			DataBlock* cur_block = block;
			if (try_position >= C2)
				return 0; /* no possible position found */
			while (cur_block != 0){
				if (cur_block->data->_operator != DATACELL_OPERATOR_NOT_USED) {
					if (memory_layout[cur_pos].usage != UNUSED) {
						matches_here = 0;
						break;
					}
				}
				cur_pos++;
				cur_pos%=59049;
				cur_block = cur_block->next;
			}
			if (matches_here)
				break; /* done */
			try_position++;
			if (try_position == C2)
				return 0; /* reached end. */
		}while(1);
		/* try_position now contains a position that is possible.
		 * now reserve memory! */
		cur_pos = try_position;
		cur_block = block;
		while (cur_block != 0){
			if (cur_block->data->_operator != DATACELL_OPERATOR_NOT_USED) {
				if (cur_block->data->_operator != DATACELL_OPERATOR_DONTCARE)
					memory_layout[cur_pos].usage = DATA;
				else
					memory_layout[cur_pos].usage = RESERVED_DATA;
				memory_layout[cur_pos].data = cur_block;
				COPY_CODE_POSITION(memory_layout[cur_pos].code_position, cur_block->code_position, cur_block->code_position);
			}
			cur_pos++;
			cur_pos%=59049;
			cur_block = cur_block->next;
		}
		/* memory has been reserved. -> done. */
		return 1;
	}
}

int put_all_memcells_together(MemoryCell fixedoffset[], MemoryCell toinitial[], MemoryCell preinitial[], MemoryCell memory_layout[], int* last_preinitialized_cell_offset, int end_of_initialization_code, int no_error_printing){
	int last_toinitial=-1;
	int last_preinitial=-1;
	int i;
	int first_fixedoffset_tobeinitialized=C2+1;
	int RQ_at_position;
	int startoffset = C2+1;
	/* get size of used space in memory layout arrays. */
	for (i=0;i<=C2;i++){
		if (toinitial[i].usage != UNUSED)
			last_toinitial = i;
		if (preinitial[i].usage != UNUSED) {
			last_preinitial = i;
		}
		/* copy fixedoffset to memory_layout. */
		memory_layout[i].usage = fixedoffset[i].usage;
		memory_layout[i].data = fixedoffset[i].data;
		memory_layout[i].code = fixedoffset[i].code;
		COPY_CODE_POSITION(memory_layout[i].code_position, fixedoffset[i].code_position, fixedoffset[i].code_position);
		if ((fixedoffset[i].usage == CODE || fixedoffset[i].usage == DATA) && i < first_fixedoffset_tobeinitialized) {
			first_fixedoffset_tobeinitialized = i;
		}
	}
	if (last_toinitial >= 0){
		int tmp = (last_toinitial+94-(C2%94))%94;
		int matches = 0;
		if (tmp == 0)
			tmp=94;
		startoffset = C2-(94-tmp)-last_toinitial;
		/* now: try to match. if it does not: decrement startoffset by 94 and try again. do so until startoffset is not negative. */
		while (startoffset >= 0){
			/* Test for match. If matches: break; */
			for (i=0;i<=last_toinitial;i++){
				if (memory_layout[i+startoffset].usage != UNUSED && toinitial[i].usage != UNUSED)
					break;
			}
			if (i == last_toinitial+1) {
				matches = 1;
				break;
			}
			startoffset -= 94;
		}
		if (!matches){
			return 0;
		}
		/* put toinitial into memory_layout, start at startoffset. */
		for (i=0;i<=last_toinitial;i++){
			if (toinitial[i].usage != UNUSED){
				if (memory_layout[i+startoffset].usage != UNUSED){
					if (!no_error_printing)
						fprintf(stderr,"Error: Internal error while matching memory areas.\n");
					return 0;
				}
				memory_layout[i+startoffset].usage = toinitial[i].usage;
				memory_layout[i+startoffset].data = toinitial[i].data;
				memory_layout[i+startoffset].code = toinitial[i].code;
				COPY_CODE_POSITION(memory_layout[i+startoffset].code_position, toinitial[i].code_position, toinitial[i].code_position);
			}
		}
		if (startoffset < first_fixedoffset_tobeinitialized){
			first_fixedoffset_tobeinitialized = startoffset;
		}
	}

	/* get first offset from fixedoffset that must be initialized: first_fixedoffset_tobeinitialized
	 *
	 * Program should end with "RQ" => crazied cells will have good values for us.
	 * possible addresses (mod 94): 16/17: RQ, 17/18: RQ, 35/36: RQ, 51/52: RQ, 52/53: RQ, 74/75: RQ, 80/81: RQ, 93/0: RQ */
	RQ_at_position = -1;
	for (i=startoffset-2;i>=0;i--){
		if (i%94 == 16 || i%94 == 17 || i%94 == 35 || i%94 == 51 || i%94 == 52 || i%94 == 74 || i%94 == 80 || i%94 == 93) {
			if (memory_layout[i].usage != PREINITIALIZED_CODE && memory_layout[i+1].usage != PREINITIALIZED_CODE) {
				/* OKAY! */
				RQ_at_position = i;
				break;
			}
		}
	}
	if (RQ_at_position < 500) {
		return 0; /* not enough space. => must be wide over 500, but we wont test other values here. */
		/* if it doesn't match we will throw an error at an other position. */
	}
	if (RQ_at_position > end_of_initialization_code)
		RQ_at_position = end_of_initialization_code+1;

	while(RQ_at_position%94 != 16 && RQ_at_position%94 != 17 && RQ_at_position%94 != 35 && RQ_at_position%94 != 51 && RQ_at_position%94 != 52 && RQ_at_position%94 != 74 && RQ_at_position%94 != 80 && RQ_at_position%94 != 93) {
		RQ_at_position++;
	}

	if (last_preinitialized_cell_offset != 0)
		*last_preinitialized_cell_offset = RQ_at_position+1;

	/* "RQ" has to be written automatically at (last_preinitialized_cell_offset-1,last_preinitialized_cell_offset) later. */

	if (last_preinitial >= 0){
		/* try to match this array in front of RQ_at_position. */
		int matches = 0;
		int first_possible_position = RQ_at_position-1-last_preinitial;
		first_possible_position -= (first_possible_position%94);

		while (first_possible_position >= 0 && !matches){
			/* Test for match. If matches: break; */
			for (i=0;i<=last_preinitial;i++){
				if (memory_layout[i+first_possible_position].usage != UNUSED && preinitial[i].usage != UNUSED)
					break;
			}
			if (i == last_preinitial+1) {
				matches = 1;
				break;
			}
			first_possible_position -= 94;
		}
		if (!matches || first_possible_position < 0){
			return 0;
		}

		/* put preinitial into memory_layout, start at startoffset. */
		for (i=0;i<=last_preinitial;i++){
			if (preinitial[i].usage != UNUSED){
				if (memory_layout[i+first_possible_position].usage != UNUSED){
					if (!no_error_printing)
						fprintf(stderr,"Error: Internal error while matching memory areas.\n");
					return 0;
				}
				memory_layout[i+first_possible_position].usage = preinitial[i].usage;
				memory_layout[i+first_possible_position].data = preinitial[i].data;
				memory_layout[i+first_possible_position].code = preinitial[i].code;
				COPY_CODE_POSITION(memory_layout[i+first_possible_position].code_position, preinitial[i].code_position, preinitial[i].code_position);
			}
		}
		
	}
	return 1; /* successful */
}

