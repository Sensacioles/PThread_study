/* Trabalho de Introducao a Programacao Multithread com PThreads
*  Aluno: Giovanni Sencioles
*  Professor: Flavio Giraldeli
*  Disciplina: Sistemas Operacionais 2023/1
*  IFES Campus Serra */

//Comandos iniciais para remover avisos e "ignorar" timespec do pthread.h
#pragma once
#define _CRT_SECURE_NO_WARNINGS 1
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#pragma comment(lib,"pthreadVC2.lib")
#define HAVE_STRUCT_TIMESPEC

//Bibliotecas utilizadas
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

//Definicao de macros
#define TAM_LIN_MATRIZ 9 //Numero de linhas da matriz
#define TAM_COL_MATRIZ 9 //Numero de colunas da matriz
#define SEMENTE 13 //Seed de srand() com intuito de gerar sempre a mesma matriz aleatoria
#define RANDOM_LIMIT 32000 //Maior numero que possa ser gerado aleatoriamente
#define TAM_LIN_MACRO 1	//Numero de linhas do macrobloco
#define TAM_COL_MACRO 1	//Numero de colunas do macrobloco
#define QTD_CORES 3	//Numero de nucleos a serem utilizados baseado na lista abaixo
/*
#define N_PCORES_AMD 6	//Numero de nucleos fisicos da CPU do PC1 (Desktop, AMD)
#define N_LCORES_AMD 6	//Numero de nucleos logicos/virtuais da CPU do PC1
#define N_PCORES_INT 4	//Numero de nucleos fisicos da CPU do PC2 (Notebook, Intel) 
#define N_LCORES_INT 8	//Numero de nucleos logicos/virtuais da CPU do PC2
*/

//Criacao de variaveis globais
int** matriz[TAM_LIN_MATRIZ][TAM_COL_MATRIZ];	//Matriz a alocar valores aleatorios
int count_primo; //Contador de numeros primos
pthread_mutex_t mtx;	//Mutex a fim de garantir atomicidade da thread
int qtd_bloco;	//Quantidade total de macroblocos
int disp_bloco;	//Contador de blocos disponíveis

//Estrutura representando limites do macrobloco
typedef struct {
	//top_l e top_c sao os indices do elemento inicial do bloco, bot_l e bot_c sao os finais. 
	//data armazena o numero extraido da matriz
	int top_l, top_c, bot_l, bot_c, data;	
} macrobloco;

//Vetor de macroblocos global
macrobloco* vet_bloco;

//Prototipo para funcao de identificar numero primo
int acha_primo(int n);


//Funcao para identificar numero primo. Retorno como 0 = nao-primo
int acha_primo(int n) {
	if (n == 0 || n == 1) {	//0 e 1 nao sao primos
		return 0;
	}
	for (int i = 2; i <= sqrt(n); i++) {
		if (n % i == 0) {	//Verifica se numero tem divisores
			return 0;
		}
	}
	count_primo++; //Caso nao encontre nenhum divisor ate a raiz quadrada do numero, ele eh primo
}

//Funcao para geracao de matriz aleatoria e atribuicao dos valores a matriz global
void gera_matriz() {
	srand(SEMENTE);	//Definindo seed para geracao de numeros aleatorios
	for (int i = 0; i < TAM_LIN_MATRIZ; i++) {
		for (int j = 0; j < TAM_COL_MATRIZ; j++) {
			matriz[i][j] = rand() % RANDOM_LIMIT;	//Gerando numeros aleatorios de 0 a 31999
		}
	}
}

