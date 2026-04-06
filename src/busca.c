#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "busca.h"

/* ------------------------------------------------------------------ */
/*  Auxiliares                                                          */
/* ------------------------------------------------------------------ */

/* Troca dois ponteiros de Registro */
static void trocar(Registro **a, Registro **b) {
    Registro *tmp = *a;
    *a = *b;
    *b = tmp;
}

/*
 * Verifica se 'str' comeca com 'prefixo'.
 * Ambos devem estar em maiusculas (normalizados pelo CSV parser).
 * Retorna 1 se sim, 0 caso contrario.
 */
static int comeca_com(const char *str, const char *prefixo) {
    int i = 0;
    while (prefixo[i] != '\0') {
        if (str[i] != prefixo[i]) return 0;
        i++;
    }
    return 1;
}

/* ------------------------------------------------------------------ */
/*  Quicksort por municipio                                             */
/* ------------------------------------------------------------------ */

/*
 * Particao de Lomuto: escolhe arr[high] como pivo e reorganiza o
 * subarray arr[low..high] de modo que todos os elementos < pivo
 * fiquem a esquerda e todos > pivo fiquem a direita.
 *
 * Retorna o indice final do pivo.
 */
static int particionar(Registro **arr, int low, int high) {
    Registro *pivo = arr[high];
    int i = low - 1;

    for (int j = low; j < high; j++) {
        /* Compara pelo campo municipio */
        if (strcmp(arr[j]->municipio, pivo->municipio) <= 0) {
            i++;
            trocar(&arr[i], &arr[j]);
        }
    }
    trocar(&arr[i + 1], &arr[high]);
    return i + 1;
}

void quicksort_municipio(Registro **arr, int low, int high) {
    if (low >= high) return; /* caso base: subarray de 0 ou 1 elemento */

    int p = particionar(arr, low, high);

    /* Recursao nas duas metades */
    quicksort_municipio(arr, low, p - 1);
    quicksort_municipio(arr, p + 1, high);
}

/* ------------------------------------------------------------------ */
/*  Busca binaria por prefixo de municipio                             */
/* ------------------------------------------------------------------ */

int busca_binaria_prefixo(Registro **arr, int n, const char *prefixo) {
    if (!prefixo || prefixo[0] == '\0') return 0; /* prefixo vazio: todos */

    int low    = 0;
    int high   = n - 1;
    int result = -1;
    int plen   = (int)strlen(prefixo);

    while (low <= high) {
        int mid = low + (high - low) / 2; /* evita overflow de (low+high)/2 */

        /* Compara apenas os primeiros plen caracteres */
        int cmp = strncmp(arr[mid]->municipio, prefixo, plen);

        if (cmp == 0) {
            result = mid;       /* encontrou, mas continua buscando a esquerda */
            high   = mid - 1;  /* para achar a PRIMEIRA ocorrencia */
        } else if (cmp < 0) {
            low = mid + 1;      /* prefixo e maior: busca na metade direita */
        } else {
            high = mid - 1;     /* prefixo e menor: busca na metade esquerda */
        }
    }

    return result; /* -1 se nao encontrado */
}

/* ------------------------------------------------------------------ */
/*  Funcao principal de busca                                           */
/* ------------------------------------------------------------------ */
/*  Conversao de datas para inteiro YYYYMMDD para comparacao          */
/* ------------------------------------------------------------------ */

/*
 * Converte data no formato do CSV "DD/MM/YYYY" para inteiro YYYYMMDD.
 * Ex: "02/02/2026" -> 20260202
 */
static int csv_data_para_int(const char *data) {
    int i;
    /* Verifica se tem ao menos 10 chars no formato DD/MM/YYYY */
    for (i = 0; i < 10; i++) if (data[i] == '\0') return 0;
    if (data[2] != '/' || data[5] != '/') return 0;

    int d = (data[0] - '0') * 10 + (data[1] - '0');
    int m = (data[3] - '0') * 10 + (data[4] - '0');
    int y = (data[6] - '0') * 1000 + (data[7] - '0') * 100 +
            (data[8] - '0') * 10  + (data[9] - '0');
    return y * 10000 + m * 100 + d;
}

/*
 * Converte data no formato HTML "YYYY-MM-DD" para inteiro YYYYMMDD.
 * Ex: "2026-02-01" -> 20260201
 */
