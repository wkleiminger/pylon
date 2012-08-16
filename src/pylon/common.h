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
  Module    : common
  Used by   : All
  Purpose   : Provides general constants and routines used in the project
  
  Version   : 1.0
  Date      : 05.04.2012
  Author    : Daniel Pauli
\******************************************************************************/

#ifndef __COMMON_H
#define __COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

////////////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////////////

#define DEFAULT_SMART_METER_IP     "169.254.59.110"
#define DEFAULT_SMART_METER_PORT   "7259"
#define DEFAULT_POST_URL           "http://n.ethz.ch/~paulid/flukso.php"

// Capacity of the buffer to hold measurements when energy server not reachable
// NOTE: Linux uses an optimistic memory allocation strategy, so
//       out of memory will most probably be detected upon read/write 
//       rather than upon calling malloc
#define MEASUREMENT_BUFFER_SIZE    (60*60*24)

// Warning messages will be logged when the measurement buffer reaches
// this threshold
#define MEASUREMENT_BUFFER_LIMIT   60


////////////////////////////////////////////////////////////////////////////////
// GLOBAL VARIABLES
////////////////////////////////////////////////////////////////////////////////

// Maximum level of messages to log
extern int log_level;


////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

double mean(const double values[], size_t count);
double median(const double values[], size_t count);
double normalize(double value, double min, double max, double avg);

// Macro for convenient logging. Use like printf() with preceding priority value
// Examples:
// LOG(0, "Critical Error\n");
// LOG(1, "Warning: Function returned with code %d\n", ret);
#define LOG(level, ...) if (level <= log_level) log_message(__FILE__, __func__, level, __VA_ARGS__)

// Macro to determine the number of elements in a static array
#define ARRAY_LENGTH(array) (sizeof((array))/sizeof((array)[0]))

void log_message(const char* file, const char* func, int level, const char *format, ...);

int stricmp(const char* s1, const char* s2);

#endif // __COMMON_H
