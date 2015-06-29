/**********************************************************************
Program: Assignment #3: POSIX Programming Assignment

Description: This program simulates a version of the producer/
consumer problem in the form of a candy factory. The two producers
are a Frog Bites generator and an Escargot Suckers generator,
and the two consumers are employees Lucy and Ethel. The buffer size
is 10, and there are no more than 3 Frog Bites on the conveyor
belt at any given time. Production ends once 100 candies have been
produced, and a production report is outputted

Files:
	- mizzoCandy.h (this one)
	- mizzoCandy.c
	- producer.c
	- consumer.c
	- Makefile

- Viet Truong, 815466048, Spring 2015 CS 570

*********************************************************************/

#ifndef MIZZO_CANDY_H
#define MIZZO_CANDY_H

#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // for sleep
#include <string.h> //for string compare

#define MAX_FROGS 3
#define MAX_BELT_SIZE 10		// Total items on conveyor belt at once
#define TOTAL_CANDIES 100		// Total items produced by a candy generator

#define FROG_BITE 1
#define ESCARGOT 0

#define MSPERSEC 1000			// One thousand milliseconds per second
#define NSPERMS 1000000			// One million nanoseconds per millisecond

typedef struct {
	int storage[MAX_BELT_SIZE];		// conveyor belt storage

	// Counters
	int beltCount;
	int frogCount;
	int escargotCount;
	int barrierCount;

	// Totals
	int totalProduced;
	int totalConsumed;

	// Semaphores
	sem_t fillCount;			// Items produced
	sem_t emptyCount;			// remaining space
	sem_t frogSem;				// Frogs on conveyor belt
	sem_t barrierSem;

	// Mutexes
	pthread_mutex_t mutex;		// Protects the counters
} semBuffer;

typedef struct {
	semBuffer *crit_section;
	int produced;
	int duration;
	char *name;
} producer;

typedef struct {
	semBuffer *crit_section;
	int duration;
	int frogBiteConsumed;
	int escargotConsumed;
	char *name;
} consumer;

void *produceCandy (void *c);

void *consumeCandy (void *w);

#endif


