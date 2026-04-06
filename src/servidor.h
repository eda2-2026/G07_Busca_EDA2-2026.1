#ifndef SERVIDOR_H
#define SERVIDOR_H

#include "tabela_hash.h"

/*
 * Inicia o servidor HTTP na porta especificada.
 * Bloqueia indefinidamente atendendo requisicoes.
 *
 * Rotas disponíveis:
 *   GET /                              -> index.html
 *   GET /estilo.css                    -> estilo.css
 *   GET /app.js                        -> app.js
 *   GET /api/estados                   -> JSON lista de estados
 *   GET /api/municipios?estado=XX      -> JSON lista de municipios do estado
 *   GET /api/bandeiras                 -> JSON lista de bandeiras
 *   GET /api/busca?estado=&municipio=
 *                 &produto=&bandeira=  -> JSON resultados da busca
 */
void iniciar_servidor(TabelaHash *tabela, int porta);

#endif /* SERVIDOR_H */
