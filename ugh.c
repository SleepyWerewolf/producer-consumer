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

void *produceCandy (void *c) {
	producer *Producer = (producer *)c;
	semBuffer *producerCritSection = Producer->crit_section;
	int candyMade = 0;
	int loop = 1;
	char *candyName = Producer->name;
	struct timespec SleepTime;
	int testLoop;

	SleepTime.tv_sec = Producer->duration / MSPERSEC;
	SleepTime.tv_nsec = (Producer->duration % MSPERSEC) * NSPERMS;

	while (loop) {

		// Produce Item
		if (strcmp(candyName, "frog bite") == 0)
			candyMade = FROG_BITE;
		else if (strcmp(candyName, "escargot") == 0)
			candyMade = ESCARGOT;

		sem_wait(&producerCritSection->emptyCount); 		// Decrement empty count
		pthread_mutex_lock(&producerCritSection->mutex); 	// Enter critical section
			if (producerCritSection->totalProduced < TOTAL_CANDIES) {

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

				// Update local thread counter
				Producer->produced++;

				// Output
				printf("Belt: %d frogs + %d escargots = %d. produced: %d\t", 
					producerCritSection->frogCount, producerCritSection->escargotCount, 
					producerCritSection->beltCount, producerCritSection->totalProduced);
				if (candyMade == FROG_BITE)
					printf("Added crunchy frog bite.\n");
				else if (candyMade == ESCARGOT)
					printf("Added escargot sucker.\n");
				fflush(stdout);
			} else loop = 0;

		pthread_mutex_unlock(&producerCritSection->mutex);	// Exit critical region
		sem_post(&producerCritSection->fillCount);			// Increment count of full slots

		// Sleep
		nanosleep(&SleepTime, NULL);
	}

	pthread_exit(NULL);
}

void *consumeCandy (void *w) {
	consumer *Consumer = (consumer *)w;
	semBuffer *consumerCritSection = Consumer->crit_section;
	int candyConsumed = 0, i;
	int loop = 1;
	struct timespec SleepTime;

	SleepTime.tv_sec = Consumer->duration / MSPERSEC;
	SleepTime.tv_nsec = (Consumer->duration % MSPERSEC) * NSPERMS;
	
	while (loop) {
		sem_wait(&consumerCritSection->fillCount);			// Decrement full count
		pthread_mutex_lock(&consumerCritSection->mutex);	// Enter critical section
			if (consumerCritSection->totalConsumed < TOTAL_CANDIES) {

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

				// Update local thread counters
				if (candyConsumed == FROG_BITE)
					Consumer->frogBiteConsumed++;
				else if (candyConsumed == ESCARGOT)
					Consumer->escargotConsumed++;

				// Output
				printf("Belt: %d frogs + %d escargots = %d. produced: %d\t", 
					consumerCritSection->frogCount, consumerCritSection->escargotCount, 
					consumerCritSection->beltCount, consumerCritSection->totalProduced);
				if (candyConsumed == FROG_BITE)
					printf("%s consumed crunchy frog bite.\n", Consumer->name);
				else if (candyConsumed == ESCARGOT)
					printf("%s consumed escargot sucker.\n", Consumer->name);
				fflush(stdout);
			} else {
				loop = 0;
				sem_post(&consumerCritSection->barrierSem); // Barrier semaphore in main thread
			}

		pthread_mutex_unlock(&consumerCritSection->mutex);	// Exit critical region
		sem_post(&consumerCritSection->emptyCount);			// Increment count of empty slots

		// Sleep
		nanosleep(&SleepTime, NULL);
	}

	pthread_exit(NULL);
}

int main (int argc, char *argv[]) {
	int flagCheck;
	char *dick;

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

	// Checking optional command line arguments
	while ((flagCheck = getopt(argc, argv, "E:L:f:e:")) != -1) {
		switch(flagCheck) {
			case 'E':
				ethel->duration = atoi(optarg);
				break;
			case 'L':
				lucy->duration = atoi(optarg);
				break;
			case 'f':
				frogBite->duration = atoi(optarg);
				break;
			case 'e':
				escargot->duration = atoi(optarg);
				break;
			case '?':
				exit(0);
			default:
				exit(0);
		}
	}

	pthread_t producerThread, consumerThread;
	pthread_t frogThread, escargotThread, lucyThread, ethelThread;

	// Initialize semaphores
	sem_init(&crit_section->fillCount, 0, 0);
	sem_init(&crit_section->emptyCount, 0, MAX_BELT_SIZE);
	sem_init(&crit_section->frogSem, 0, MAX_FROGS);
	sem_init(&crit_section->barrierSem, 0, 0);

	// Initialize mutexes
	pthread_mutex_init(&crit_section->mutex, NULL);

	// Producer Threads
	pthread_create(&frogThread, NULL, produceCandy, (void*) frogBite);
	pthread_create(&escargotThread, NULL, produceCandy, (void*) escargot);

	// Consumer Threads
	pthread_create(&ethelThread, NULL, consumeCandy, (void*) ethel);
	pthread_create(&lucyThread, NULL, consumeCandy, (void*) lucy);

	// Production Output
	sem_wait(&crit_section->barrierSem);
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