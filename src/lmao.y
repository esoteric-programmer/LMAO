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

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "typedefs.h"
#include "malbolge.h"


#ifndef YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
#define YYLTYPE_IS_DECLARED
#endif

#define COPY_CODE_POSITION(to, from_start, from_end) {(to).first_line = (from_start).first_line;(to).first_column = (from_start).first_column;(to).last_line = (from_end).last_line;(to).last_column = (from_end).last_column;};


/**
 * Print error.
 */
int yyerror(const char *s);

/* avoid implicit declaration warning */
int yylex(void);

/**
 * Inserts a label into the label tree.
 */
int insert_label(unsigned char destination_type, void* destination, const char* label, LabelTree** labeltree, YYLTYPE* code_position);

/**
 * Adds a data block to the list of data blocks.
 */
void add_datablock(DataBlock* block, DataBlocks* datablocks);

/**
 * Adds a code block to the list of code blocks.
 */
void add_codeblock(CodeBlock* block, CodeBlocks* codeblocks);

/**
 * Counts the number of escaped characters ('\' followed by arbitrary character) in a given string.
 */
int count_escaped(const char* s);

YYLTYPE yylloc;
%}

%define parse.lac full
// TODO bison version check
%error_verbose
//%define parse.error verbose

/* Symbols. */
%union
{
	const char* s_val;
	unsigned int i_val;
	unsigned char c_val;
	unsigned char prefix;

	XlatCycle* xlat;
	DataAtom* dataatom;
	DataCell* datacell;

	DataBlock* datablock;
	CodeBlock* codeblock;
};

%token <s_val> IDENTIFIER
%token <s_val> LABEL
%token EMPTYLINE
%token CSEC ".CODE"
%token DSEC ".DATA"
%token RNOP
%token SLASH
%token OFFSET ".OFFSET or @"
%token DONTCARE
%token NOTUSED
%token BRACKETLEFT
%token BRACKETRIGHT
%token COMMA
%token <s_val> U_PREFIXED_IDENTIFIER
%token <s_val> R_PREFIXED_IDENTIFIER
%token <s_val> STRING
%token <i_val> CONSTANT
%token <c_val> COMMAND
%token <c_val> PLUSMINUS "+ or -"
%token <c_val> MULDIV "* or /"
%token <c_val> SHIFT
%token CRAZY

%type <xlat> XlatCycle
%type <xlat> Codeexpression
%type <datacell> Dataexpression
%type <i_val> Offset
%type <datablock> Dataexpressions
%type <datablock> Datablock
%type <codeblock> Codeexpressions
%type <codeblock> Codeblock
%type <dataatom> Dataatom
%type <datacell> Product
%type <datacell> Crazied
%type <datacell> Sum

%start Start

%%

Start:			EMPTYLINE Start | Program;

Program:		| Code Program
				| Data Program;

Code:			CSEC Codeblocks;

Codeblocks:		Codeblock { if ($1 != 0) add_codeblock($1, &codeblocks);}
				| Codeblocks EMPTYLINE Codeblock { if ($3 != 0) add_codeblock($3, &codeblocks);};

Offset:			OFFSET CONSTANT IgnoreEmptylines {$$=$2;};

IgnoreEmptylines:	EMPTYLINE IgnoreEmptylines |

Codeblock:		{$$ = 0;}
				| Offset LABEL Codeexpressions { if (!insert_label(0, $3, $2, &labeltree, &@2)) return 1; $$=$3; if ($$ != 0) $$->offset = $1;}
				| LABEL Codeexpressions { if (!insert_label(0, $2, $1, &labeltree, &@1)) return 1; $$=$2; };

Codeexpressions:	{ $$ = 0;}
				| LABEL Codeexpressions { if (!insert_label(0, $2, $1, &labeltree, &@1)) return 1; $$=$2; }
				| Codeexpression Codeexpressions { $$=(CodeBlock*)malloc(sizeof(CodeBlock)); $$->next = $2; $$->virtual_block = 0; if ($2 != 0) $2->prev = $$; $$->prev=0; $$->offset=-1; $$->command=$1; if ($2 == 0) $$->num_of_blocks = 1; else $$->num_of_blocks = $2->num_of_blocks+1; COPY_CODE_POSITION($$->code_position, @1, @1); };

Codeexpression:		RNOP {$$=(XlatCycle*)malloc(sizeof(XlatCycle)); $$->cmd = MALBOLGE_COMMAND_NOP; $$->next = $$; /*RNOP: link to itself*/ COPY_CODE_POSITION($$->code_position,@1,@1); } //COPY_CODE_POSITION(@$, @1, @1);}
				| XlatCycle {$$=$1; };//COPY_CODE_POSITION(@$, @1, @1);};

