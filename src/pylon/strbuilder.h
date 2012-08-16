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

#ifndef __STRBUILDER_H
#define __STRBUILDER_H

#include <stdlib.h>


////////////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////////////

// Opaque type
typedef struct StringBuilder_s StringBuilder;


////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

// Creates a new string builder
StringBuilder* strbuilder_create(void);

// Releases the specified string builder including the constructed string
void strbuilder_free(StringBuilder* sb);

// Appends the specified format string to the current string
int strbuilder_printf(StringBuilder* sb, const char* format, ...);

// Returns a pointer to the current string
// Note that this pointer can change when appending more strings or
// invoking pack() so make a copy if necessary
char* strbuilder_str(StringBuilder* sb);

// Returns a copy of the current string
// Caller needs to release memory using free()
char* strbuilder_copy(StringBuilder* sb);

// Returns the length of the current string
size_t strbuilder_length(StringBuilder* sb);

// Resets the string builder so that a new string can be constructed
// Does not release memory
void strbuilder_reset(StringBuilder* sb);

// Releases memory that was reserved but is not used by the current string
void strbuilder_pack(StringBuilder* sb);


#endif // __STRBUILDER_H

