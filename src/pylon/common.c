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

#include "common.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>

////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION
////////////////////////////////////////////////////////////////////////////////

// Initialize default log level
int log_level = 2;

static pthread_mutex_t m_lock = PTHREAD_MUTEX_INITIALIZER;

void log_message(const char* file, const char* func, int level, const char *format, ...)
{
	// Acquire lock
	int error = pthread_mutex_lock(&m_lock);
	if (error) {
		LOG(0, "Failed to lock mutex: %s\n", strerror(error));
		return;
	}
	
	fprintf(stderr, "[%s:%s]\t(%d) ", file, func, level);
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  
  // Release lock
  pthread_mutex_unlock(&m_lock);
}

int less_than(const void* a, const void* b)
{
	return *(double*)a < *(double*)b;
}

double mean(const double values[], size_t count)
{
	// Check for bordercase
	if (count == 0) {
		return 0.0; // More robust than NaN
	}

	// Just do it
	double sum = 0.0;
	for (int i = 0; i < count; i++) {
		sum += values[i];
	}
	
	return sum / count;
}

double median(const double values[], size_t count)
{
	// Check for bordercase
	if (count == 0) {
		return 0.0; // More robust than NaN
	}

	// Make a copy of the array, as qsort is destructive
	double tmp[count];
	memcpy(tmp, values, sizeof(tmp));

	// Sort values
	qsort(tmp, count, sizeof(double), less_than);
	
	// Pick the one in the middle
	return tmp[count/2];
}

double normalize(double value, double min, double max, double avg)
{
	// Handle bordercases
	if (value <= min) return 0.0;
	if (value >= max) return 1.0;
	
	// Perform linear interpolation separately for below/above average
	if (value < avg) {
		return 0.5 * (avg != min ? (value - min) / (avg - min) : 1);
	} else {
		return 0.5 * (avg != max ? 1 + (value - avg) / (max - avg) : 1);
	}
}

int stricmp(const char* s1, const char* s2)
{
	int diff;
	// Repeat until difference found or end of string reached
	while (!(diff = toupper(*s1) - toupper(*s2)) && *s1) {
		++s1;
		++s2;
	}
	return diff;
}

