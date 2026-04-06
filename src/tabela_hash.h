#ifndef TABELA_HASH_H
#define TABELA_HASH_H

#include "tipos.h"

typedef struct BucketNode {
    char estado[4];           /* chave: sigla do estado (ex: "SP") */
    Registro **registros;     /* array de ponteiros para Registro  */
    int count;                /* quantidade de registros atual     */
    int capacidade;           /* capacidade alocada do array       */
    struct BucketNode *proximo; /* encadeamento para colisoes      */
} BucketNode;


typedef struct {
    BucketNode *buckets[HASH_SIZE]; /* vetor de slots */
    int total_estados;              /* quantos estados distintos foram inseridos */
} TabelaHash;

/* Aloca e inicializa uma nova tabela hash vazia */
TabelaHash *criar_tabela_hash(void);

/* Insere um registro na tabela, indexado pelo campo estado */
void inserir_tabela_hash(TabelaHash *tabela, Registro *reg);

/* Busca o bucket (no) correspondente ao estado. Retorna NULL se nao encontrar */
BucketNode *buscar_bucket(TabelaHash *tabela, const char *estado);

/* Ordena os arrays de registros de todos os buckets por municipio (quicksort) */
void ordenar_buckets(TabelaHash *tabela);

/* Libera toda a memoria da tabela hash */
void liberar_tabela_hash(TabelaHash *tabela);

/* Coleta ponteiros para todos os BucketNodes em arr[], retorna a quantidade */
int listar_buckets(TabelaHash *tabela, BucketNode **arr, int max);

#endif /* TABELA_HASH_H */