//Funcao para divisao da matriz em macroblocos/submatrizes e gerar um vetor para armazenar os mesmos
void gera_macro() {
	qtd_bloco = (TAM_LIN_MATRIZ * TAM_COL_MATRIZ) / (TAM_LIN_MACRO * TAM_COL_MACRO);
	disp_bloco = 0;
	vet_bloco = malloc(sizeof(macrobloco) * qtd_bloco);	//Vetor de "objetos" de macroblocos 
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

//Funcao para contabilizar numeros primos na matriz de forma serial 
void conta_serial() {
	for (int i = 0; i < TAM_LIN_MATRIZ; i++) {
		for (int j = 0; j < TAM_COL_MATRIZ; j++) {
			//Usando a funcao acha_primo() no elemento M(ixj), automaticamente incrementando count_primo
			acha_primo(matriz[i][j]);
		}
	}
	printf("\nNa matriz %d x %d, foram encontrados %d numeros primos.", TAM_LIN_MATRIZ, TAM_COL_MATRIZ, count_primo);
}

//Funcao para contabilizar numeros primos na matriz paralelamente
void primo_paralela() {
	macrobloco bloc;	//Variavel local para armazenar o macrobloco a ser trabalhado por cada thread
	while (1) {
		if (disp_bloco > 0) {
			pthread_mutex_lock(&mtx);	//Regiao critica para atribuicao de macrobloco atomicamente	
			bloc = vet_bloco[qtd_bloco-disp_bloco];	//Atribui um macrobloco a ser utilizado
			disp_bloco--; //Decrementa a variavel de controle de blocos disponiveis
			pthread_mutex_unlock(&mtx);	//Libera o mutex para outra thread buscar macrobloco
			for (int i = bloc.top_l; i < bloc.bot_l; i++) {
				for (int j = bloc.top_c; j < bloc.bot_c; j++) {
					pthread_mutex_lock(&mtx);
					acha_primo(matriz[i][j]);	//Realiza a busca de numeros primos exclusivamente
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

//Funcao para criar vetor de threads, usando a funcao primo_paralela() como rotina de cada uma
void conta_paralela() {
	//Criacao de vetor de threads parametrizada a partir da funcao set_cores()
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

//Funcao para printar matriz (para fins de teste)
void printa_matriz() {
	for (int i = 0; i < TAM_LIN_MATRIZ; i++) {
		for (int j = 0; j < TAM_COL_MATRIZ; j++) {
			printf(" %d ", matriz[i][j]);
		}
		printf("\n");
	}
}

//Funcao para printar macroblocos (para fins de testes)
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
	//Declaracao da struct timespec para calculo dos tempos das buscas
	//struct timespec tempo_serial_ini, tempo_serial_end, tempo_parallel_ini, tempo_parallel_end;
	//OBS.: timespec deu erro porque CLOCK_REALTIME nao esta definido no time.h
	gera_matriz();	//Gera matriz randomica
	gera_macro();	//Divide a matriz em submatrizes (macroblocos)
	pthread_mutex_init(&mtx, NULL);	//Inicializando o mutex pra trabalhar a regiao critica
	//printa_matriz();	//Visualizar a matriz
	//printa_blocos();	//Visualizar os macroblocos por primeiro e ultimo elemento
	printf("***METODO SERIAL***\n");
	//clock_gettime(CLOCK_REALTIME, &tempo_serial_ini);	//Marca o comeco do tempo da busca serial
	clock_t tempo_serial_ini = clock();
	conta_serial();	//Busca serial, o metodo single-thread
	clock_t tempo_serial_end = clock();
	//clock_gettime(CLOCK_REALTIME, &tempo_serial_end);	//Marca o fim do tempo da busca serial
	//printf("Tempo de processamento: %ld\n \n", (tempo_serial_end.tv_sec - tempo_serial_ini.tv_sec));
	printf("\nTempo de processamento: %.4lf segundos \n \n", (double)(tempo_serial_end-tempo_serial_ini)/CLOCKS_PER_SEC);
	printf("***METODO PARALELIZADO (%d THREADS, %d macroblocos de dimensoes %d x %d)***\n",QTD_CORES,qtd_bloco,TAM_LIN_MACRO,TAM_COL_MACRO);
	//clock_gettime(CLOCK_REALTIME, &tempo_parallel_ini);	//Marca o comeco do tempo da busca paralelizada
	clock_t tempo_parallel_ini = clock();
	conta_paralela();	//Busca paralelizada, o metodo multi-thread
	clock_t tempo_parallel_end = clock();
	//clock_gettime(CLOCK_REALTIME, &tempo_parallel_end);	//Marca o fim do tempo da busca paralelizada
	//printf("Tempo de processamento: %ld\n \n", (tempo_parallel_end.tv_sec - tempo_parallel_ini.tv_sec));
	printf("\nTempo de processamento: %.4lf segundos \n \n", (double)(tempo_parallel_end - tempo_parallel_ini)/CLOCKS_PER_SEC);
	pthread_mutex_destroy(&mtx);	//Desaloca mutex pois nao sera mais usado
	return 0;
}