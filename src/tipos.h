#ifndef TIPOS_H
#define TIPOS_H

/* Tamanho maximo de campos de texto */
#define MAX_CAMPO     256
/* Limite de registros carregados do CSV */
#define MAX_REGISTROS 50000
/* Tamanho da tabela hash (numero primo > 27 estados) */
#define HASH_SIZE     53
/* Limite de resultados por consulta */
#define MAX_RESULTADOS 500

typedef struct {
    char regiao[8];
    char estado[4];
    char municipio[MAX_CAMPO];
    char revenda[MAX_CAMPO];
    char cnpj[32];
    char rua[MAX_CAMPO];
    char numero[16];
    char complemento[MAX_CAMPO];
    char bairro[MAX_CAMPO];
    char cep[16];
    char produto[32];
    char data[16];
    char valor_venda[16];
    char valor_compra[16];
    char unidade[16];
    char bandeira[64];
} Registro;

/* Resultado comum às rotinas de busca (liberar campo registros com free) */
typedef struct {
    Registro **registros;
    int        count;
    int        total;
} ResultadoBusca;

#endif /* TIPOS_H */
