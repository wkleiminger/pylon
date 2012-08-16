/*******************************************************************************
* Copyright (c) 2012, Institute for Pervasive Computing, ETH Zurich.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
* notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* 3. Neither the name of the Institute nor the names of its contributors
* may be used to endorse or promote products derived from this software
* without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*
* This file is part of the Pylon Smart Metering framework.
*******************************************************************************/

/******************************************************************************\
  Project   : Pylon
  Module    : args
  Used by   : smlogger
  Purpose   : Provides utility functions to parse command line arguments.
  
  Version   : 1.0
  Date      : 25.04.2012
  Author    : Daniel Pauli
\******************************************************************************/

#ifndef __ARGS_H
#define __ARGS_H

////////////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////////////

// Flag marking arguments as optional on the command line
#define OPTIONAL 0x80


////////////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////////////

// Specifies the data type of a command line argument
typedef enum {
	ARG_STRING,
	ARG_FLAG,
	ARG_INT,
	ARG_FLOAT
} ArgType;

// Structure representing a command line argument
// An application using this module typically specifies its supported arguments
// in a static array of type Argument[]
typedef struct Argument_s {
	char* name;          // A string identifying the argument, e.g. timeout
	char* prefix;        // A string preceding the value of the argument, e.g. -t
	char* value;         // Pointer to the string holding the value of the argument
	ArgType type;        // Data type of the argument
	char* description;   // Usage information about the argument
} Argument;


////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

// Parses the command line arguments as passed by main() and returns
// an array holding the values.
// args[] must be an array of type Argument specifying the supported arguments.
// The parsed argument values are then returned in the Argument.value attribute.
int args_parse(Argument args[], int argc, char** argv);

// Returns the value of the argument with the specified name
char* args_value(const Argument args[], const char* name);

// Displays usage information about the program, e.g.
// Usage: ./myprogram [-t timeout]
void args_printUsage(const Argument args[], const char* cmd);

// Displays detailed information about the available arguments
void args_printInfo(const Argument args[]);

#endif // __ARGS_H

