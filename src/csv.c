#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csv.h"
#include "tipos.h"

/* Remove \r, \n e espacos do final da string */
static void trim_fim(char *s) {
    int len = (int)strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r' ||
                       s[len - 1] == ' '  || s[len - 1] == '\t')) {
        s[--len] = '\0';
    }
}

/* Remove espacos do inicio da string */
static void trim_inicio(char *s) {
    int i = 0;
    while (s[i] == ' ' || s[i] == '\t') i++;
    if (i > 0) {
        int j = 0;
        while (s[i]) s[j++] = s[i++];
        s[j] = '\0';
    }
}

/* Converte string para maiusculas (ASCII) */
static void para_maiusculas(char *s) {
    for (int i = 0; s[i]; i++) {
        if (s[i] >= 'a' && s[i] <= 'z')
            s[i] = (char)(s[i] - 32);
    }
}

/*
 * Faz o parse de uma linha CSV separada por ';'.
 * Preenche o registro reg com os campos encontrados.
 * Retorna 1 em sucesso, 0 se a linha for invalida.
 */
static int parse_linha(char *linha, Registro *reg) {
    /* Array de ponteiros para inicio de cada campo */
    char *campos[16];
    int n = 0;
    char *p = linha;

    /* Separa a linha nos delimitadores ';' */
    while (n < 16) {
        campos[n++] = p;
        while (*p && *p != ';') p++;
        if (*p == ';') {
            *p = '\0';
            p++;
        } else {
            break;
        }
    }

    /* Linha incompleta (menos de 16 campos) */
    if (n < 16) return 0;

    /* Copia e limpa cada campo */
    trim_fim(campos[0]);  trim_inicio(campos[0]);
    trim_fim(campos[1]);  trim_inicio(campos[1]);
    trim_fim(campos[2]);  trim_inicio(campos[2]);
    trim_fim(campos[3]);  trim_inicio(campos[3]);
    trim_fim(campos[4]);  trim_inicio(campos[4]);
    trim_fim(campos[5]);  trim_inicio(campos[5]);
    trim_fim(campos[6]);  trim_inicio(campos[6]);
    trim_fim(campos[7]);  trim_inicio(campos[7]);
    trim_fim(campos[8]);  trim_inicio(campos[8]);
    trim_fim(campos[9]);  trim_inicio(campos[9]);
    trim_fim(campos[10]); trim_inicio(campos[10]);
    trim_fim(campos[11]); trim_inicio(campos[11]);
    trim_fim(campos[12]); trim_inicio(campos[12]);
    trim_fim(campos[13]); trim_inicio(campos[13]);
    trim_fim(campos[14]); trim_inicio(campos[14]);
    trim_fim(campos[15]); trim_inicio(campos[15]);

    strncpy(reg->regiao,      campos[0],  sizeof(reg->regiao)      - 1);
    strncpy(reg->estado,      campos[1],  sizeof(reg->estado)      - 1);
    strncpy(reg->municipio,   campos[2],  sizeof(reg->municipio)   - 1);
    strncpy(reg->revenda,     campos[3],  sizeof(reg->revenda)     - 1);
    strncpy(reg->cnpj,        campos[4],  sizeof(reg->cnpj)        - 1);
    strncpy(reg->rua,         campos[5],  sizeof(reg->rua)         - 1);
    strncpy(reg->numero,      campos[6],  sizeof(reg->numero)      - 1);
    strncpy(reg->complemento, campos[7],  sizeof(reg->complemento) - 1);
    strncpy(reg->bairro,      campos[8],  sizeof(reg->bairro)      - 1);
    strncpy(reg->cep,         campos[9],  sizeof(reg->cep)         - 1);
    strncpy(reg->produto,     campos[10], sizeof(reg->produto)     - 1);
    strncpy(reg->data,        campos[11], sizeof(reg->data)        - 1);
    strncpy(reg->valor_venda, campos[12], sizeof(reg->valor_venda) - 1);
    strncpy(reg->valor_compra,campos[13], sizeof(reg->valor_compra)- 1);
    strncpy(reg->unidade,     campos[14], sizeof(reg->unidade)     - 1);
    strncpy(reg->bandeira,    campos[15], sizeof(reg->bandeira)    - 1);

    /* Normaliza campos de busca para maiusculas */
    para_maiusculas(reg->estado);
    para_maiusculas(reg->municipio);
    para_maiusculas(reg->produto);
    para_maiusculas(reg->bandeira);
    para_maiusculas(reg->regiao);

    /* Descarta registros sem estado ou municipio */
    if (reg->estado[0] == '\0' || reg->municipio[0] == '\0') return 0;

    return 1;
}

Registro *carregar_csv(const char *caminho, int *total) {
    FILE *f = fopen(caminho, "r");
    if (!f) {
        fprintf(stderr, "[csv] Nao foi possivel abrir: %s\n", caminho);
        return NULL;
    }

    Registro *registros = (Registro *)malloc(MAX_REGISTROS * sizeof(Registro));
    if (!registros) {
        fclose(f);
        return NULL;
    }

    char linha[2048];
    int count = 0;
    int primeira_linha = 1;

    while (fgets(linha, sizeof(linha), f)) {
        if (primeira_linha) {
            /* Remove BOM UTF-8 (EF BB BF) se presente */
            char *inicio = linha;
            if ((unsigned char)inicio[0] == 0xEF &&
                (unsigned char)inicio[1] == 0xBB &&
                (unsigned char)inicio[2] == 0xBF) {
                /* Shift left para remover os 3 bytes do BOM */
                int i = 0;
                while (inicio[i + 3]) { inicio[i] = inicio[i + 3]; i++; }
                inicio[i] = '\0';
            }
            primeira_linha = 0;
            continue; /* pula cabecalho */
        }

        if (count >= MAX_REGISTROS) {
            fprintf(stderr, "[csv] Limite de %d registros atingido.\n", MAX_REGISTROS);
            break;
        }

        /* Inicializa o registro com zeros */
        Registro *reg = &registros[count];
        memset(reg, 0, sizeof(Registro));

        if (parse_linha(linha, reg)) {
            count++;
        }
    }

    fclose(f);
    *total = count;
    printf("[csv] %d registros carregados de '%s'\n", count, caminho);
    return registros;
}
