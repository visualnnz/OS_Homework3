#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

// #define n_data 40000000
#define max_tasks 200 

// double data[n_data];
enum task_status { UNDONE, PROCESS, DONE };

struct sorting_task {
	double* a;
	int n_a;
	int status;
};

struct sorting_task tasks[max_tasks];

int n_data = 0;

int n_tasks = 0;
int n_undone = 0;
int n_done = 0;

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER; 

void merge_lists (double* a1, int n_a1, double* a2, int n_a2) ;
void merge_sort (double* a, int n_a) ;

void* worker (void* ptr)
{
	while (1) {
		pthread_mutex_lock(&m);	
		while (n_undone == 0)
		{
			//pthread_mutex_unlock(&m) ;
			pthread_cond_wait(&cv, &m);
		}
		
		int i;
		for (i = 0 ; i < n_tasks ; i++)
		{
			if (tasks[i].status == UNDONE)
				break;
		}

		tasks[i].status = PROCESS;
		n_undone--;
		pthread_mutex_unlock(&m);

		printf("[Thread %ld] starts Task %d\n", pthread_self(), i);

		merge_sort(tasks[i].a, tasks[i].n_a);

		printf("[Thread %ld] completed Task %d\n", pthread_self(), i);

		pthread_mutex_lock(&m);
		tasks[i].status = DONE;
		n_done++;
		pthread_mutex_unlock(&m) ;
	}
}

int main (int argc, char* argv[])
{
	char opt;
	int n_threads = 0;
	while((opt = getopt(argc, argv, "d:t:")) != -1)
	{
		switch(opt)
		{
			case 'd':
				n_data = atoi(optarg);
				if(n_data <= 0)
				{
					printf("Invalid value: string\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 't':
				n_threads = atoi(optarg);
				if(n_threads <= 0)
				{
					printf("Invalid value: string\n");
					exit(EXIT_FAILURE);
				}
				break;
			case '?':
				printf("Invalid option or no value\n");
				exit(EXIT_FAILURE);
				break;
			default:
				break;
		}
	}

	double* data = calloc(n_data, sizeof(double));

	struct timeval ts;
	gettimeofday(&ts, NULL);
	srand(ts.tv_usec * ts.tv_sec);

	int i;
	for (i = 0; i < n_data; i++)
	{
		int num = rand();
		int den = rand();
		if (den != 0.0)			
			data[i] = ((double) num) / ((double) den);
		else
			data[i] = ((double) num);

	}

	// n_threads = 8;

	struct timeval start;
	gettimeofday(&start, NULL);

	pthread_t threads[n_threads];
	for (i = 0; i < n_threads; i++)
	{
		pthread_create(
			&(threads[i]),
			NULL,
			worker,
			NULL);
	}

	for (i = 0; i < max_tasks; i++) 
	{
		pthread_mutex_lock(&m);

		tasks[n_tasks].a = data + (n_data / max_tasks) * n_tasks;
		tasks[n_tasks].n_a = n_data / max_tasks;
		if (n_tasks == max_tasks - 1) 
			tasks[n_tasks].n_a = tasks[n_tasks].n_a + n_data % max_tasks;
		tasks[n_tasks].status = UNDONE;

		n_undone++;
		n_tasks++;

		pthread_cond_signal(&cv);
		pthread_mutex_unlock(&m);
	}	
	

	pthread_mutex_lock(&m);
	while (n_done < max_tasks)
	{
		pthread_mutex_unlock(&m);

		pthread_mutex_lock(&m);
	}
	pthread_mutex_unlock(&m);

	int n_sorted = n_data / n_tasks;
	for (i = 1; i < n_tasks; i++)
	{
		merge_lists(data, n_sorted, tasks[i].a, tasks[i].n_a);
		n_sorted = n_sorted + tasks[i].n_a;
	}


	//merge_sort(data, n_data) ;

#ifdef DEBUG	
	for (int i = 0 ; i < n_data ; i++) {
			printf("%lf ", data[i]) ;
	}
#endif

	struct timeval end;
	gettimeofday(&end, NULL);

	long seconds = end.tv_sec - start.tv_sec;
	long micros = (seconds * 1000000) + (end.tv_usec - start.tv_usec);

	double execution_time = micros / 1000000;

	printf("\nExecution time: %ld.%06ld sec\n", seconds, micros);
	
	return EXIT_SUCCESS ;
}


void merge_lists (double* a1, int n_a1, double* a2, int n_a2)
{
	double* a_m = (double *) calloc(n_a1 + n_a2, sizeof(double));
	int i = 0;

	int top_a1 = 0;
	int top_a2 = 0;

	for (i = 0; i < n_a1 + n_a2; i++)
	{
		if (top_a2 >= n_a2) // 2nd
		{
			a_m[i] = a1[top_a1];
			top_a1++;
		}
		else if (top_a1 >= n_a1) 
		{
			a_m[i] = a2[top_a2];
			top_a2++;
		}
		else if (a1[top_a1] < a2[top_a2]) 
		{
			a_m[i] = a1[top_a1];
			top_a1++;
		}
		else // 1st
		{
			a_m[i] = a2[top_a2];
			top_a2++;
		}
	}
	memcpy(a1, a_m, (n_a1 + n_a2) * sizeof(double));
	free(a_m);
}

void merge_sort (double* a, int n_a)
{
	if (n_a < 2)
		return ;

	double* a1;
	int n_a1;
	double* a2;
	int n_a2;

	a1 = a;
	n_a1 = n_a / 2;

	a2 = a + n_a1;
	n_a2 = n_a - n_a1;

	merge_sort(a1, n_a1);
	merge_sort(a2, n_a2);

	merge_lists(a1, n_a1, a2, n_a2);
}

