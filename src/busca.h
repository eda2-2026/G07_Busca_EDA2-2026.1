#ifndef BUSCA_H
#define BUSCA_H

#include "tipos.h"
#include "tabela_hash.h"

/*
 * quicksort_municipio - ordena um array de ponteiros para Registro
 * pelo campo municipio em ordem lexicografica crescente.
 *
 * Algoritmo: Quicksort com particao de Lomuto.
 * Complexidade: O(n log n) medio, O(n²) pior caso.
 *
 * Parametros:
 *   arr  - array de ponteiros para Registro
 *   low  - indice inicial (inclusivo)
 *   high - indice final (inclusivo)
 */
void quicksort_municipio(Registro **arr, int low, int high);

/*
 * busca_binaria_prefixo - encontra o primeiro indice no array ordenado
 * cujo campo municipio comeca com o prefixo dado.
 *
 * Algoritmo: Busca binaria com reducao a esquerda para encontrar a
 * primeira ocorrencia.
 * Complexidade: O(log n).
 *
 * Parametros:
 *   arr     - array de ponteiros para Registro ordenado por municipio
 *   n       - tamanho do array
 *   prefixo - string prefixo a buscar (deve estar em maiusculas)
 *
 * Retorno:
 *   Indice da primeira ocorrencia, ou -1 se nao encontrado.
 */
int busca_binaria_prefixo(Registro **arr, int n, const char *prefixo);

/*
 * ResultadoBusca - resultado de uma consulta
 */
typedef struct {
    Registro **registros;   /* array de ponteiros para os registros encontrados */
    int        count;       /* quantidade de resultados retornados             */
    int        total;       /* total de matches (pode ser > count se limitado) */
} ResultadoBusca;

/*
 * buscar - funcao principal de busca combinada.
 *
 * Fluxo:
 *   1. Hash lookup por estado -> O(1)
 *   2. Busca binaria por prefixo de municipio -> O(log n)
 *   3. Varredura linear para filtrar produto, bandeira e datas -> O(m)
 *
 * Parametros (NULL ou "" significa "sem filtro"):
 *   tabela       - tabela hash pre-construida
 *   estado       - sigla do estado (ex: "SP")
 *   municipio    - prefixo do municipio (ex: "SAO")
 *   produto      - nome exato do produto (ex: "GASOLINA")
 *   bandeira     - nome exato da bandeira (ex: "VIBRA")
 *   data_inicio  - data inicial no formato "YYYY-MM-DD" (ex: "2026-02-01")
 *   data_fim     - data final no formato "YYYY-MM-DD"  (ex: "2026-02-28")
 *
 * Retorno: ResultadoBusca (campo registros deve ser liberado com free())
 */
ResultadoBusca buscar(TabelaHash *tabela,
                      const char *estado,
                      const char *municipio,
                      const char *produto,
                      const char *bandeira,
                      const char *data_inicio,
                      const char *data_fim);

#endif /* BUSCA_H */
