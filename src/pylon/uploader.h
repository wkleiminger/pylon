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
  Module    : uploader
  Used by   : smlogger
  Purpose   : Provides routines to asynchronously send data to a web service.
  
  Version   : 1.0
  Date      : 06.05.2012
  Author    : Daniel Pauli
\******************************************************************************/

#ifndef __UPLOADER_H
#define __UPLOADER_H

#include <stdlib.h>

// Initializes the module to send data to the web service at the specified url.
// 'token' is an opaque string used to authenticate the measurements. 
// 'queueSize' specifies the maximum number of measurements that can be buffered
// before uploading. 
// 'numThreads' specifies the number of threads used to transmit data concurrently.
int uploader_init(const char* url, const char* token, size_t queueSize, int numThreads);

// Finalizes the module by releasing all associated resources. This especially 
// includes handles for libcurl objects.
void uploader_cleanup(void);

// Inserts the provided data into the upload queue in order to send it 
// asynchronously to the remote web service. The call will therefore always 
// return immediately. 'data' must point to a JSON encoded string containing an 
// arbitrary measurement. 
// NOTE: The buffer must be allocated on the heap via malloc(), because memory
//       is released using free() after the data has been transferred
int uploader_send(const char* data);

// Returns the current number of data items in the upload queue. 
int uploader_queueSize(void);

// Specifies the time interval in milliseconds for an individual upload thread 
// to wait when the queue is empty or after some transient error 
// (e.g. destination unreachable) occurred before retrying.
void uploader_setInterval(int interval);


#endif // __UPLOADER_H

