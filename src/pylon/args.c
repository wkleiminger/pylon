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

#include "args.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int isValid(const Argument* arg);
Argument* lookupByPrefix(Argument args[], const char* prefix);

int args_parse(Argument args[], int argc, char** argv)
{	
	int ok = 1;

	Argument* orderedArg = args;
	for (int i = 1; i < argc; i++) {

		// Find table entry matching prefix
		Argument* arg = lookupByPrefix(args, argv[i]);
		if (arg) {
			// Handle prefixed argument

			if ((arg->type &~ OPTIONAL) == ARG_FLAG) {
				// Flag is set
				arg->value = argv[i];
			} else {

				// Check if value specified
				if ((i+1 >= argc) || lookupByPrefix(args, argv[i+1])) {
					printf("Missing value for option '%s' after prefix %s\n", arg->name, arg->prefix);
					ok = 0;
					continue;
				}

				i++; // Move from prefix to actual value

				arg->value = argv[i];
			}
		} else {
			// Handle ordered argument

			// Skip unordered arguments in table
			while (orderedArg->prefix) ++orderedArg;

			// Check if entry exists
			if (!orderedArg->name) {
				printf("Unexpected argument: '%s'\n", argv[i]);
				ok = 0;
				continue;
			}

			orderedArg->value = argv[i];
			
			// Ordered argument specified
			++orderedArg;
		}
		
	}
		
	// Check if all arguments are valid
	for (const Argument* it = args; it->name; it++) {
		if (!it->value && !(it->type & OPTIONAL)) {
			printf("Missing argument: '%s'\n", it->name);
			ok = 0;
		} else if (!isValid(it)) {
			printf("Invalid value for argument '%s': %s\n", it->name, it->value);
			ok = 0;
		}
	}

	return ok;
}



void args_printUsage(const Argument args[], const char* cmd)
{
	printf("Usage: %s", cmd);
	for (const Argument* it = args; it->name; it++) {

		int brackets = (it->type & OPTIONAL) || (it->type == ARG_FLAG);
		int flag     = (it->type &~ OPTIONAL) == ARG_FLAG;

		printf(" ");
		if (brackets) printf("[");
		if (it->prefix) printf("%s", it->prefix);
		if (it->prefix && !flag) printf(" "); 
		if (!flag) printf("%s", it->name);
		if (brackets) printf("]");
	}
	printf("\n");
}

void args_printInfo(const Argument args[])
{
	int nw = 0;
	int vw = 0;
	for (const Argument* it = args; it->name; it++) {
		int len = strlen(it->name);
		if (nw < len) nw = len;
		len = it->value ? strlen(it->value) : 0;
		if (vw < len) vw = len;
	}
	for (const Argument* it = args; it->name; it++) {
		printf("%-*s : %-*s", nw, it->name, vw, it->value ? it->value : "");
		if (it->description) {
			printf(" -- %s", it->description);
		}
		printf("\n");
	}
}

char* args_value(const Argument args[], const char* name)
{
	for (const Argument* it = args; it->name; it++) {
		if (strcmp(it->name, name) == 0) {
			return it->value;
		}
	}
	return NULL;
}

Argument* lookupByPrefix(Argument args[], const char* prefix)
{
	for (Argument* it = args; it->name; it++) {
		if (it->prefix && strcmp(it->prefix, prefix) == 0) {
			return it;
		}
	}
	return NULL;
}

int isValid(const Argument* arg)
{
	if (!arg->value) {
		return arg->type & OPTIONAL;
	}
	char* endptr = NULL;
	switch (arg->type &~ OPTIONAL) {
	case ARG_STRING:
		return 1;
	case ARG_INT:
		strtol(arg->value, &endptr, 0);
		return endptr != arg->value;
	case ARG_FLOAT:
		strtod(arg->value, &endptr);
		return endptr != arg->value;
	case ARG_FLAG:
		return 1;
	default:
		return 0;
	}
}

