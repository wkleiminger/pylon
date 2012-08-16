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
  Module    : queue
  Used by   : smlogger
  Purpose   : Provides a thread-safe fixed-size queue for 
              producer consumer problems
  
  Version   : 1.0
  Date      : 06.05.2012
  Author    : Daniel Pauli
\******************************************************************************/

#include "queue.h"

#include <pthread.h>

#include "common.h"

// Capacity levels to log in order to foresee overflows
static const double s_capLevels[] = 
	{0.01, 0.25, 0.5, 0.75, 0.99};

#define CAP_DEV 0.01

////////////////////////////////////////////////////////////////////////////////
// TYPES
////////////////////////////////////////////////////////////////////////////////

// The queue
struct Queue_s {

	// Pointer to the buffer to hold the items
	char* buffer;

	// Number of elements
	size_t count;
	
	// Maximum number of elements that can be stored in the buffer
	size_t capacity;
	
	// The size of a single item
	size_t itemSize;
	
	// Index of the first element
	int head;
	
	// Index of the last element
	int tail;
	
	// Current capacity log level
	int level;
	
	// The mutex
	pthread_mutex_t lock;	
};


////////////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////////////

Queue* queue_create(size_t capacity, size_t itemSize)
{
	Queue* queue = malloc(sizeof(Queue));
	if (!queue) {
		LOG(0, "malloc failed: %s\n", strerror(errno));
		return NULL;
	}
	
	int error = pthread_mutex_init(&queue->lock, NULL);
	if (error) {
		LOG(0, "Failed to create mutex: %s\n", strerror(error));
		free(queue);
		return NULL;
	}
	
	queue->buffer = malloc(capacity * itemSize);
	if (!queue->buffer) {
		LOG(0, "Failed to allocate buffer: %s\n", strerror(errno));
		free(queue);
		return NULL;
	}
	
	queue->capacity = capacity;
	queue->itemSize = itemSize;
	queue->count = 0;
	queue->head = 0;
	queue->tail = 0;
	queue->level = 0;

	return queue;
}

void queue_free(Queue* queue)
{
	if (queue) {
		pthread_mutex_destroy(&queue->lock);
		free(queue->buffer);
		free(queue);
	}
}

int queue_enqueue(Queue* queue, const void* item)
{
	// Acquire lock
	int error = pthread_mutex_lock(&queue->lock);
	if (error) {
		LOG(0, "Failed to lock mutex: %s\n", strerror(error));
		return 0;
	}

	// Check if buffer full
	if (queue->count >= queue->capacity) {
		pthread_mutex_unlock(&queue->lock);
		return 0;
	}

	// Check if some threshold reached
	for (int i = queue->level; i < ARRAY_LENGTH(s_capLevels); i++) {
		if (queue->count == (int)((s_capLevels[i] + CAP_DEV) * queue->capacity)) {
			LOG(1, "Measurement buffer exceeds %.0f%% of its capacity\n", s_capLevels[i]*100);
			queue->level = i+1;
		}
	}
	
	// Store measurement at the back of the queue
	memcpy(queue->buffer + queue->tail * queue->itemSize, item, queue->itemSize);
	queue->tail = (queue->tail+1) % queue->capacity;
	queue->count++;

  // Success
  pthread_mutex_unlock(&queue->lock);
	return 1;

}

int queue_dequeue(Queue* queue, void* item)
{
	// Acquire lock
	int error = pthread_mutex_lock(&queue->lock);
	if (error) {
		LOG(0, "Failed to lock mutex: %s\n", strerror(error));
		return 0;
	}

	// Check if queue empty
	if (queue->count == 0) {
		pthread_mutex_unlock(&queue->lock);
		return 0; 
	}

	// Check if some threshold reached
	for (int i = 0; i < queue->level; i++) {
		if (queue->count == (int)((s_capLevels[i] - CAP_DEV) * queue->capacity)) {
			LOG(1, "Measurement buffer falls below %.0f%% of its capacity\n", s_capLevels[i]*100);
			queue->level = i;
		}
	}
	
	// Remove measurement from the front of the queue
	memcpy(item, queue->buffer + queue->head * queue->itemSize, queue->itemSize);
	queue->head = (queue->head+1) % queue->capacity;
	queue->count--;
	
	pthread_mutex_unlock(&queue->lock);	
	return 1; // Success
}

size_t queue_count(Queue* queue)
{
	// Acquire lock
	int error = pthread_mutex_lock(&queue->lock);
	if (error) {
		LOG(0, "Failed to lock mutex: %s\n", strerror(error));
		return -1;
	}

	int count = queue->count;
	
	pthread_mutex_unlock(&queue->lock);	
	return count;
}

void queue_clear(Queue* queue)
{
	// Acquire lock
	int error = pthread_mutex_lock(&queue->lock);
	if (error) {
		LOG(0, "Failed to lock mutex: %s\n", strerror(error));
		return;
	}

	queue->count = 0;
	queue->head = 0;
	queue->tail = 0;
	
	pthread_mutex_unlock(&queue->lock);	
}

