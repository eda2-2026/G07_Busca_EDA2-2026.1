#include <stdio.h>
#include <stdlib.h>
#include "csv.h"
#include "tabela_hash.h"
#include "busca.h"
#include "servidor.h"
#include "busca_interpolacao.h"

#define ALUNO1_NOME      "VICTOR LEANDRO ROCHA DE ASSIS"
#define ALUNO1_MATRICULA "222021826"
#define ALUNO2_NOME      "PEDRO LUIZ FONSECA DA SILVA"
#define ALUNO2_MATRICULA "231036980"

#define CSV_PADRAO "02-cados-abertos-preco-gasolina-etanol.csv"
#define PORTA_PADRAO 8080

static void imprimir_cabecalho(void) {
    printf("============================================================\n");
    printf("  Busca de Precos de Combustiveis - EDA2 2026.1\n");
    printf("  Grupo 07\n");
    printf("  %-20s  Matricula: %s\n", ALUNO1_NOME, ALUNO1_MATRICULA);
    printf("  %-20s  Matricula: %s\n", ALUNO2_NOME, ALUNO2_MATRICULA);
    printf("============================================================\n\n");
}

int main(int quantidade_argumentos, char *argumentos[]) {
    imprimir_cabecalho();

    const char *caminho_csv = CSV_PADRAO;
    int porta               = PORTA_PADRAO;

    if (quantidade_argumentos >= 2) caminho_csv = argumentos[1];
    if (quantidade_argumentos >= 3) porta       = atoi(argumentos[2]);

    printf("[main] Carregando: %s\n", caminho_csv);
    int total = 0;
    Registro *registros = carregar_csv(caminho_csv, &total);
    if (!registros || total == 0) {
        fprintf(stderr, "[main] Falha ao carregar CSV. Encerrando.\n");
        return 1;
    }

    printf("[main] Construindo tabela hash...\n");
    TabelaHash *tabela = criar_tabela_hash();
    if (!tabela) {
        free(registros);
        return 1;
    }
    for (int indice = 0; indice < total; indice++) {
        inserir_tabela_hash(tabela, &registros[indice]);
    }
    printf("[main] %d estados indexados.\n", tabela->total_estados);

    printf("[main] Ordenando municipios (Quicksort)...\n");
    ordenar_buckets(tabela);

    printf("[main] Construindo indice por data (interpolacao)...\n");
    IndicePorData *indice_data = indice_por_data_criar(registros, total);
    if (!indice_data) {
        fprintf(stderr, "[main] Falha ao criar indice por data.\n");
        liberar_tabela_hash(tabela);
        free(registros);
        return 1;
    }

    printf("[main] Pronto. Acesse: http://localhost:%d\n\n", porta);
    iniciar_servidor(tabela, registros, total, indice_data, porta);

    indice_por_data_liberar(indice_data);
    liberar_tabela_hash(tabela);
    free(registros);
    return 0;
}
