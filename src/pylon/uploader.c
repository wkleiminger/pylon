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

#include "uploader.h"

#include <pthread.h>

#include <curl/curl.h>

#include "queue.h"
#include "timer.h"
#include "common.h"

////////////////////////////////////////////////////////////////////////////////
// CONSTANTS
////////////////////////////////////////////////////////////////////////////////

// Timeout in milliseconds for POST requests
#define SEND_TIMEOUT 10000

////////////////////////////////////////////////////////////////////////////////
// STATIC VARIABLES
////////////////////////////////////////////////////////////////////////////////

static Queue* m_queue;
static pthread_t* m_threads;
static const char* m_url;
static const char* m_token;
static int m_interval;
static int m_numThreads;
static int m_running;


////////////////////////////////////////////////////////////////////////////////
// 
////////////////////////////////////////////////////////////////////////////////

// Entry point for threads performing the upload
static void* uploadProc(void* arg);

// Executes a HTTP POST request using CURL
static int performPOST(CURL* curl, const char* data);

// Dummy function to discard the HTTP response body
static size_t nullWriteFunction(char* ptr, size_t size, size_t nmemb, void* userdata);


////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION
////////////////////////////////////////////////////////////////////////////////

int uploader_init(const char* url, const char* token, size_t queueSize, int numThreads)
{
	m_url = url;
	m_token = token;	
	m_numThreads = numThreads;

	uploader_setInterval(1000);

	// Initialize CURL
	// This must be done before calling curl_easy_init from any other thread
	CURLcode code = curl_global_init(CURL_GLOBAL_ALL);
	if (code != CURLE_OK) {
		LOG(0, "Failed to initialize CURL (%s): %s\n", 
			curl_version(), curl_easy_strerror(code));
		return 0;
	}

	// Initialize queue
	m_queue = queue_create(queueSize, sizeof(char*));
	if (!m_queue) {
		LOG(0, "Failed to create upload queue: %s\n", strerror(errno));
		return 0;
	}

	// Create array to hold thread IDs
	m_threads = calloc(numThreads, sizeof(pthread_t));
	if (!m_threads) {
		LOG(0, "Failed to allocate thread array\n");
		queue_free(m_queue);
		return 0;
	}

	// Spawn sender threads
	m_running = 1; // Enter loop in senderProc
	for (int threadNum = 0; threadNum < m_numThreads; threadNum++) {
		
		int error = pthread_create(&m_threads[threadNum], NULL, uploadProc, (void*)threadNum);
		if (error) {
			LOG(1, "Failed to create sender thread %d: %s\n", threadNum, strerror(error));
			m_numThreads = threadNum; // So we can have only that many threads
			break;
		}
	}

	LOG(2, "Sending data to %s with token '%s' using %d threads and queue with capacity %d\n", 
		m_url, m_token, m_numThreads, queueSize);	
	
	return 1; // Success
}

void uploader_cleanup(void)
{
	// Terminate sender threads
	m_running = 0; // Leave loop in senderProc
	for (int threadNum = 0; threadNum < m_numThreads; threadNum++) {
		
		int error = pthread_join(m_threads[threadNum], NULL);
		if (error) {
			LOG(0, "Failed to join sender thread %d: %s\n", threadNum, strerror(error));
		}
	}
	free(m_threads);
	
	// Free queue
	queue_free(m_queue);
	
	// Finalize CURL
	curl_global_cleanup();	
}

int uploader_send(const char* payload)
{
	if (!queue_enqueue(m_queue, &payload)) {
		LOG(0, "Upload queue full\n");
		return 0;
	}
	
	return 1; // Success
}

void* uploadProc(void* arg)
{
	// Get thread number
	int threadNum = (int)arg;
	
	// Do not start all at the same time
	timer_sleep(m_interval * threadNum / m_numThreads);

	// Get handle to CURL (only use one handle in one thread at any time!)
	CURL* curl = curl_easy_init();
	if (!curl) {
		LOG(0, "Failed to initialize curl for thread %d\n", threadNum);
		return NULL;
	}
	
	// Process measurements
	while (m_running) {
		
		// Grab a measurement from queue
		char* data = NULL;
		if (!queue_dequeue(m_queue, &data) || !data) {
			// Nothing to do, so have a break
			timer_sleep(m_interval);
			continue;
		}
		
		// Send measurement to server
		while (!performPOST(curl, data)) {
			// Failed to send measurement
			// Wait a bit and then try again
			timer_sleep(m_interval);
		}
		
		LOG(3, "Thread %d sent measurement successfully\n", threadNum);
		
		// Cleanup
		free(data);
	}
	
	// Cleanup
	curl_easy_cleanup(curl);

	return NULL;
}

static int lastError = 0;

int performPOST(CURL* curl, const char* data)
{
	// Prepare header
	struct curl_slist* headerlist = NULL;
	headerlist = curl_slist_append(headerlist, "Content-Type: application/json");
	
	// Prepare POST request
	curl_easy_setopt(curl, CURLOPT_URL, m_url);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);	
	curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, SEND_TIMEOUT);

	// Discard response body
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &nullWriteFunction);

	// Perform request
	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {

		// Do not flood the log
		if (lastError != res) {
			LOG(1, "Failed to perform POST request: %s\n", curl_easy_strerror(res));
		}
		lastError = res;
		return 0;
	}

	// Cleanup
	curl_slist_free_all(headerlist);

	// Check response code	
	long code;
	res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
	if (res != CURLE_OK) {
		LOG(0, "Failed to get response code: %s\n", curl_easy_strerror(res));
		return 0;
	}
	if ((code != 201 /* CREATED */) && (code != 204 /* NO CONTENT */)) {
		if (lastError != code) {
			LOG(1, "Failed to upload measurement: HTTP response is %ld\n", code);
		}
		lastError = code;
		return 0;
	}
	
	LOG(4, "Measurement sent successfully\n");
	
	if (lastError) {
		LOG(1, "Measurement finally sent\n");
		lastError = 0;
	}
	
	return 1; // Success	
}

size_t nullWriteFunction(char* ptr, size_t size, size_t nmemb, void* userdata)
{
	// Do nothing
	return size*nmemb;
}

int uploader_queueSize(void)
{
	return queue_count(m_queue);
}

void uploader_setInterval(int interval)
{
	m_interval = interval;
}

