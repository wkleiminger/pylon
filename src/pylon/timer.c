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
  Module    : timer
  Used by   : meter
  Purpose   : Provides routines to measure execution time and perform sleep.
  
  Version   : 1.0
  Date      : 06.04.2012
  Author    : Daniel Pauli
\******************************************************************************/

#include "timer.h"

#include <time.h>

#include <string.h>
#include <errno.h>

#include "common.h"

uint64_t startTime = 0;

uint64_t timer_now(void) {

	// Query monotonic clock
	struct timespec ts = {0};
	if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
		LOG(0, "Failed to get timestamp: %s\n", strerror(errno));
		return 0;
	}
	
	// Return timesspec as milliseconds
	return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void timer_start(void)
{
	startTime = timer_now();
}

int timer_stop(void)
{
	int elapsed = timer_now() - startTime; 
	return elapsed;
}

int timer_restart(void)
{
	int elapsed = timer_stop();
	timer_start();
	return elapsed;
}

int timer_sleep(int interval)
{
	struct timespec ts;
	ts.tv_sec = interval / 1000;
	ts.tv_nsec = (interval % 1000) * 1000000;
	while (nanosleep(&ts, &ts) == -1) {
		if (errno != EINTR) return 0; // Failed
	}
	
	return 1; // Success 
}

int timer_barrier(uint64_t* barrier, int interval, int* elapsed)
{
	int ret = 0;

	// Do not sleep for the first time
	if (*barrier > 0) {

		// Check how long to sleep
		*elapsed = timer_now() - *barrier;
		if (*elapsed < interval) {
			ret = timer_sleep(interval - *elapsed);
		}
		
	} else {
		*elapsed = 0;
		ret = 1;
	}
	
	// Update timestamp
	*barrier = timer_now();
	return ret;
}


