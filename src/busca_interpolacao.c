#include <stdlib.h>
#include <string.h>
#include "busca_interpolacao.h"

static int comeca_com(const char *str, const char *prefixo) {
    int i = 0;
    while (prefixo[i] != '\0') {
        if (str[i] != prefixo[i]) return 0;
        i++;
    }
    return 1;
}

static int csv_data_para_int(const char *data) {
    int i;
    for (i = 0; i < 10; i++)
        if (data[i] == '\0') return 0;
    if (data[2] != '/' || data[5] != '/') return 0;

    int d = (data[0] - '0') * 10 + (data[1] - '0');
    int m = (data[3] - '0') * 10 + (data[4] - '0');
    int y = (data[6] - '0') * 1000 + (data[7] - '0') * 100 +
            (data[8] - '0') * 10 + (data[9] - '0');
    return y * 10000 + m * 100 + d;
}

static int html_data_para_int(const char *data) {
    int i;
    for (i = 0; i < 10; i++)
        if (data[i] == '\0') return 0;
    if (data[4] != '-' || data[7] != '-') return 0;

    int y = (data[0] - '0') * 1000 + (data[1] - '0') * 100 +
            (data[2] - '0') * 10 + (data[3] - '0');
    int m = (data[5] - '0') * 10 + (data[6] - '0');
    int d = (data[8] - '0') * 10 + (data[9] - '0');
    return y * 10000 + m * 100 + d;
}

static int chave_data_reg(const Registro *r) {
    return csv_data_para_int(r->data);
}

/* ------------------------------------------------------------------ */
/*  Mergesort por chave_data_reg (estavel)                              */
/* ------------------------------------------------------------------ */

static void merge_bloco(Registro **a, Registro **aux, int esq, int meio, int dir) {
    int i = esq, j = meio + 1, k = esq;
    while (i <= meio && j <= dir) {
        int ki = chave_data_reg(a[i]);
        int kj = chave_data_reg(a[j]);
        if (ki <= kj)
            aux[k++] = a[i++];
        else
            aux[k++] = a[j++];
    }
    while (i <= meio) aux[k++] = a[i++];
    while (j <= dir) aux[k++] = a[j++];
    for (i = esq; i <= dir; i++) a[i] = aux[i];
}

static void mergesort_interno(Registro **a, Registro **aux, int esq, int dir) {
    if (esq >= dir) return;
    int meio = esq + (dir - esq) / 2;
    mergesort_interno(a, aux, esq, meio);
    mergesort_interno(a, aux, meio + 1, dir);
    merge_bloco(a, aux, esq, meio, dir);
}

