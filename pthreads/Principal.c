#pragma once
#define _CRT_SECURE_NO_WARNINGS 1
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#pragma comment(lib,"pthreadVC2.lib")
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define MAX_SIZE_MATRIZ 31999

#define MACROBLOCO_ALTURA 10
#define MACROBLOCO_LARGURA 10

#define SEED 8008135

#define MATRIZ_ALTURA 13000
#define MATRIZ_LARGURA 13000

#define NUM_THREADS 4


int** matriz;
int primos_cont = 0;

int qnt_macrobloco;
int linhas_macrobloco;
int colunas_macrobloco;

pthread_mutex_t mutex_primos;
pthread_mutex_t mutex_macrobloco;
pthread_mutex_t mutex_erro_printf;

int macrobloco_proximo = 1;


int ehPrimo(int n) {
	//if (n == 2 || n == 3) return 1;
	//if (n < 2 || n % 2 == 0) return 0;
	//if (n < 9) return 1;
	//if (n % 3 == 0) return 0;
	//int r = sqrt(n);
	//for (int i = 5; i <= r; i += 6) {
	//	if (n % i == 0) return 0;
	//	if (n % (i + 2) == 0) return 0;
	//}

	for (int i = 2; i <= sqrt(n); i++) {
		if (n % i == 0) return 0;
	}

	return 1;
}

// funções para alocar e liberar matrizes retiradas da apostila de programação em c (pag. 113)
int** Alocar_matriz_real(int m, int n)
{
	int** v; /* ponteiro para a matriz */
	int i; /* variável auxiliar */
	if (m < 1 || n < 1) { /* verifica parametros recebidos */
		printf("** Erro: Parametro invalido **\n");
		return (NULL);
	}
	/* aloca as linhas da matriz */
	v = calloc(m, sizeof(int*)); /* Um vetor de m ponteiros para float */
	if (v == NULL) {
		printf("** Erro: Memoria Insuficiente **");
		return (NULL);
	}

	/* aloca as colunas da matriz */
	for (i = 0; i < m; i++) {
		v[i] = calloc(n, sizeof(int)); /* m vetores de n floats */
		if (v[i] == NULL) {
			printf("** Erro: Memoria Insuficiente **");
			return (NULL);
		}
	}

	return (v); /* retorna o ponteiro para a matriz */
}
int** Liberar_matriz_real(int m, int n, int** v)
{
	int i; /* variável auxiliar */
	if (v == NULL) return (NULL);
	if (m < 1 || n < 1) { /* verifica parâmetros recebidos */
		printf("** Erro: Parametro invalido **\n");
		return (v);
	}
	for (i = 0; i < m; i++) free(v[i]); /* libera as linhas da matriz */
	free(v); /* libera a matriz (vetor de ponteiros) */
	return (NULL); /* retorna um ponteiro nulo */
}

void preencher_matriz(int** matriz, int altura, int largura) {
	int i, j;
	srand(SEED);
	for (i = 0; i < altura; i++) {
		for (j = 0; j < largura; j++) matriz[i][j] = rand() % MAX_SIZE_MATRIZ;
	}
}

void buscaSerial() {
	primos_cont = 0;
	for (int i = 0; i < MATRIZ_ALTURA; i++) {
		for (int j = 0; j < MATRIZ_LARGURA; j++) {
			if (ehPrimo(matriz[i][j])) {
				primos_cont++;
			}
		}
	}
}

