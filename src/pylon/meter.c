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
  Module    : meter
  Used by   : smartmeter
  Purpose   : Provides basic operations to sample from an arbitrary sensor
              using a separate thread and a specified sampling rate
  
  Version   : 1.0
  Date      : 05.04.2012
  Author    : Daniel Pauli
\******************************************************************************/

#include "meter.h"

#include <pthread.h>
#include <string.h>
#include <stdint.h>

#include "timer.h"
#include "common.h"

////////////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////////////

struct MeterHandle_s {
	pthread_t thread;     // POSIX handle of the thread
	int interval;         // Time in milliseconds between two measurements
	int running;          // Flag used for thread termination
	meter_proc measure;   // Callback provided by the client module
};


////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION
////////////////////////////////////////////////////////////////////////////////

static void* threadproc(void* arg);



void* threadproc(void* arg)
{
	// Get context
	MeterHandle* handle = (MeterHandle*)arg;
	if (!handle) {
		LOG(0, "No context specified\n");
		return NULL;
	}

	uint64_t barrier = 0;

	// Repeat until meter_stop invoked
	while (handle->running) {
	
		// Ensure that measuring no more than every 'interval' milliseconds
		// if specified
		if (handle->interval >= 0) {
			int elapsed = 0;
			if (!timer_barrier(&barrier, handle->interval, &elapsed)) {
				LOG(2, "Can't keep up with measurement interval %d ms, time elapsed: %d ms\n", 
					handle->interval, elapsed);
			}
		}
		
		// Invoke callback
		handle->measure(handle);
	}

	return NULL;
}


MeterHandle* meter_start(int interval, meter_proc measure)
{
	// Create context
	MeterHandle* handle = malloc(sizeof(MeterHandle));
	if (!handle) {
		LOG(0, "Failed to allocate handle\n");
		return NULL;
	}
	
	// Set parameters
	handle->interval = interval;
	handle->measure  = measure;
	handle->running  = 1; // Enter loop in threadproc
	
	// Create thread
	int error = pthread_create(&handle->thread, NULL, threadproc, handle);
	if (error) {
		LOG(0, "Failed to create thread: %s\n", strerror(error));
		free(handle);
		handle = NULL;
	}
	
	return handle;
}

int meter_stop(MeterHandle* handle)
{
	if (!handle) {
		LOG(0, "No handle specified\n");
		return 0;
	}

	if (!handle->running) {
		LOG(1, "Not running\n");
		return 1;
	}

	// Request threadproc to terminate
	handle->running = 0;
	
	return 1; // Success
}

int meter_join(MeterHandle* handle)
{
	if (!handle) {
		LOG(0, "No handle specified\n");
		return 0;
	}

	// Join thread
	int error = pthread_join(handle->thread, NULL);
	if (error != 0) {
		LOG(0, "Failed to join thread: %s\n", strerror(error));
		return 0;
	}
	
	// Cleanup
	free(handle);
	
	return 1; // Success
}
