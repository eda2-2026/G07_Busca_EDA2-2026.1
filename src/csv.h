#ifndef CSV_H
#define CSV_H

#include "tipos.h"

/*
 * Carrega o arquivo CSV de precos de combustiveis.
 *
 * Parametros:
 *   caminho - caminho para o arquivo .csv
 *   total   - (saida) numero de registros carregados
 *
 * Retorno:
 *   Ponteiro para array de Registro alocado dinamicamente.
 *   O chamador e responsavel por liberar com free().
 *   Retorna NULL em caso de erro.
 */
Registro *carregar_csv(const char *caminho, int *total);

#endif /* CSV_H */