void* thread_busca(void* diov) {
	int macrobloco_atual;
	
	while (1) {
		
		if (macrobloco_proximo > qnt_macrobloco) {
			pthread_mutex_unlock(&mutex_macrobloco); 
			break;
		}

		pthread_mutex_lock(&mutex_macrobloco);
		macrobloco_atual = macrobloco_proximo++;
		pthread_mutex_unlock(&mutex_macrobloco);

		int linha_inicio = ((macrobloco_atual - 1) / colunas_macrobloco) * MACROBLOCO_ALTURA;
		int linha_fim = linha_inicio + MACROBLOCO_ALTURA - 1;
		int coluna_inicio = ((macrobloco_atual - 1) % colunas_macrobloco) * MACROBLOCO_LARGURA;
		int coluna_fim = coluna_inicio + MACROBLOCO_LARGURA - 1;

		if (linha_inicio < 0 || linha_inicio >= MATRIZ_ALTURA ||
			linha_fim < 0 || linha_fim >= MATRIZ_ALTURA ||
			coluna_inicio < 0 || coluna_inicio >= MATRIZ_LARGURA ||
			coluna_fim < 0 || coluna_fim >= MATRIZ_LARGURA) 
		{
			pthread_mutex_lock(&mutex_erro_printf);
			printf("Error: violacao de acesso ao ler local (provavel que o a divisao da matriz por macroblocos nao seja exata)");
			exit(EXIT_FAILURE);
			pthread_mutex_unlock(&mutex_erro_printf);
		}

		for (int i = linha_inicio; i <= linha_fim; i++) {
			for (int j = coluna_inicio; j <= coluna_fim; j++) {
				if (ehPrimo(matriz[i][j])) {
					pthread_mutex_lock(&mutex_primos);
					primos_cont++;
					pthread_mutex_unlock(&mutex_primos);
				}
			}
		}
	}

	pthread_exit(NULL);
}

void buscaParalela() {
	primos_cont = 0;
	pthread_t threads[NUM_THREADS];

	qnt_macrobloco = (int)(ceil((double)(MATRIZ_ALTURA * MATRIZ_LARGURA) / (MACROBLOCO_ALTURA * MACROBLOCO_LARGURA)));
	linhas_macrobloco = (int)(ceil((double)MATRIZ_ALTURA / MACROBLOCO_ALTURA));
	colunas_macrobloco = (int)(ceil((double)MATRIZ_LARGURA / MACROBLOCO_LARGURA));

	pthread_mutex_init(&mutex_primos, NULL);
	pthread_mutex_init(&mutex_macrobloco, NULL);
	pthread_mutex_init(&mutex_erro_printf, NULL);

	for (int i = 0; i < NUM_THREADS; i++) pthread_create(&threads[i], NULL, thread_busca, NULL);

	for (int i = 0; i < NUM_THREADS; i++) pthread_join(threads[i], NULL);

	pthread_mutex_destroy(&mutex_primos);
	pthread_mutex_destroy(&mutex_macrobloco);
}


int main() {
	clock_t timer_inicio, timer_fim;
	double tempo_serial, tempo_paralelo, speedup;

	printf("Alocando e preenchendo matriz... ");
	matriz = Alocar_matriz_real(MATRIZ_ALTURA, MATRIZ_LARGURA);
	preencher_matriz(matriz, MATRIZ_ALTURA, MATRIZ_LARGURA);
	printf("ok!\n");

	printf("\nIniciando busca serial... ");
	timer_inicio = clock();
	buscaSerial();
	printf("ok!\n");
	timer_fim = clock();
	tempo_serial = ((double)(timer_fim - timer_inicio)) / CLOCKS_PER_SEC;
	printf("Total de numeros primos encontrados: %d\n", primos_cont);

	printf("\nIniciando busca paralela... ");
	timer_inicio = clock();
	buscaParalela();
	printf("ok!\n");
	timer_fim = clock();
	tempo_paralelo = ((double)(timer_fim - timer_inicio)) / CLOCKS_PER_SEC;
	printf("Total de numeros primos encontrados: %d\n", primos_cont);

	speedup = tempo_serial / tempo_paralelo;

	printf("\nTempo serial: %lf segundos\n", tempo_serial);
	printf("Tempo paralelo: %lf segundos\n", tempo_paralelo);
	printf("Speedup: %lf\n", speedup);

	printf("\nQuantidade de macroblocos: %d\nQuantidade de threads: %d\n", qnt_macrobloco, NUM_THREADS);

	Liberar_matriz_real(MATRIZ_ALTURA, MATRIZ_LARGURA, matriz);
}