static int mergesort_por_data(Registro **a, int n) {
    if (n <= 1) return 0;
    Registro **aux = (Registro **)malloc((size_t)n * sizeof(Registro *));
    if (!aux) return -1;
    mergesort_interno(a, aux, 0, n - 1);
    free(aux);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Busca por interpolacao em chave numerica (data YYYYMMDD)           */
/* ------------------------------------------------------------------ */

/*
 * Menor indice i tal que chave(a[i]) >= alvo; retorna n se nao existir.
 */
static int primeiro_indice_geq(Registro **a, int n, int alvo) {
    if (n <= 0) return 0;
    int k0 = chave_data_reg(a[0]);
    int kn = chave_data_reg(a[n - 1]);
    if (alvo <= k0) return 0;
    if (alvo > kn) return n;

    int lo = 0, hi = n - 1;
    int resp = n;

    while (lo <= hi) {
        int klo = chave_data_reg(a[lo]);
        int khi = chave_data_reg(a[hi]);
        if (alvo <= klo) {
            resp = lo;
            break;
        }
        if (alvo > khi) break;

        if (khi == klo) {
            lo++;
            continue;
        }

        long long numer = (long long)(alvo - klo) * (hi - lo);
        int pos = lo + (int)(numer / (khi - klo));
        if (pos < lo) pos = lo;
        if (pos > hi) pos = hi;

        int km = chave_data_reg(a[pos]);
        if (km < alvo)
            lo = pos + 1;
        else {
            resp = pos;
            hi = pos - 1;
        }
    }
    return resp;
}

/*
 * Maior indice i tal que chave(a[i]) <= alvo; retorna -1 se nao existir.
 */
static int ultimo_indice_leq(Registro **a, int n, int alvo) {
    if (n <= 0) return -1;
    int k0 = chave_data_reg(a[0]);
    int kn = chave_data_reg(a[n - 1]);
    if (alvo < k0) return -1;
    if (alvo >= kn) return n - 1;

    int lo = 0, hi = n - 1;
    int resp = -1;

    while (lo <= hi) {
        int klo = chave_data_reg(a[lo]);
        int khi = chave_data_reg(a[hi]);
        if (alvo >= khi) {
            resp = hi;
            break;
        }
        if (alvo < klo) break;

        if (khi == klo) {
            hi--;
            continue;
        }

        long long numer = (long long)(alvo - klo) * (hi - lo);
        int pos = lo + (int)(numer / (khi - klo));
        if (pos < lo) pos = lo;
        if (pos > hi) pos = hi;

        int km = chave_data_reg(a[pos]);
        if (km > alvo)
            hi = pos - 1;
        else {
            resp = pos;
            lo = pos + 1;
        }
    }
    return resp;
}

IndicePorData *indice_por_data_criar(Registro *base, int total) {
    IndicePorData *ind = (IndicePorData *)calloc(1, sizeof(IndicePorData));
    if (!ind || total <= 0 || !base) {
        free(ind);
        return NULL;
    }

    ind->itens = (Registro **)malloc((size_t)total * sizeof(Registro *));
    if (!ind->itens) {
        free(ind);
        return NULL;
    }

    for (int i = 0; i < total; i++) ind->itens[i] = &base[i];
    ind->n = total;
    if (mergesort_por_data(ind->itens, ind->n) != 0) {
        free(ind->itens);
        free(ind);
        return NULL;
    }
    return ind;
}

void indice_por_data_liberar(IndicePorData *ind) {
    if (!ind) return;
    free(ind->itens);
    free(ind);
}

ResultadoBusca buscar_interpolacao(IndicePorData *ind,
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

    if (!ind || !ind->itens || ind->n <= 0) return resultado;

    int di = (data_inicio && data_inicio[0]) ? html_data_para_int(data_inicio) : 0;
    int df = (data_fim && data_fim[0]) ? html_data_para_int(data_fim) : 0;

    int inicio = 0;
    int fim = ind->n - 1;

    if (di > 0) inicio = primeiro_indice_geq(ind->itens, ind->n, di);
    if (df > 0) {
        int u = ultimo_indice_leq(ind->itens, ind->n, df);
        fim = u;
    }

    if (inicio > fim || inicio >= ind->n || fim < 0) return resultado;

    int capacidade = 1024;
    resultado.registros = (Registro **)malloc(capacidade * sizeof(Registro *));
    if (!resultado.registros) return resultado;

    int tem_estado    = (estado != NULL && estado[0] != '\0');
    int tem_municipio = (municipio != NULL && municipio[0] != '\0');
    int tem_produto   = (produto != NULL && produto[0] != '\0');
    int tem_bandeira  = (bandeira != NULL && bandeira[0] != '\0');
    int tem_data      = (di > 0 || df > 0);

    for (int i = inicio; i <= fim; i++) {
        Registro *r = ind->itens[i];

        if (tem_estado && strcmp(r->estado, estado) != 0) continue;
        if (tem_municipio && !comeca_com(r->municipio, municipio)) continue;
        if (tem_produto && strcmp(r->produto, produto) != 0) continue;
        if (tem_bandeira && strcmp(r->bandeira, bandeira) != 0) continue;

        if (tem_data) {
            int rd = chave_data_reg(r);
            if (rd == 0) continue;
            if (di > 0 && rd < di) continue;
            if (df > 0 && rd > df) continue;
        }

        resultado.total++;

        if (resultado.count >= MAX_RESULTADOS) continue;

        if (resultado.count >= capacidade) {
            capacidade *= 2;
            Registro **novo = (Registro **)realloc(
                resultado.registros, capacidade * sizeof(Registro *));
            if (!novo) return resultado;
            resultado.registros = novo;
        }

        resultado.registros[resultado.count++] = r;
    }

    return resultado;
}
