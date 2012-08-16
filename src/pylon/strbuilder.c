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
  Module    : strbuilder
  Used by   : smlogger
  Purpose   : Provides the functionality to build a string of variable length
              using subsequent printf calls.
  
  Version   : 1.0
  Date      : 06.05.2012
  Author    : Daniel Pauli
\******************************************************************************/

#include "strbuilder.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "common.h"

////////////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////////////

// Buffer size for the first realloc()
#define INITIAL_CAPACITY 32

////////////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////////////

// State of the string builder
struct StringBuilder_s {

	// Pointer to the string
	char* str;
	
	// Length of the string
	size_t len;

	// Size of the buffer backing the string
	size_t capacity;
};


////////////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////////////

StringBuilder* strbuilder_create(void)
{
	StringBuilder* sb = malloc(sizeof(StringBuilder));
	sb->str = NULL;
	sb->len = 0;
	sb->capacity = 0;
	return sb;
}

void strbuilder_free(StringBuilder* sb)
{
	if (sb) {
		free(sb->str);
		free(sb);
	}
}

int strbuilder_printf(StringBuilder* sb, const char* format, ...)
{
	// Initial guess for the number of bytes to print
	int writeSize = strlen(format);
	
	// Repeat if buffer was to small for printf
	int done = 0;
	while (!done) {
	
		// Check if buffer needs to grow
		size_t minCapacity = sb->len + writeSize + 1; // include trailing '\0'
		if (sb->capacity < minCapacity) {

			// Compute new capacity as a power of two
			size_t newCapacity = sb->capacity;
			while (newCapacity < minCapacity) {
				newCapacity = newCapacity ? newCapacity*2 : INITIAL_CAPACITY;
			} 

			// Grow buffer
			char* buf = realloc(sb->str, newCapacity);
			if (!buf) {
				LOG(0, "Failed to grow buffer: %s\n", strerror(errno));
				return -1;
			}
		
			// Update state
			sb->str = buf;
			sb->capacity = newCapacity;
		} 
		
		// Compute size left in buffer for snprintf
		// Do not include trailing '\0' - it will be overwritten
		int size = sb->capacity - sb->len;
		
		// Perform printf
		va_list args;
		va_start(args, format);
		writeSize = vsnprintf(sb->str + sb->len, size, format, args);
		va_end(args);
		
		// Check if all bytes (excluding '\0') were written
		done = writeSize < size;
	}
	
	// Success
	sb->len += writeSize;
	return writeSize;
}

char* strbuilder_copy(StringBuilder* sb)
{
	if (!sb->str) {
		return NULL;
	}

	char* str = malloc(sb->len+1);
	if (!str) {
		LOG(0, "malloc failed: %s\n", strerror(errno));
		return NULL;
	}
	
	memcpy(str, sb->str, sb->len+1);
	return str;
}

char* strbuilder_str(StringBuilder* sb)
{
	return sb->str;
}

size_t strbuilder_length(StringBuilder* sb)
{
	return sb->len;
}

void strbuilder_reset(StringBuilder* sb)
{
	sb->len = 0;
}

void strbuilder_pack(StringBuilder* sb)
{
	// Free zero-length strings
	if (sb->len == 0) {
		free(sb->str);
		sb->str = NULL;
		sb->capacity = 0;
		return;
	}

	// Trim buffer
	char* buf = realloc(sb->str, sb->len+1);
	if (!buf) {
		LOG(1, "Failed to trim buffer: %s\n", strerror(errno));
		return;
	}

	// Update state
	sb->str = buf;
	sb->capacity = sb->len+1;	
}