static int html_data_para_int(const char *data) {
    int i;
    for (i = 0; i < 10; i++) if (data[i] == '\0') return 0;
    if (data[4] != '-' || data[7] != '-') return 0;

    int y = (data[0] - '0') * 1000 + (data[1] - '0') * 100 +
            (data[2] - '0') * 10  + (data[3] - '0');
    int m = (data[5] - '0') * 10 + (data[6] - '0');
    int d = (data[8] - '0') * 10 + (data[9] - '0');
    return y * 10000 + m * 100 + d;
}

/* ------------------------------------------------------------------ */

ResultadoBusca buscar(TabelaHash *tabela,
                      const char *estado,
                      const char *municipio,
                      const char *produto,
                      const char *bandeira,
                      const char *data_inicio,
                      const char *data_fim) {

    ResultadoBusca resultado;
    resultado.count     = 0;
    resultado.total     = 0;
    resultado.registros = NULL;

    /* Pre-converte limites de data para inteiros (0 = sem filtro) */
    int di = (data_inicio && data_inicio[0]) ? html_data_para_int(data_inicio) : 0;
    int df = (data_fim    && data_fim[0])    ? html_data_para_int(data_fim)    : 0;

    /* Coleta os buckets a percorrer */
    BucketNode *buckets[64];
    int nb = 0;

    if (estado != NULL && estado[0] != '\0') {
        /* --- PASSO 1: Hash lookup O(1) --- */
        BucketNode *b = buscar_bucket(tabela, estado);
        if (b != NULL) {
            buckets[0] = b;
            nb = 1;
        }
        /* Estado nao encontrado: resultado vazio */
    } else {
        /* Sem filtro de estado: coleta todos os buckets */
        nb = listar_buckets(tabela, buckets, 64);
    }

    if (nb == 0) return resultado;

    /* Aloca array de resultado */
    int capacidade = 1024;
    resultado.registros = (Registro **)malloc(capacidade * sizeof(Registro *));
    if (!resultado.registros) return resultado;

    int tem_municipio = (municipio != NULL && municipio[0] != '\0');
    int tem_produto   = (produto   != NULL && produto[0]   != '\0');
    int tem_bandeira  = (bandeira  != NULL && bandeira[0]  != '\0');
    int tem_data      = (di > 0 || df > 0);

    for (int b = 0; b < nb; b++) {
        BucketNode *bucket = buckets[b];
        int inicio = 0;
        int fim    = bucket->count;

        if (tem_municipio) {
            /* --- PASSO 2: Busca binaria pelo prefixo O(log n) --- */
            int idx = busca_binaria_prefixo(bucket->registros,
                                            bucket->count, municipio);
            if (idx < 0) continue; /* municipio nao existe neste estado */

            inicio = idx;

            /*
             * Avanca fim ate acabar o prefixo.
             * Como o array esta ordenado, todos os registros com aquele
             * prefixo estao contiguos a partir de 'inicio'.
             */
            fim = inicio;
            while (fim < bucket->count &&
                   comeca_com(bucket->registros[fim]->municipio, municipio)) {
                fim++;
            }
        }

        /* --- PASSO 3: Varredura linear com filtros O(m) --- */
        for (int i = inicio; i < fim; i++) {
            Registro *r = bucket->registros[i];

            /* Filtro por produto (comparacao exata) */
            if (tem_produto && strcmp(r->produto, produto) != 0) continue;

            /* Filtro por bandeira (comparacao exata) */
            if (tem_bandeira && strcmp(r->bandeira, bandeira) != 0) continue;

            /* Filtro por intervalo de data */
            if (tem_data) {
                int rd = csv_data_para_int(r->data);
                if (rd == 0) continue;              /* data invalida no registro */
                if (di > 0 && rd < di) continue;   /* anterior ao inicio        */
                if (df > 0 && rd > df) continue;   /* posterior ao fim          */
            }

            resultado.total++;

            /* Limita o array de saida a MAX_RESULTADOS */
            if (resultado.count >= MAX_RESULTADOS) continue;

            /* Expande array se necessario */
            if (resultado.count >= capacidade) {
                capacidade *= 2;
                resultado.registros = (Registro **)realloc(
                    resultado.registros, capacidade * sizeof(Registro *));
                if (!resultado.registros) return resultado;
            }

            resultado.registros[resultado.count++] = r;
        }
    }

    return resultado;
}
