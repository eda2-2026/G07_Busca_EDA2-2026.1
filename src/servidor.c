#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Copia segura sem os warnings de truncacao do strncpy */
static void copia_safe(char *dst, const char *src, int max) {
    int i;
    for (i = 0; i < max - 1 && src[i] != '\0'; i++) dst[i] = src[i];
    dst[i] = '\0';
}

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef int socklen_t;
  #define CLOSE_SOCKET(s) closesocket(s)
  #define INIT_SOCKETS() do { WSADATA w; WSAStartup(MAKEWORD(2,2),&w); } while(0)
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <unistd.h>
  #define CLOSE_SOCKET(s) close(s)
  #define INIT_SOCKETS()
  typedef int SOCKET;
#endif

#include "servidor.h"
#include "busca.h"
#include "busca_sequencial.h"
#include "tipos.h"

static TabelaHash   *g_tabela;
static Registro     *g_registros;
static int           g_total_registros;
static IndicePorData *g_indice_data;

/* Diretorio dos arquivos do frontend (relativo ao CWD ao executar) */
#define FRONTEND_DIR "frontend/"

/* Tamanho do buffer de requisicao HTTP */
#define BUF_REQ  4096
/* Tamanho do buffer de resposta JSON (~2MB) */
#define BUF_RESP (2 * 1024 * 1024)

/* ------------------------------------------------------------------ */
/*  URL decode                                                          */
/* ------------------------------------------------------------------ */

static int hex_para_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

/* Decodifica %XX e '+' da query string no lugar (in-place) */
static void url_decode(char *s) {
    char *leitura  = s;
    char *escrita  = s;
    while (*leitura) {
        if (*leitura == '%' && leitura[1] && leitura[2]) {
            *escrita++ = (char)(hex_para_int(leitura[1]) * 16 +
                                hex_para_int(leitura[2]));
            leitura += 3;
        } else if (*leitura == '+') {
            *escrita++ = ' ';
            leitura++;
        } else {
            *escrita++ = *leitura++;
        }
    }
    *escrita = '\0';
}

/* Converte para maiusculas in-place */
static void para_maiusculas(char *s) {
    for (int i = 0; s[i]; i++) {
        if (s[i] >= 'a' && s[i] <= 'z') s[i] = (char)(s[i] - 32);
    }
}

/* ------------------------------------------------------------------ */
/*  Parser de query string                                              */
/* ------------------------------------------------------------------ */

/*
 * Extrai o valor do parametro 'chave' da query string 'qs'.
 * Escreve o resultado em 'saida' (max 'max' bytes).
 * Retorna 1 se encontrou, 0 caso contrario.
 */
