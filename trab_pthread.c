/* PThreads C library study
*  Author: Giovanni Sencioles
*  2023
*/

//Ignore warnings and pthread.h timespec
#pragma once
#define _CRT_SECURE_NO_WARNINGS 1
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#pragma comment(lib,"pthreadVC2.lib")
#define HAVE_STRUCT_TIMESPEC

//Used libraries
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

//Macros used to input matrix features
#define TAM_LIN_MATRIZ 9 //Matrix number of lines
#define TAM_COL_MATRIZ 9 //Matrix number of columns
#define SEMENTE 13 //srand() seed for generating the same random matrix
#define RANDOM_LIMIT 32000 //Max limit element generated
#define TAM_LIN_MACRO 1	//Block number of lines
#define TAM_COL_MACRO 1	//Block number of columns
#define QTD_CORES 3	//CPU cores used

//Global variables
int** matriz[TAM_LIN_MATRIZ][TAM_COL_MATRIZ];	//Random matrix
int count_primo; //Prime number counter
pthread_mutex_t mtx;	//PThread mutex for critical zone manipulation
int qtd_bloco;	//Number of blocks
int disp_bloco;	//Available blocks counter

//Block edges
typedef struct {
	int top_l, top_c, bot_l, bot_c, data;	
} macrobloco;

//Global block vector
macrobloco* vet_bloco;

//Prime number tracker prototype
int acha_primo(int n);

//Prime number tracker function
int acha_primo(int n) {
	if (n == 0 || n == 1) {	
		return 0;
	}
	for (int i = 2; i <= sqrt(n); i++) {
		if (n % i == 0) {	
			return 0;
		}
	}
	count_primo++; 
}

//Generate random matrix
void gera_matriz() {
	srand(SEMENTE);	//Assign the seed defined globally
	for (int i = 0; i < TAM_LIN_MATRIZ; i++) {
		for (int j = 0; j < TAM_COL_MATRIZ; j++) {
			matriz[i][j] = rand() % RANDOM_LIMIT;	//Randomly generating numbers from 0 to the defined limit
		}
	}
}

//Generate block from matrix
void gera_macro() {
	qtd_bloco = (TAM_LIN_MATRIZ * TAM_COL_MATRIZ) / (TAM_LIN_MACRO * TAM_COL_MACRO);
	disp_bloco = 0;
	vet_bloco = malloc(sizeof(macrobloco) * qtd_bloco);	//Block vector
	for (int i = 0; i < TAM_LIN_MATRIZ; i += TAM_LIN_MACRO) {
		for (int j = 0; j < TAM_COL_MATRIZ; j += TAM_COL_MACRO) {
			vet_bloco[disp_bloco].top_l = i;
			vet_bloco[disp_bloco].top_c = j;
			vet_bloco[disp_bloco].bot_l = i + TAM_LIN_MACRO - 1;
			vet_bloco[disp_bloco].bot_c = j + TAM_COL_MACRO - 1;
			disp_bloco++;
		}
	}
}

//Serial prime number counter
void conta_serial() {
	for (int i = 0; i < TAM_LIN_MATRIZ; i++) {
		for (int j = 0; j < TAM_COL_MATRIZ; j++) {
			acha_primo(matriz[i][j]);
		}
	}
	printf("\nIn the %d x %d matrix, %d prime numbers were found.", TAM_LIN_MATRIZ, TAM_COL_MATRIZ, count_primo);
}

//Parallelized prime number counter
void primo_paralela() {
	macrobloco bloc;	//Local variable to alloc blocks from a vector
	while (1) {
		if (disp_bloco > 0) {
			pthread_mutex_lock(&mtx);	//Critical zone start to guarantee block uniqueness	
			bloc = vet_bloco[qtd_bloco-disp_bloco];	//Assign an available block
			disp_bloco--; //Decrement available blocks
			pthread_mutex_unlock(&mtx);	//Release mutex
			for (int i = bloc.top_l; i < bloc.bot_l; i++) {
				for (int j = bloc.top_c; j < bloc.bot_c; j++) {
					pthread_mutex_lock(&mtx);
					acha_primo(matriz[i][j]);	//Exclusively search for prime numbers
					pthread_mutex_unlock(&mtx);
				}
			}
		}
		else {
			printf("\nTotal de numeros primos encontrados: %d", count_primo);
			pthread_exit(NULL);
		}
	}
	
}

//Function to create thread vector
void conta_paralela() {
	//Parametrized vector creation using set core number
	pthread_t vet_thread[QTD_CORES];	
	for (int i = 0; i < QTD_CORES; i++) {
		if (pthread_create(&vet_thread[i], NULL, primo_paralela, NULL) != 0) {
			perror("Pthread_create falhou!");
			exit(1);
		}
	}
	for (int j = 0; j < QTD_CORES; j++) {
		if (pthread_join(vet_thread[j], NULL)!=0) {
			perror("Pthread_join falhou!");
				exit(1);
		}
	}
}

//Function to display matrix. USE ONLY FOR TESTS
void printa_matriz() {
	for (int i = 0; i < TAM_LIN_MATRIZ; i++) {
		for (int j = 0; j < TAM_COL_MATRIZ; j++) {
			printf(" %d ", matriz[i][j]);
		}
		printf("\n");
	}
}

//Function to display blocks. USE ONLY FOR TESTS
void printa_blocos() {
	printf("\nMacroblocos %d x %d gerados: %d\n", TAM_LIN_MACRO, TAM_COL_MACRO, qtd_bloco);
	for (int i = 0; i < qtd_bloco; i++) {
		macrobloco b = vet_bloco[i];
		printf("\nMacrobloco %d:\n", i + 1);
		printf("Primeiro elemento: %d\n", matriz[b.top_l][b.top_c]);
		printf("Ultimo elemento: %d\n", matriz[b.bot_l][b.bot_c]);
	}
}

int main(int argc, char* argv[]) {
	gera_matriz();	//Generate random matrix
	gera_macro();	//Divide matrix into blocks
	pthread_mutex_init(&mtx, NULL);	//Initializing mutex
	//printa_matriz();	//REMOVE COMMENT TO TEST. Display generated matrix
	//printa_blocos();	//REMOVE COMMENT TO TEST. Display generated blocks
	
	printf("***SERIAL SEARCH***\n");

	clock_t tempo_serial_ini = clock();
	conta_serial();	//Serial search
	clock_t tempo_serial_end = clock();
	
	printf("\nSinglethread time: %.4lf segundos \n \n", (double)(tempo_serial_end-tempo_serial_ini)/CLOCKS_PER_SEC);
	
	printf("***PARALLELIZED SEARCH (%d THREADS, %d BLOCKS WITH %d x %d DIMENSIONS)***\n",QTD_CORES,qtd_bloco,TAM_LIN_MACRO,TAM_COL_MACRO);
	
	clock_t tempo_parallel_ini = clock();
	conta_paralela();	//Parallelized search
	clock_t tempo_parallel_end = clock();
	
	printf("\nMultithread time: %.4lf segundos \n \n", (double)(tempo_parallel_end - tempo_parallel_ini)/CLOCKS_PER_SEC);
	
	pthread_mutex_destroy(&mtx);	//Mutex not used anymore
	return 0;
}
