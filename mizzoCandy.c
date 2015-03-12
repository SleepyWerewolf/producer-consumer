#include "mizzoCandy.h"

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
