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

#ifndef __TIMER_H
#define __TIMER_H

#include <stdint.h>

////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

// Initiates measuring of execution time
// NOTE: A static variable is used to hold the timestamp, so do not used
//       nested calls
void timer_start(void);

// Stops measuring of execution time and returns the elapsed time in 
// milliseconds
int timer_stop(void);

// Returns elapsed time in milliseconds and re-initiates measuring of execution
// time
int timer_restart(void);

// Returns the current POSIX timestamp
uint64_t timer_now(void);

// Sleeps for the specified time interval in milliseconds
int timer_sleep(int interval);

// Ensures that subsequent code is not executed more often than every 'interval'
// milliseconds. 'barrier' is a pointer to an u64 integer variable holding context
// between subsequent calls, 'elapsed' returns the effective time in milliseconds
// since the last call.
int timer_barrier(uint64_t* barrier, int interval, int* elapsed);

#endif // __TIMER_H