XlatCycle:		COMMAND { $$=(XlatCycle*)malloc(sizeof(XlatCycle)); $$->next = 0; $$->cmd = $1; COPY_CODE_POSITION($$->code_position,@1,@1); } //COPY_CODE_POSITION(@$, @1, @1); }
				| COMMAND SLASH XlatCycle { $$=(XlatCycle*)malloc(sizeof(XlatCycle)); $$->next = $3; $$->cmd = $1; COPY_CODE_POSITION($$->code_position,@1,@1); }; //COPY_CODE_POSITION(@$, @1, @3); };

Data:			DSEC Datablocks;

Datablocks:		Datablock {if ($1 != 0) add_datablock($1, &datablocks);}
				| Datablocks EMPTYLINE Datablock {if ($3 != 0) add_datablock($3, &datablocks);};

Datablock:		{ $$ = 0; }
				| Offset LABEL Dataexpressions { if (!insert_label(1, $3, $2, &labeltree, &@2)) return 1; $$=$3; $$->offset = $1; }
				| LABEL Dataexpressions { if (!insert_label(1, $2, $1, &labeltree, &@1)) return 1; $$=$2; };

Dataexpressions:	{ $$ = 0;}
				| LABEL Dataexpressions { if (!insert_label(1, $2, $1, &labeltree, &@1)) return 1; $$=$2; }
				| Dataexpression Dataexpressions { $$=(DataBlock*)malloc(sizeof(DataBlock)); $$->next = $2; if ($2 != 0) $2->prev = $$; $$->prev=0; $$->offset=-1; $$->data=$1; if ($2 == 0) $$->num_of_blocks = 1; else $$->num_of_blocks = $2->num_of_blocks+1; COPY_CODE_POSITION($$->code_position, @1, @1); }
				| STRING Dataexpressions { 
int length = (int)strlen($1)-2;
//COPY_CODE_POSITION(@$,@1,@2);
if (length <= 0){
  $$=$2;
}
else{
  int escaped = count_escaped($1);
  int processed = 0;
  DataBlock* tmp;
  DataBlock* tmp_old=0;
  int i;
  int char_start_column;
  int char_end_column;
  $$=0;
  for (i=0;i<length;i++){
    tmp=(DataBlock*)malloc(sizeof(DataBlock));
    if ($$==0)
      $$=tmp;
    tmp->prev = tmp_old;
    if (tmp_old != 0)
      tmp_old->next = tmp;
    tmp->next = $2;
    if ($2 != 0)
      $2->prev = tmp;
    char_start_column = @1.first_column+i+1;
    tmp->offset=-1;
    tmp->data=(DataCell*)malloc(sizeof(DataCell));
    tmp->data->_operator = DATACELL_OPERATOR_LEAF_ELEMENT;
    tmp->data->left_element = 0;
    tmp->data->right_element = 0;
    tmp->data->leaf_element = (DataAtom*)malloc(sizeof(DataAtom));
    tmp->data->leaf_element->destination_label = 0;
    tmp->data->leaf_element->operand_label = 0;
    if ($1[i+1] == '\\'){
      i++;
      tmp->data->leaf_element->number = (int)((unsigned char)$1[i+1]);
      switch ($1[i+1]){
        case 'n':
          tmp->data->leaf_element->number = (int)((unsigned char)'\n');
          break;
        case 'r':
          tmp->data->leaf_element->number = (int)((unsigned char)'\r');
          break;
        case 't':
          tmp->data->leaf_element->number = (int)((unsigned char)'\t');
          break;
        case '0':
          tmp->data->leaf_element->number = 0;
          break;
        default:
          break;
      }
    }else
      tmp->data->leaf_element->number = (int)((unsigned char)$1[i+1]);
    if ($2 == 0)
      tmp->num_of_blocks = (length-escaped-processed);
    else
      tmp->num_of_blocks = $2->num_of_blocks+(length-escaped-processed);
    tmp_old = tmp; processed++;
    char_end_column = @1.first_column+i+1;
    COPY_CODE_POSITION(tmp->data->leaf_element->code_position,@1,@1);
    tmp->data->leaf_element->code_position.first_column = char_start_column;
    tmp->data->leaf_element->code_position.last_column = char_end_column;
    COPY_CODE_POSITION(tmp->code_position, tmp->data->leaf_element->code_position, tmp->data->leaf_element->code_position);
  }
} }
				| STRING COMMA Dataexpression Dataexpressions { 
int length = (int)strlen($1)-2;
//COPY_CODE_POSITION(@$,@1,@4);
if (length <= 0)
  $$ = $4;
else {
  int escaped = count_escaped($1);
  int processed = 0;
  DataBlock* tmp;
  DataBlock* tmp_old=0;
  int i;
  $$=0;
  for (i=0;i<length*2-1;i++){
    int char_start_column=0;
    int char_end_column=0;
    int char_start_line=0;
    int char_end_line=0;
    tmp=(DataBlock*)malloc(sizeof(DataBlock));
    if ($$==0)
      $$=tmp;
    tmp->prev = tmp_old;
    if (tmp_old != 0)
      tmp_old->next = tmp;
    tmp->next = $4;
    if ($4 != 0)
      $4->prev = tmp;
    tmp->offset=-1;
    tmp->data=(DataCell*)malloc(sizeof(DataCell));
    tmp->data->_operator = DATACELL_OPERATOR_LEAF_ELEMENT;
    tmp->data->left_element = 0;
    tmp->data->right_element = 0;
    tmp->data->leaf_element = 0;
    if (i%2==0){
      char_start_column = @1.first_column+i/2+1;
      char_start_line = @1.first_line;
      char_end_line = @1.first_line;
      tmp->data->leaf_element = (DataAtom*)malloc(sizeof(DataAtom));
      tmp->data->leaf_element->destination_label = 0;
      tmp->data->leaf_element->operand_label = 0;
      tmp->data->leaf_element->destination_label = 0;
      tmp->data->leaf_element->operand_label = 0;
      if ($1[i/2+1] == '\\'){
        i+=2;
        tmp->data->leaf_element->number = (int)((unsigned char)$1[i/2+1]);
        switch ($1[i/2+1]){
          case 'n':
            tmp->data->leaf_element->number = (int)((unsigned char)'\n');
            break;
          case 'r':
            tmp->data->leaf_element->number = (int)((unsigned char)'\r');
            break;
          case 't':
            tmp->data->leaf_element->number = (int)((unsigned char)'\t');
            break;
          case '0':
            tmp->data->leaf_element->number = 0;
            break;
          default:
            break;
        }
      }else
        tmp->data->leaf_element->number = (int)((unsigned char)$1[i/2+1]);
        char_end_column = @1.first_column+i/2+1;
    }else{
      memcpy(tmp->data,$3,sizeof(DataCell));
      char_start_column = @3.first_column;
      char_start_line = @3.first_line;
      char_end_line = @3.last_line;
      char_end_column = @3.last_column;
    }
    if ($4 == 0)
      tmp->num_of_blocks = ((length-escaped)*2-1-processed);
    else
      tmp->num_of_blocks = $4->num_of_blocks+((length-escaped)*2-1-processed);
    tmp_old = tmp;
    processed++;
    tmp->data->leaf_element->code_position.first_line = char_start_line;
    tmp->data->leaf_element->code_position.first_column = char_start_column;
    tmp->data->leaf_element->code_position.last_line = char_end_line;
    tmp->data->leaf_element->code_position.last_column = char_end_column;
    COPY_CODE_POSITION(tmp->code_position, tmp->data->leaf_element->code_position, tmp->data->leaf_element->code_position);
  }
} };

