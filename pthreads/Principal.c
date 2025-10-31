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

#define RESET   "\033[0m"
#define BOLD    "\033[1m"
//#define GREEN   "\033[32m"
#define CYAN    "\033[36m"
#define YELLOW  "\033[33m"
#define MAGENTA "\033[35m"
#define BLUE    "\033[34m"
#define GREEN "\033[38;5;46m"
#define LIGHT_GREEN "\033[38;5;118m"
#define GRAY "\033[38;5;240m"

// tamanho maximo dos numeros da matriz
#define MAX_SIZE_MATRIZ 31999

// tamanho dos macroblocos
#define MACROBLOCO_ALTURA 10
#define MACROBLOCO_LARGURA 10

// seed para a geracao de numeros "aleatorios"
#define SEED 8008135

// dimensoes da matriz
#define MATRIZ_ALTURA 5000
#define MATRIZ_LARGURA 5000

// numero de threads
#define NUM_THREADS 8

// otimizacao na funcao de verificação de numeros primos
// 1 para ativar, 0 para desativar
#define OTIMIZADO 0


// variaveis globais
int** matriz;
int primos_cont = 0;

int qnt_macrobloco;
int linhas_macrobloco;
int colunas_macrobloco;

pthread_mutex_t mutex_primos;
pthread_mutex_t mutex_macrobloco;
pthread_mutex_t mutex_erro_printf;

int macrobloco_proximo = 1;


// funcao para verificar se um numero e primo
int eh_primo(int n) {
	// se OTIMIZADO = 1, usa a versao otimizada
	if (OTIMIZADO) {
		if (n == 2 || n == 3) return 1;
		if (n < 2 || n % 2 == 0) return 0;
		if (n < 9) return 1;
		if (n % 3 == 0) return 0;
		int r = sqrt(n);
		for (int i = 5; i <= r; i += 6) {
			if (n % i == 0) return 0;
			if (n % (i + 2) == 0) return 0;
		}
	}
	// versao nao otimizada
	else {
		for (int i = 2; i <= sqrt(n); i++) {
			if (n % i == 0) return 0;
		}
	}
	
	// se chegou aqui, o numero e primo
	return 1;
}

