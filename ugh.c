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

#define NUM_THREADS 3

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

	// Conditional Variable
	pthread_cond_t barrierCond; 	// Barrier

	// Mutexes
	pthread_mutex_t mutex;		// Protects the counters
	pthread_mutex_t groupMutex; // Barrier mutex
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

void *produceCandy (void *c) {
	producer *Producer = (producer *)c;
	semBuffer *producerCritSection = Producer->crit_section;
	int candyMade = 0;
	char *candyName = Producer->name;

	int maxSemTest, emptySemTest;
	int semTest;

	//Critical Point
	while (producerCritSection->totalProduced < TOTAL_CANDIES) {

		// Produce Item
		if (strcmp(candyName, "frog bite") == 0)
			candyMade = FROG_BITE;
		else if (strcmp(candyName, "escargot") == 0)
			candyMade = ESCARGOT;

		sem_wait(&producerCritSection->emptyCount); 		// Decrement empty count
		pthread_mutex_lock(&producerCritSection->mutex); 	// Enter critical region

			// Handle Frog Bites
			if (candyMade == FROG_BITE) {
				pthread_mutex_unlock(&producerCritSection->mutex);
				sem_wait(&producerCritSection->frogSem);
				pthread_mutex_lock(&producerCritSection->mutex);
				producerCritSection->storage[producerCritSection->beltCount++] = candyMade;
				producerCritSection->frogCount++;
			}

			// Handle Escargot Sucker
			else if (candyMade == ESCARGOT) {
				producerCritSection->storage[producerCritSection->beltCount++] = candyMade;
				producerCritSection->escargotCount++;
			}

			// Update Counters
			producerCritSection->totalProduced++;

			// Output
			printf("Belt: %d frogs + %d escargots = %d. produced: %d\t", 
				producerCritSection->frogCount, producerCritSection->escargotCount, 
				producerCritSection->beltCount, producerCritSection->totalProduced);
			if (candyMade == FROG_BITE)
				printf("Added crunchy frog bite.\n");
			else if (candyMade == ESCARGOT)
				printf("Added escargot sucker.\n");
			fflush(stdout);

		pthread_mutex_unlock(&producerCritSection->mutex);	// Exit critical region
		sem_post(&producerCritSection->fillCount);			// Increment count of full slots

		// Update local thread counter
		Producer->produced++;
	}

	// Barrier
	pthread_mutex_lock(&producerCritSection->groupMutex);
		if (++producerCritSection->barrierCount == NUM_THREADS)
			pthread_cond_signal(&producerCritSection->barrierCond);
	pthread_mutex_unlock(&producerCritSection->groupMutex);

	pthread_exit(NULL);
}

void *consumeCandy (void *w) {
	printf("\nConsumer is started\n");
	consumer *Consumer = (consumer *)w;
	semBuffer *consumerCritSection = Consumer->crit_section;
	int candyConsumed = 0, i;

	int maxSemTest, emptySemTest;	
	
	while (consumerCritSection->totalConsumed < TOTAL_CANDIES) {

		sem_wait(&consumerCritSection->fillCount);			// Decrement full count
		pthread_mutex_lock(&consumerCritSection->mutex);	// Enter critical region

			// Candy is first item off of conveyor belt
			candyConsumed = consumerCritSection->storage[0];

			// Rearrange storage
			for (i=0; i<consumerCritSection->beltCount; i++)
				consumerCritSection->storage[i] = consumerCritSection->storage[i+1];
			consumerCritSection->beltCount--;


			// Handle Frog Bites
			if (candyConsumed == FROG_BITE) {
				sem_post(&consumerCritSection->frogSem);
				consumerCritSection->frogCount--;
			}

		// Handle Escargot Suckers
			else if (candyConsumed == ESCARGOT) 
				consumerCritSection->escargotCount--;

			// Update counters
			consumerCritSection->totalConsumed++;

			// Output
			printf("Belt: %d frogs + %d escargots = %d. produced: %d\t", 
				consumerCritSection->frogCount, consumerCritSection->escargotCount, 
				consumerCritSection->beltCount, consumerCritSection->totalProduced);
			if (candyConsumed == FROG_BITE)
				printf("%s consumed crunchy frog bite.\n", Consumer->name);
			else if (candyConsumed == ESCARGOT)
				printf("%s consumed escargot sucker.\n", Consumer->name);
			fflush(stdout);

		pthread_mutex_unlock(&consumerCritSection->mutex);	// Exit critical region
		sem_post(&consumerCritSection->emptyCount);			// Increment count of empty slots

		// Update local thread counters
		if (candyConsumed == FROG_BITE)
			Consumer->frogBiteConsumed++;
		else if (candyConsumed == ESCARGOT)
			Consumer->escargotConsumed++;

	}

	// Barrier
	pthread_mutex_lock(&consumerCritSection->groupMutex);
		if (++consumerCritSection->barrierCount == NUM_THREADS)
			pthread_cond_signal(&consumerCritSection->barrierCond);
	pthread_mutex_unlock(&consumerCritSection->groupMutex);

	pthread_exit(NULL);
}

