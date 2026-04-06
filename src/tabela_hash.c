#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tabela_hash.h"
#include "busca.h"   /* quicksort_municipio */

/*
 * Funcao hash djb2 adaptada para siglas de estado (strings curtas).
 *
 * Complexidade: O(k) onde k e o comprimento da chave (2 chars = O(1)).
 * Distribui uniformemente as 27 siglas de estados brasileiros.
 */
static unsigned int hash_estado(const char *estado) {
    unsigned int h = 5381;
    int i = 0;
    while (estado[i] != '\0') {
        h = ((h << 5) + h) + (unsigned char)estado[i]; /* h * 33 + c */
        i++;
    }
    return h % HASH_SIZE;
}

TabelaHash *criar_tabela_hash(void) {
    TabelaHash *t = (TabelaHash *)calloc(1, sizeof(TabelaHash));
    if (!t) {
        fprintf(stderr, "[hash] Falha ao alocar tabela hash\n");
        return NULL;
    }
    return t;
}

void inserir_tabela_hash(TabelaHash *tabela, Registro *reg) {
    if (!tabela || !reg || reg->estado[0] == '\0') return;

    unsigned int idx = hash_estado(reg->estado);
    BucketNode *node = tabela->buckets[idx];

    /* Percorre a lista encadeada procurando o bucket do estado */
    while (node != NULL) {
        if (strcmp(node->estado, reg->estado) == 0) break;
        node = node->proximo;
    }

    /* Estado ainda nao esta na tabela: cria novo bucket */
    if (node == NULL) {
        node = (BucketNode *)calloc(1, sizeof(BucketNode));
        if (!node) return;

        node->estado[0] = reg->estado[0];
        node->estado[1] = reg->estado[1];
        node->estado[2] = reg->estado[2];
        node->estado[3] = '\0';
        node->capacidade = 2048;
        node->registros  = (Registro **)malloc(node->capacidade * sizeof(Registro *));
        if (!node->registros) { free(node); return; }

        /* Insere na cabeca da lista encadeada deste slot */
        node->proximo        = tabela->buckets[idx];
        tabela->buckets[idx] = node;
        tabela->total_estados++;
    }

    /* Expande o array se necessario (amortizado O(1)) */
    if (node->count >= node->capacidade) {
        node->capacidade *= 2;
        node->registros = (Registro **)realloc(
            node->registros, node->capacidade * sizeof(Registro *));
        if (!node->registros) return;
    }

    node->registros[node->count++] = reg;
}

BucketNode *buscar_bucket(TabelaHash *tabela, const char *estado) {
    if (!tabela || !estado) return NULL;

    unsigned int idx = hash_estado(estado);
    BucketNode *node = tabela->buckets[idx];

    /* O(1) no caso medio; O(colisoes) no pior caso */
    while (node != NULL) {
        if (strcmp(node->estado, estado) == 0) return node;
        node = node->proximo;
    }
    return NULL;
}

void ordenar_buckets(TabelaHash *tabela) {
    if (!tabela) return;
    int ordenados = 0;
    for (int i = 0; i < HASH_SIZE; i++) {
        BucketNode *node = tabela->buckets[i];
        while (node != NULL) {
            /* Quicksort nos registros do bucket por municipio */
            if (node->count > 1) {
                quicksort_municipio(node->registros, 0, node->count - 1);
            }
            ordenados++;
            node = node->proximo;
        }
    }
    printf("[hash] %d buckets ordenados por municipio\n", ordenados);
}

void liberar_tabela_hash(TabelaHash *tabela) {
    if (!tabela) return;
    for (int i = 0; i < HASH_SIZE; i++) {
        BucketNode *node = tabela->buckets[i];
        while (node != NULL) {
            BucketNode *proximo = node->proximo;
            free(node->registros);
            free(node);
            node = proximo;
        }
    }
    free(tabela);
}

int listar_buckets(TabelaHash *tabela, BucketNode **arr, int max) {
    if (!tabela || !arr) return 0;
    int n = 0;
    for (int i = 0; i < HASH_SIZE && n < max; i++) {
        BucketNode *node = tabela->buckets[i];
        while (node != NULL && n < max) {
            arr[n++] = node;
            node = node->proximo;
        }
    }
    return n;
}