// funcoes para alocar e liberar matrizes retiradas da apostila de programacao em c (pag. 113)
int** alocar_matriz_real(int m, int n)
{
	int** v; /* ponteiro para a matriz */
	int i; /* vari�vel auxiliar */
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
int** liberar_matriz_real(int m, int n, int** v)
{
	int i; /* vari�vel auxiliar */
	if (v == NULL) return (NULL);
	if (m < 1 || n < 1) { /* verifica par�metros recebidos */
		printf("** Erro: Parametro invalido **\n");
		return (v);
	}
	for (i = 0; i < m; i++) free(v[i]); /* libera as linhas da matriz */
	free(v); /* libera a matriz (vetor de ponteiros) */
	return (NULL); /* retorna um ponteiro nulo */
}

// funcao para preencher a matriz com numeros aleatorios
void preencher_matriz(int** matriz, int altura, int largura) {
	int i, j;
	// inicializa a seed para a geracao de numeros aleatorios
	srand(SEED);

	for (i = 0; i < altura; i++) {
		for (j = 0; j < largura; j++) matriz[i][j] = rand() % MAX_SIZE_MATRIZ;
	}
}

// funcao de busca serial
void busca_serial() {
	// reseta o contador de primos
	primos_cont = 0;
	
	for (int i = 0; i < MATRIZ_ALTURA; i++) {
		for (int j = 0; j < MATRIZ_LARGURA; j++) {
			if (eh_primo(matriz[i][j])) primos_cont++;			
		}
	}
}

// a funcao rodada por cada thread
void* thread_busca(void* diov) {
	// identificador do macrobloco atual
	int macrobloco_atual;
	int primos_cont_macrobloco;
	
	while (1) {
		
		// verifica se ja processou todos os macroblocos
		if (macrobloco_proximo > qnt_macrobloco) {
			pthread_mutex_unlock(&mutex_macrobloco); 
			break;
		}

		// pega o proximo macrobloco a ser processado (zona critica)
		pthread_mutex_lock(&mutex_macrobloco);
		macrobloco_atual = macrobloco_proximo++;
		pthread_mutex_unlock(&mutex_macrobloco);

		// calcula os limites do macrobloco atual
		int linha_inicio = ((macrobloco_atual - 1) / colunas_macrobloco) * MACROBLOCO_ALTURA;
		int linha_fim = linha_inicio + MACROBLOCO_ALTURA - 1;
		int coluna_inicio = ((macrobloco_atual - 1) % colunas_macrobloco) * MACROBLOCO_LARGURA;
		int coluna_fim = coluna_inicio + MACROBLOCO_LARGURA - 1;

		// verifica se os limites estao dentro da matriz (caso contrario, erro de acesso a memoria)
		if (linha_inicio < 0 || linha_inicio >= MATRIZ_ALTURA ||
			linha_fim < 0 || linha_fim >= MATRIZ_ALTURA ||
			coluna_inicio < 0 || coluna_inicio >= MATRIZ_LARGURA ||
			coluna_fim < 0 || coluna_fim >= MATRIZ_LARGURA) 
		{
			pthread_mutex_lock(&mutex_erro_printf);
			printf("Error: violacao de acesso ao ler local (provavel que o a divisao da matriz por macroblocos nao seja exata)");
			pthread_mutex_unlock(&mutex_erro_printf);
			exit(EXIT_FAILURE);
		}

		// percorre o macrobloco atual contando os numeros primos
		primos_cont_macrobloco = 0;
		for (int i = linha_inicio; i <= linha_fim; i++) {
			for (int j = coluna_inicio; j <= coluna_fim; j++) {
				if (eh_primo(matriz[i][j])) {
					// atualiza o contador de primos do macrobloco
					primos_cont_macrobloco++;
				}
			}
		}
		pthread_mutex_lock(&mutex_primos);
		primos_cont += primos_cont_macrobloco;
		pthread_mutex_unlock(&mutex_primos);
	}

	pthread_exit(NULL);
}

// funcao de busca paralela
void busca_paralela() {
	// reseta o contador de primos
	primos_cont = 0;
	pthread_t threads[NUM_THREADS];

	// calcula a quantidade de macroblocos
	qnt_macrobloco = (int)(ceil((double)(MATRIZ_ALTURA * MATRIZ_LARGURA) / (MACROBLOCO_ALTURA * MACROBLOCO_LARGURA)));
	// calcula a quantidade de linhas e colunas de macroblocos
	linhas_macrobloco = (int)(ceil((double)MATRIZ_ALTURA / MACROBLOCO_ALTURA));
	colunas_macrobloco = (int)(ceil((double)MATRIZ_LARGURA / MACROBLOCO_LARGURA));

	// inicializa mutexes
	pthread_mutex_init(&mutex_primos, NULL);
	pthread_mutex_init(&mutex_macrobloco, NULL);
	pthread_mutex_init(&mutex_erro_printf, NULL);

	// cria as threads para rodarem a funcao thread_busca()
	for (int i = 0; i < NUM_THREADS; i++) pthread_create(&threads[i], NULL, thread_busca, NULL);
	for (int i = 0; i < NUM_THREADS; i++) pthread_join(threads[i], NULL);

	// destroi mutexes
	pthread_mutex_destroy(&mutex_primos);
	pthread_mutex_destroy(&mutex_macrobloco);
	pthread_mutex_destroy(&mutex_erro_printf);
}

// funcao principal
int main() {
	clock_t timer_inicio, timer_fim;
	double tempo_serial, tempo_paralelo, speedup;
	int primos_cont_serial;

	printf("Alocando e preenchendo matriz... ");
	matriz = alocar_matriz_real(MATRIZ_ALTURA, MATRIZ_LARGURA);
	preencher_matriz(matriz, MATRIZ_ALTURA, MATRIZ_LARGURA);
	printf("%sok!%s\n", GREEN, RESET);

	printf("Iniciando busca serial... ");
	timer_inicio = clock();
	busca_serial();
	printf("%sok!%s\n", GREEN, RESET);
	timer_fim = clock();
	tempo_serial = ((double)(timer_fim - timer_inicio)) / CLOCKS_PER_SEC;
	primos_cont_serial = primos_cont;
	//printf("Total de numeros primos encontrados: %d\n", primos_cont);

	printf("Iniciando busca paralela... ");
	timer_inicio = clock();
	busca_paralela();
	printf("%sok!%s\n", GREEN, RESET);
	timer_fim = clock();
	tempo_paralelo = ((double)(timer_fim - timer_inicio)) / CLOCKS_PER_SEC;
	//printf("Total de numeros primos encontrados: %d\n", primos_cont);

	printf("%s\nExecucao concluida com sucesso!%s\n\n", GREEN, RESET);

	speedup = tempo_serial / tempo_paralelo;
	printf("%s========================================%s\n", BOLD, RESET);
	printf("%s   RESULTADOS DA EXECUCAO DO PROGRAMA%s\n", GREEN, RESET);
	printf("%s========================================%s\n\n", BOLD, RESET);

	printf("%s-----------------------------------------%s\n", GRAY, RESET);
	printf("%s          Quantidade de primos%s\n", GREEN, RESET);
	printf("%s-----------------------------------------%s\n", GRAY, RESET);
	printf("Serial	                   %d\n", primos_cont_serial);
	printf("Paralelo                   %d\n", primos_cont);

	printf("%s-----------------------------------------%s\n", GRAY, RESET);
	printf("%s                  Tempo%s\n", GREEN, RESET);
	printf("%s-----------------------------------------%s\n", GRAY, RESET);
	printf("Serial	                   %lfs\n", tempo_serial);
	printf("Paralelo                   %lfs\n", tempo_paralelo);
	printf("Speedup	                   %s%lfx %s\n", GREEN, speedup, RESET);
	printf("%s-----------------------------------------%s\n", GRAY, RESET);

	/*printf("Tempo serial: %lf segundos\n", tempo_serial);
	printf("Tempo paralelo: %lf segundos\n", tempo_paralelo);
	printf("Speedup: %lf\n", speedup);*/

	printf("%s              Configuracoes%s\n", GREEN, RESET);
	printf("%s-----------------------------------------%s\n", GRAY, RESET);
	printf("Threads utilizadas         %d\n", NUM_THREADS);
	printf("Macroblocos processados    %d\n", qnt_macrobloco);
	printf("%s-----------------------------------------%s\n\n", GRAY, RESET);

	//printf("\nQuantidade de macroblocos: %d\nQuantidade de threads: %d\n", qnt_macrobloco, NUM_THREADS);

	liberar_matriz_real(MATRIZ_ALTURA, MATRIZ_LARGURA, matriz);
}