Dataatom:		CONSTANT { $$=(DataAtom*)malloc(sizeof(DataAtom)); $$->destination_label = 0; $$->number = $1; $$->operand_label = 0; $$->code_position.first_column = @1.first_column; COPY_CODE_POSITION($$->code_position,@1,@1);}
				| IDENTIFIER { $$=(DataAtom*)malloc(sizeof(DataAtom)); $$->destination_label = $1; $$->number = 0; $$->operand_label = 0; COPY_CODE_POSITION($$->code_position,@1,@1);}
				| R_PREFIXED_IDENTIFIER { $$=(DataAtom*)malloc(sizeof(DataAtom)); $$->destination_label = $1; $$->number = 1; $$->operand_label = 0; COPY_CODE_POSITION($$->code_position,@1,@1);}
				| U_PREFIXED_IDENTIFIER IDENTIFIER { $$=(DataAtom*)malloc(sizeof(DataAtom)); $$->destination_label = $1; $$->number = 0; $$->operand_label = $2; COPY_CODE_POSITION($$->code_position,@1,@2);};





Dataexpression: 	Dataexpression SHIFT Crazied { $$=(DataCell*)malloc(sizeof(DataCell)); $$->leaf_element = 0; $$->_operator = ($2=='>'?DATACELL_OPERATOR_ROTATE_R:DATACELL_OPERATOR_ROTATE_L); $$->left_element = $1; $$->right_element = $3; }
				| Crazied { $$=$1;}
				| DONTCARE { $$=(DataCell*)malloc(sizeof(DataCell)); $$->leaf_element = 0; $$->_operator = DATACELL_OPERATOR_DONTCARE; $$->left_element = 0; $$->right_element = 0;}
				| NOTUSED { $$=(DataCell*)malloc(sizeof(DataCell)); $$->leaf_element = 0; $$->_operator = DATACELL_OPERATOR_NOT_USED; $$->left_element = 0; $$->right_element = 0;};