int main (int argc, char *argv[]) {
	int flagCheck;

	semBuffer *crit_section = malloc(sizeof(semBuffer));

	// Initialize totals and counters all to 0
	crit_section->beltCount = crit_section->frogCount = crit_section->escargotCount = 
	crit_section->totalProduced = crit_section->totalConsumed = crit_section->barrierCount = 0;

	producer *frogBite = malloc(sizeof(producer));
	frogBite->crit_section = crit_section;
	frogBite->produced = frogBite->duration = 0;
	frogBite->name = "frog bite";

	producer *escargot = malloc(sizeof(producer));
	escargot->crit_section = crit_section;
	escargot->produced = escargot->duration = 0;
	escargot->name = "escargot";

	consumer *lucy = malloc(sizeof(consumer));
	lucy->crit_section = crit_section;
	lucy->frogBiteConsumed = lucy->escargotConsumed = lucy->duration = 0;
	lucy->name = "Lucy";

	consumer *ethel = malloc(sizeof(consumer));
	ethel->crit_section = crit_section;
	ethel->frogBiteConsumed = ethel->escargotConsumed = ethel->duration = 0;
	ethel->name = "Ethel";

	/*
	while ((flagCheck = getopt(argc, argv, "ELfe")) != -1) {
		switch(flagCheck) {
			case 'E':
				printf("You put Ethel's duration to %d\n", optopt);
				break;
			case 'L':
				printf("You put Lucy's duration to %d\n", optopt);
				break;
			case 'f':
				printf("You put Frog Bites duration to %d\n", optopt);
				break;
			case 'e':
				printf("Yout put Escargot Suckers duration to %d\n", optopt);
				break;
			case '?':
				if (optopt == 'E' || optopt == 'L' || optopt == 'f' || optopt == 'e')
					printf("Option -%c requires an argument.\n", optopt);
				else if (isprint(optopt))
					printf("Unknown option -%c.\n", optopt);
				else printf("Unknown option character\n");
				exit(0);
			default:
				break;
		}
	}
	*/

	pthread_t producerThread, consumerThread;
	pthread_t frogThread, escargotThread, lucyThread, ethelThread;

	// Initialize semaphores
	sem_init(&crit_section->fillCount, 0, 0);
	sem_init(&crit_section->emptyCount, 0, MAX_BELT_SIZE);
	sem_init(&crit_section->frogSem, 0, MAX_FROGS);

	// Initialize mutexes
	pthread_mutex_init(&crit_section->mutex, NULL);

	// Initialize conditional variable
	pthread_cond_init(&crit_section->barrierCond, 0);

	// Producer Threads
	pthread_create(&frogThread, NULL, produceCandy, (void*) frogBite);
	pthread_create(&escargotThread, NULL, produceCandy, (void*) escargot);

	// Consumer Threads
	pthread_create(&ethelThread, NULL, consumeCandy, (void*) ethel);
	pthread_create(&lucyThread, NULL, consumeCandy, (void*) lucy);

	// Production Output
	while (crit_section->barrierCount < NUM_THREADS) 
		pthread_cond_wait(&crit_section->barrierCond, &crit_section->groupMutex);
	printf("\nPRODUCTION REPORT\n");
	printf("------------------------------------------\n");
	printf("Crunchy Frog Bite producer generated %d candies\n", frogBite->produced);
	printf("Escargot Sucker producer generated %d candies\n", escargot->produced);
	printf("Lucy consumed %d Crunchy Frog Bites + %d Escargot Suckers = %d\n", lucy->frogBiteConsumed, lucy->escargotConsumed, lucy->frogBiteConsumed + lucy->escargotConsumed);
	printf("Ethel consumed %d Crunchy Frog Bites + %d Escargot Suckers = %d\n", ethel->frogBiteConsumed, ethel->escargotConsumed, ethel->frogBiteConsumed + ethel->escargotConsumed);
	
	free(lucy);
	free(ethel);
	free(escargot);
	free(frogBite);
	free(crit_section);

	exit(0);
}