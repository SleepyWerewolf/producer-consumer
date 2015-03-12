#include "mizzoCandy.h"

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
