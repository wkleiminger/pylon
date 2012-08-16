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
  Module    : fluksometer
  Used by   : smlogger
  Purpose   : Provides operations to read data from the internal Flukso
              sensor board.
  
  Version   : 1.0
  Date      : 28.04.2012
  Author    : Daniel Pauli
\******************************************************************************/

#ifndef __FLUKSOMETER_H
#define __FLUKSOMETER_H

#include "smartmeter.h"


////////////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////////////

// Default path of the FIFO special file providing the sensor data
#define FLUKSOMETER_DEFAULT_FIFO "/var/run/spid/delta/out"

////////////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////////////

// Callback used to notify about incoming data
typedef void(*fluksometer_cb)(const SmartMeter_Data* m);


////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////////////////////////////////////////////////////////////////////////

// Initializes the fluksometer module
//   fifo     : The path to the FIFO special file prividing the data
//              or NULL for the default path
//   callback : Function to be called upon data received
int fluksometer_init(const char* fifo, fluksometer_cb callback);

// Starts the fluksometer thread in order to perform
// measurements at the specified time interval
int fluksometer_start(void);

// Stops the fluksometer thread
int fluksometer_stop(void);

// Waits for the fluksometer thread to terminate
int fluksometer_join(void);

// Reads a measurement
int fluksometer_measure(SmartMeter_Data* m);


#endif // __FLUKSOMETER_H

