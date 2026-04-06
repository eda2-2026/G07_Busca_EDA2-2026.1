#ifndef BUSCA_INTERPOLACAO_H
#define BUSCA_INTERPOLACAO_H

#include "tipos.h"

/*
 * Indice auxiliar: ponteiros para registros ordenados por data (YYYYMMDD).
 * Construido na inicializacao com ordenacao mergesort propria deste modulo.
 */
typedef struct {
    Registro **itens;
    int n;
} IndicePorData;

IndicePorData *indice_por_data_criar(Registro *base, int total);
void indice_por_data_liberar(IndicePorData *ind);

/*
 * Busca por interpolacao sobre o eixo temporal:
 * com filtro de data, localiza o subintervalo [inicio, fim] via
 * busca por interpolacao em chaves numericas de data; depois varre
 * linearmente aplicando estado, prefixo de municipio, produto e bandeira.
 *
 * Sem filtro de data, o intervalo e o vetor inteiro (varredura linear O(n)).
 * resultado.registros deve ser liberado com free().
 */
ResultadoBusca buscar_interpolacao(IndicePorData *ind,
                                   const char *estado,
                                   const char *municipio,
                                   const char *produto,
                                   const char *bandeira,
                                   const char *data_inicio,
                                   const char *data_fim);

#endif /* BUSCA_INTERPOLACAO_H */
