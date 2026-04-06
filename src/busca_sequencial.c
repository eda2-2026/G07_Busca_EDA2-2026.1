#include <stdlib.h>
#include <string.h>
#include "busca_sequencial.h"

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

ResultadoBusca buscar_sequencial(Registro *base,
                                 int total,
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

    if (!base || total <= 0) return resultado;

    int di = (data_inicio && data_inicio[0]) ? html_data_para_int(data_inicio) : 0;
    int df = (data_fim && data_fim[0]) ? html_data_para_int(data_fim) : 0;

    int capacidade = 1024;
    resultado.registros = (Registro **)malloc(capacidade * sizeof(Registro *));
    if (!resultado.registros) return resultado;

    int tem_estado    = (estado != NULL && estado[0] != '\0');
    int tem_municipio = (municipio != NULL && municipio[0] != '\0');
    int tem_produto   = (produto != NULL && produto[0] != '\0');
    int tem_bandeira  = (bandeira != NULL && bandeira[0] != '\0');
    int tem_data      = (di > 0 || df > 0);

    for (int i = 0; i < total; i++) {
        Registro *r = &base[i];

        if (tem_estado && strcmp(r->estado, estado) != 0) continue;
        if (tem_municipio && !comeca_com(r->municipio, municipio)) continue;
        if (tem_produto && strcmp(r->produto, produto) != 0) continue;
        if (tem_bandeira && strcmp(r->bandeira, bandeira) != 0) continue;

        if (tem_data) {
            int rd = csv_data_para_int(r->data);
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