static int get_param(const char *qs, const char *chave, char *saida, int max) {
    saida[0] = '\0';
    if (!qs || !qs[0]) return 0;

    int klen = (int)strlen(chave);
    const char *p = qs;

    while (*p) {
        /* Verifica se 'chave=' esta na posicao atual */
        if (strncmp(p, chave, klen) == 0 && p[klen] == '=') {
            p += klen + 1;
            int i = 0;
            while (*p && *p != '&' && i < max - 1) {
                saida[i++] = *p++;
            }
            saida[i] = '\0';
            url_decode(saida);
            return 1;
        }
        /* Avanca ate proximo par chave=valor */
        while (*p && *p != '&') p++;
        if (*p == '&') p++;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Escape JSON                                                         */
/* ------------------------------------------------------------------ */

/* Escreve 's' escapado para JSON em 'buf', retorna bytes escritos */
static int json_escreve_str(char *buf, int buf_pos, int buf_max,
                             const char *s) {
    int n = 0;
#define PUT(c) do { if (buf_pos + n < buf_max - 1) buf[buf_pos + n++] = (c); } while(0)
    PUT('"');
    for (int i = 0; s[i]; i++) {
        unsigned char c = (unsigned char)s[i];
        if      (c == '"')  { PUT('\\'); PUT('"'); }
        else if (c == '\\') { PUT('\\'); PUT('\\'); }
        else if (c == '\n') { PUT('\\'); PUT('n'); }
        else if (c == '\r') { PUT('\\'); PUT('r'); }
        else if (c == '\t') { PUT('\\'); PUT('t'); }
        else                { PUT(c); }
    }
    PUT('"');
#undef PUT
    return n;
}

/* ------------------------------------------------------------------ */
/*  Envio de arquivos estaticos                                         */
/* ------------------------------------------------------------------ */

static const char *content_type(const char *path) {
    int n = (int)strlen(path);
    if (n >= 5 && strcmp(path + n - 5, ".html") == 0) return "text/html; charset=utf-8";
    if (n >= 4 && strcmp(path + n - 4, ".css")  == 0) return "text/css";
    if (n >= 3 && strcmp(path + n - 3, ".js")   == 0) return "application/javascript";
    return "application/octet-stream";
}

static void enviar_arquivo(SOCKET cliente, const char *caminho) {
    FILE *f = fopen(caminho, "rb");
    if (!f) {
        const char *r404 = "HTTP/1.0 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
        send(cliente, r404, (int)strlen(r404), 0);
        return;
    }

    /* Descobre tamanho */
    fseek(f, 0, SEEK_END);
    long tamanho = ftell(f);
    fseek(f, 0, SEEK_SET);

    /* Cabecalho */
    char header[256];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n"
        "Access-Control-Allow-Origin: *\r\n\r\n",
        content_type(caminho), tamanho);
    send(cliente, header, hlen, 0);

    /* Corpo em chunks */
    char chunk[4096];
    size_t lido;
    while ((lido = fread(chunk, 1, sizeof(chunk), f)) > 0) {
        send(cliente, chunk, (int)lido, 0);
    }
    fclose(f);
}

/* ------------------------------------------------------------------ */
/*  Handlers de API                                                     */
/* ------------------------------------------------------------------ */

static void enviar_json(SOCKET cliente, const char *json, int len) {
    char header[256];
    int hlen = snprintf(header, sizeof(header),
        "HTTP/1.0 200 OK\r\nContent-Type: application/json; charset=utf-8\r\n"
        "Content-Length: %d\r\nAccess-Control-Allow-Origin: *\r\n\r\n", len);
    send(cliente, header, hlen, 0);
    send(cliente, json, len, 0);
}

/* GET /api/estados */
static void handle_estados(SOCKET cliente, TabelaHash *tabela) {
    BucketNode *buckets[64];
    int nb = listar_buckets(tabela, buckets, 64);

    /* Ordena siglas para resposta consistente (selection sort simples) */
    for (int i = 0; i < nb - 1; i++) {
        int min = i;
        for (int j = i + 1; j < nb; j++) {
            if (strcmp(buckets[j]->estado, buckets[min]->estado) < 0) min = j;
        }
        if (min != i) {
            BucketNode *tmp = buckets[i];
            buckets[i] = buckets[min];
            buckets[min] = tmp;
        }
    }

    char *buf = (char *)malloc(BUF_RESP);
    if (!buf) return;

    int pos = 0;
    buf[pos++] = '[';
    for (int i = 0; i < nb; i++) {
        if (i > 0) buf[pos++] = ',';
        pos += json_escreve_str(buf, pos, BUF_RESP, buckets[i]->estado);
    }
    buf[pos++] = ']';
    buf[pos]   = '\0';

    enviar_json(cliente, buf, pos);
    free(buf);
}

/* GET /api/municipios?estado=XX */
static void handle_municipios(SOCKET cliente, TabelaHash *tabela,
                               const char *qs) {
    char estado[8] = {0};
    get_param(qs, "estado", estado, sizeof(estado));
    para_maiusculas(estado);

    BucketNode *bucket = buscar_bucket(tabela, estado);
    char *buf = (char *)malloc(BUF_RESP);
    if (!buf) return;

    int pos = 0;
    buf[pos++] = '[';

    if (bucket) {
        /* Array ja esta ordenado; extrai municipios unicos */
        char ultimo[MAX_CAMPO] = {0};
        int primeiro = 1;
        for (int i = 0; i < bucket->count; i++) {
            const char *m = bucket->registros[i]->municipio;
            if (strcmp(m, ultimo) != 0) {
                if (!primeiro) buf[pos++] = ',';
                pos += json_escreve_str(buf, pos, BUF_RESP, m);
                copia_safe(ultimo, m, (int)sizeof(ultimo));
                primeiro = 0;
            }
        }
    }

    buf[pos++] = ']';
    buf[pos]   = '\0';
    enviar_json(cliente, buf, pos);
    free(buf);
}

/* Retorna 1 se a string parece um nome de bandeira valido:
   apenas letras (A-Z), espacos e hifens; comprimento entre 2 e 30. */
static int bandeira_valida(const char *s) {
    int len = 0;
    while (s[len]) {
        char c = s[len];
        if (!((c >= 'A' && c <= 'Z') || c == ' ' || c == '-' || c == '\'')) return 0;
        len++;
    }
    return (len >= 2 && len <= 30);
}

/* GET /api/bandeiras */
static void handle_bandeiras(SOCKET cliente, TabelaHash *tabela) {
    char bandeiras[64][32];
    int nb = 0;

    BucketNode *buckets[64];
    int n = listar_buckets(tabela, buckets, 64);

    for (int b = 0; b < n && nb < 64; b++) {
        for (int i = 0; i < buckets[b]->count && nb < 64; i++) {
            const char *bnd = buckets[b]->registros[i]->bandeira;
            if (!bandeira_valida(bnd)) continue;
            /* Verifica se ja existe */
            int existe = 0;
            for (int k = 0; k < nb; k++) {
                if (strcmp(bandeiras[k], bnd) == 0) { existe = 1; break; }
            }
            if (!existe) { copia_safe(bandeiras[nb], bnd, 32); nb++; }
        }
    }

    /* Ordena (selection sort) */
    for (int i = 0; i < nb - 1; i++) {
        int min = i;
        for (int j = i + 1; j < nb; j++) {
            if (strcmp(bandeiras[j], bandeiras[min]) < 0) min = j;
        }
        if (min != i) {
            char tmp[64];
            copia_safe(tmp,             bandeiras[i],   64);
            copia_safe(bandeiras[i],    bandeiras[min], 64);
            copia_safe(bandeiras[min],  tmp,            64);
        }
    }

    char *buf = (char *)malloc(4096);
    if (!buf) return;
    int pos = 0;
    buf[pos++] = '[';
    for (int i = 0; i < nb; i++) {
        if (i > 0) buf[pos++] = ',';
        pos += json_escreve_str(buf, pos, 4096, bandeiras[i]);
    }
    buf[pos++] = ']';
    buf[pos]   = '\0';
    enviar_json(cliente, buf, pos);
    free(buf);
}

/* GET /api/busca?estado=&municipio=&produto=&bandeira=&data_inicio=&data_fim= */
static void handle_busca(SOCKET cliente, TabelaHash *tabela, const char *qs) {
    char estado[8]            = {0};
    char municipio[MAX_CAMPO] = {0};
    char produto[64]          = {0};
    char bandeira[64]         = {0};
    char data_inicio[16]      = {0};
    char data_fim[16]         = {0};

    get_param(qs, "estado",       estado,       sizeof(estado));
    get_param(qs, "municipio",    municipio,    sizeof(municipio));
    get_param(qs, "produto",      produto,      sizeof(produto));
    get_param(qs, "bandeira",     bandeira,     sizeof(bandeira));
    get_param(qs, "data_inicio",  data_inicio,  sizeof(data_inicio));
    get_param(qs, "data_fim",     data_fim,     sizeof(data_fim));

    para_maiusculas(estado);
    para_maiusculas(municipio);
    para_maiusculas(produto);
    para_maiusculas(bandeira);
    /* datas permanecem em minusculas: "2026-02-01" */

    char metodo[24] = "hash";
    get_param(qs, "metodo", metodo, sizeof(metodo));
    for (int mi = 0; metodo[mi]; mi++) {
        char c = metodo[mi];
        if (c >= 'A' && c <= 'Z') metodo[mi] = (char)(c + 32);
    }

    const char *metodo_json = "hash";
    ResultadoBusca res;
    res.count     = 0;
    res.total     = 0;
    res.registros = NULL;

    if (strcmp(metodo, "sequencial") == 0) {
        metodo_json = "sequencial";
        res = buscar_sequencial(g_registros,
                                g_total_registros,
                                estado[0] ? estado : NULL,
                                municipio[0] ? municipio : NULL,
                                produto[0] ? produto : NULL,
                                bandeira[0] ? bandeira : NULL,
                                data_inicio[0] ? data_inicio : NULL,
                                data_fim[0] ? data_fim : NULL);
    } else if (strcmp(metodo, "interpolacao") == 0) {
        metodo_json = "interpolacao";
        if (g_indice_data)
            res = buscar_interpolacao(g_indice_data,
                                      estado[0] ? estado : NULL,
                                      municipio[0] ? municipio : NULL,
                                      produto[0] ? produto : NULL,
                                      bandeira[0] ? bandeira : NULL,
                                      data_inicio[0] ? data_inicio : NULL,
                                      data_fim[0] ? data_fim : NULL);
    } else {
        res = buscar(tabela,
                     estado[0] ? estado : NULL,
                     municipio[0] ? municipio : NULL,
                     produto[0] ? produto : NULL,
                     bandeira[0] ? bandeira : NULL,
                     data_inicio[0] ? data_inicio : NULL,
                     data_fim[0] ? data_fim : NULL);
    }

    char *buf = (char *)malloc(BUF_RESP);
    if (!buf) {
        if (res.registros) free(res.registros);
        return;
    }

    int pos = 0;

    /* Calcula preco medio e min/max para os resultados retornados */
    double soma = 0.0, minp = 99999.0, maxp = 0.0;
    int com_preco = 0;
    for (int i = 0; i < res.count; i++) {
        const char *v = res.registros[i]->valor_venda;
        if (v[0] != '\0') {
            /* Converte "5,69" para double (virgula decimal) */
            double val = 0.0;
            int parte_int = 1;
            double dec = 0.1;
            for (int j = 0; v[j]; j++) {
                if (v[j] >= '0' && v[j] <= '9') {
                    if (parte_int) val = val * 10 + (v[j] - '0');
                    else { val += (v[j] - '0') * dec; dec *= 0.1; }
                } else if (v[j] == ',' || v[j] == '.') {
                    parte_int = 0;
                }
            }
            if (val > 0.0) {
                soma += val;
                if (val < minp) minp = val;
                if (val > maxp) maxp = val;
                com_preco++;
            }
        }
    }
    double media = com_preco > 0 ? soma / com_preco : 0.0;

    /* Monta JSON de resposta */
#define APPEND(fmt, ...) pos += snprintf(buf + pos, BUF_RESP - pos, fmt, ##__VA_ARGS__)

    APPEND("{");
    APPEND("\"metodo\":");
    pos += json_escreve_str(buf, pos, BUF_RESP, metodo_json);
    APPEND(",");
    APPEND("\"total\":%d,", res.total);
    APPEND("\"retornados\":%d,", res.count);
    if (com_preco > 0) {
        APPEND("\"preco_medio\":\"%.2f\",", media);
        APPEND("\"preco_min\":\"%.2f\",", minp);
        APPEND("\"preco_max\":\"%.2f\",", maxp);
    } else {
        APPEND("\"preco_medio\":null,\"preco_min\":null,\"preco_max\":null,");
    }
    APPEND("\"registros\":[");

    for (int i = 0; i < res.count; i++) {
        Registro *r = res.registros[i];
        if (i > 0) APPEND(",");
        APPEND("{");
        APPEND("\"estado\":"); pos += json_escreve_str(buf, pos, BUF_RESP, r->estado); APPEND(",");
        APPEND("\"municipio\":"); pos += json_escreve_str(buf, pos, BUF_RESP, r->municipio); APPEND(",");
        APPEND("\"revenda\":"); pos += json_escreve_str(buf, pos, BUF_RESP, r->revenda); APPEND(",");
        APPEND("\"bairro\":"); pos += json_escreve_str(buf, pos, BUF_RESP, r->bairro); APPEND(",");
        APPEND("\"produto\":"); pos += json_escreve_str(buf, pos, BUF_RESP, r->produto); APPEND(",");
        APPEND("\"valor_venda\":"); pos += json_escreve_str(buf, pos, BUF_RESP, r->valor_venda); APPEND(",");
        APPEND("\"bandeira\":"); pos += json_escreve_str(buf, pos, BUF_RESP, r->bandeira); APPEND(",");
        APPEND("\"data\":"); pos += json_escreve_str(buf, pos, BUF_RESP, r->data); APPEND(",");
        APPEND("\"cep\":"); pos += json_escreve_str(buf, pos, BUF_RESP, r->cep);
        APPEND("}");
    }

    APPEND("]}");
#undef APPEND

    if (res.registros) free(res.registros);

    enviar_json(cliente, buf, pos);
    free(buf);
}

/* ------------------------------------------------------------------ */
/*  Loop principal do servidor                                          */
/* ------------------------------------------------------------------ */

void iniciar_servidor(TabelaHash *tabela,
                      Registro *registros,
                      int total_registros,
                      IndicePorData *indice_data,
                      int porta) {
    g_tabela          = tabela;
    g_registros       = registros;
    g_total_registros = total_registros;
    g_indice_data     = indice_data;

    INIT_SOCKETS();

    SOCKET srv = socket(AF_INET, SOCK_STREAM, 0);
    if ((int)srv < 0) {
        perror("[srv] socket");
        return;
    }

    /* Reutiliza porta apos restart */
    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = 0; /* INADDR_ANY */
    addr.sin_port        = htons((unsigned short)porta);

    if (bind(srv, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("[srv] bind");
        CLOSE_SOCKET(srv);
        return;
    }

    if (listen(srv, 16) < 0) {
        perror("[srv] listen");
        CLOSE_SOCKET(srv);
        return;
    }

    printf("[srv] Escutando em http://localhost:%d\n", porta);

    char *req_buf = (char *)malloc(BUF_REQ);
    if (!req_buf) { CLOSE_SOCKET(srv); return; }

    while (1) {
        struct sockaddr_in cliente_addr;
        socklen_t clen = sizeof(cliente_addr);
        SOCKET cliente = accept(srv, (struct sockaddr *)&cliente_addr, &clen);
        if ((int)cliente < 0) continue;

        /* Le a requisicao */
        memset(req_buf, 0, BUF_REQ);
        recv(cliente, req_buf, BUF_REQ - 1, 0);

        /* Extrai metodo, caminho e query string da primeira linha */
        char path[512]  = {0};
        char qs[512]    = {0};

        /* Parse "GET /path?qs HTTP/1.x" — ignora o metodo */
        {
            char *p = req_buf;
            int  idx = 0;
            /* Avanca sobre o metodo (GET, POST, ...) */
            while (*p && *p != ' ') p++;
            if (*p == ' ') p++;
            while (*p && *p != ' ' && *p != '?' && idx < 511) path[idx++] = *p++;
            if (*p == '?') {
                p++; idx = 0;
                while (*p && *p != ' ' && idx < 511) qs[idx++] = *p++;
            }
        }

        /* Roteamento */
        if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
            enviar_arquivo(cliente, FRONTEND_DIR "index.html");
        } else if (strcmp(path, "/estilo.css") == 0) {
            enviar_arquivo(cliente, FRONTEND_DIR "estilo.css");
        } else if (strcmp(path, "/app.js") == 0) {
            enviar_arquivo(cliente, FRONTEND_DIR "app.js");
        } else if (strcmp(path, "/api/estados") == 0) {
            handle_estados(cliente, tabela);
        } else if (strcmp(path, "/api/municipios") == 0) {
            handle_municipios(cliente, tabela, qs);
        } else if (strcmp(path, "/api/bandeiras") == 0) {
            handle_bandeiras(cliente, tabela);
        } else if (strcmp(path, "/api/busca") == 0) {
            handle_busca(cliente, tabela, qs);
        } else {
            const char *r404 = "HTTP/1.0 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found";
            send(cliente, r404, (int)strlen(r404), 0);
        }

        CLOSE_SOCKET(cliente);
    }

    free(req_buf);
    CLOSE_SOCKET(srv);
}