Crazied: 		Crazied CRAZY Sum { $$=(DataCell*)malloc(sizeof(DataCell)); $$->leaf_element = 0; $$->_operator = DATACELL_OPERATOR_CRAZY; $$->left_element = $1; $$->right_element = $3; }
				| Sum { $$ = $1; };


Sum: 			Sum PLUSMINUS Product { $$=(DataCell*)malloc(sizeof(DataCell)); $$->leaf_element = 0; $$->_operator = ($2=='+'?DATACELL_OPERATOR_PLUS:DATACELL_OPERATOR_MINUS); $$->left_element = $1; $$->right_element = $3; }
				| Product { $$ = $1; };


Product: 		Product MULDIV Dataatom { $$=(DataCell*)malloc(sizeof(DataCell)); $$->leaf_element = 0; $$->_operator = ($2=='*'?DATACELL_OPERATOR_TIMES:DATACELL_OPERATOR_DIVIDE); $$->left_element = $1; $$->right_element = (DataCell*)malloc(sizeof(DataCell)); $$->right_element->leaf_element = $3; $$->right_element->_operator = DATACELL_OPERATOR_LEAF_ELEMENT; $$->right_element->left_element = 0; $$->right_element->right_element = 0; }
				| Product MULDIV BRACKETLEFT Dataexpression BRACKETRIGHT { $$=(DataCell*)malloc(sizeof(DataCell)); $$->leaf_element = 0; $$->_operator = ($2=='*'?DATACELL_OPERATOR_TIMES:DATACELL_OPERATOR_DIVIDE); $$->left_element = $1; $$->right_element = $4; }
				| Dataatom { $$=(DataCell*)malloc(sizeof(DataCell)); $$->leaf_element = $1; $$->_operator = DATACELL_OPERATOR_LEAF_ELEMENT; $$->left_element = 0; $$->right_element = 0; }
				| BRACKETLEFT Dataexpression BRACKETRIGHT { $$ = $2; };








%%

int yyerror(const char *s) {
	fprintf(stderr,"Error: %s at line %d column %d.\n",s, yylloc.first_line, yylloc.first_column);
	return 0;
}

void add_datablock(DataBlock* block, DataBlocks* datablocks){
	if (datablocks == 0)
		return; /* ERROR! */
	datablocks->datafield = (DataBlock**)realloc(datablocks->datafield, (datablocks->size+1)*sizeof(DataBlock*));
	datablocks->datafield[datablocks->size] = block;
	datablocks->size++;
}

void add_codeblock(CodeBlock* block, CodeBlocks* codeblocks){
	if (codeblocks == 0)
		return; /* ERROR! */
	codeblocks->codefield = (CodeBlock**)realloc(codeblocks->codefield, (codeblocks->size+1)*sizeof(CodeBlock*));
	codeblocks->codefield[codeblocks->size] = block;
	codeblocks->size++;
}

int insert_label(unsigned char destination_type, void* destination, const char* label, LabelTree** labeltree, YYLTYPE* code_position){
	if (labeltree == 0) {
		yyerror("Internal error: Pointer to label tree is NULL");
		return 0; /* ERROR */
	}

	if (label == 0) {
		yyerror("Internal error: Label is NULL");
		return 0; /* ERROR */
	}

	if (destination == 0) {
		yyerror("Label definition must be followed by a data or code word");
		return 0; /* ERROR */
	}

	while(*labeltree != 0) {
		/* select sub tree */
		int cmp = strncmp(label,(*labeltree)->label,101);
		if (cmp > 0){
			labeltree = &((*labeltree)->left);
		}else if (cmp < 0){
			labeltree = &((*labeltree)->right);
		}else{
			char error_msg[140];
			if (strlen(label)<=100)
				sprintf(error_msg,"Label %s is defined multiple times",label);
			else
				sprintf(error_msg,"Label is defined multiple times");
			yyerror(error_msg);
			return 0;
		}
	}

	*labeltree = (LabelTree*)malloc(sizeof(LabelTree));
	(*labeltree)->left = 0;
	(*labeltree)->right = 0;
	(*labeltree)->label = label;
	COPY_CODE_POSITION((*labeltree)->code_position, (*code_position), (*code_position));
	if (destination_type == 0) {
		/* Code Block */
		(*labeltree)->destination_code = (CodeBlock*)destination;
		(*labeltree)->destination_data = NULL;
	}else{
		/* Data Block */
		(*labeltree)->destination_code = NULL;
		(*labeltree)->destination_data = (DataBlock*)destination;
	}
	return 1;
}

int count_escaped(const char* s){
	int len = (int)strlen(s);
	int i;
	int ret=0;
	for (i=0;i<len;i++){
		if (s[i] == '\\'){
			i++;
			ret++;
		} 
	}
	return ret;
}

