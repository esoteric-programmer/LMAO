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

#ifndef MALBOLGEINCLUDED
#define MALBOLGEINCLUDED

/**
 * Trinary constant: 0t0000000000
 */
#define C0 0

/**
 * Trinary constant: 0t1111111111
 */
#define C1 29524

/**
 * Trinary constant: 0t2222222222
 */
#define C2 59048

/**
 * Trinary constant: 0t2222222221
 */
#define C21 (C2-1)

/**
 * Malbolge opcode for Nop command.
 *
 * If executed by the Malbolge interpreter, Nop does not causes the interpreter to do anything.
 */
#define MALBOLGE_COMMAND_NOP 68

/**
 * Malbolge opcode for MovD command.
 *
 * If executed by the Malbolge interpreter, MovD causes the interpreter to set the D register to the value of [D].
 */
#define MALBOLGE_COMMAND_MOVED 40

/**
 * Malbolge opcode for Opr command.
 *
 * If executed by the Malbolge interpreter, Opr causes the interpreter to execute the crazy function with the values of the A register and [D]. The result will be stored in the A register and in [D].
 * \sa crazy(unsigned int, unsigned int)
 */
#define MALBOLGE_COMMAND_OPR 62

/**
 * Malbolge opcode for Jmp command.
 *
 * If executed by the Malbolge interpreter, Jmp causes the interpreter to set the C register to the value of [D].
 */
#define MALBOLGE_COMMAND_JMP 4

/**
 * Malbolge opcode for Rot command.
 *
 * If executed by the Malbolge interpreter, Rot causes the interpreter to rotate [A] tritwise right and copy the result to the A register.
 * \sa rotate_right(unsigned int)
 */
#define MALBOLGE_COMMAND_ROT 39

/**
 * Malbolge opcode for Out command.
 *
 * If executed by the Malbolge interpreter, Out causes the interpreter to write an ASCII character from the A register modulo 256 to stdout.
 */
#define MALBOLGE_COMMAND_OUT 5

/**
 * Malbolge opcode for In command.
 *
 * If executed by the Malbolge interpreter, In causes the interpreter to read an ASCII character from stdin and stores it in the A register.
 * If end-of-file is read, A is set to C2=59048.
 */
#define MALBOLGE_COMMAND_IN 23

/**
 * Malbolge opcode for Hlt command.
 *
 * If executed by the Malbolge interpreter, Hlt causes the interpreter to stop execution and terminate.
 */
#define MALBOLGE_COMMAND_HALT 81

/**
 * xlat2 translation.
 */
#define XLAT2 "5z]&gqtyfr$(we4{WP)H-Zn,[%\\3dL+Q;>U!pJS72FhOA1CB6v^=I_0/8|jsb9m<.TVac`uY*MK'X~xDl}REokN:#?G\"i@"

/**
 * Malbolge crazy operator.
 *
 * \param a TODO
 * \param d TODO
 * \return TODO
 */
unsigned int crazy(unsigned int a, unsigned int d);

/**
 * Tritwise rotation.
 *
 * \param d TODO
 * \return TODO
 */
unsigned int rotate_right(unsigned int d);

#endif

