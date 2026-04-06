#ifndef SERVIDOR_H
#define SERVIDOR_H

#include "tabela_hash.h"
#include "tipos.h"
#include "busca_interpolacao.h"

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
 *   GET /api/busca?metodo=hash|sequencial|interpolacao&estado=&municipio=
 *                 &produto=&bandeira=  -> JSON resultados da busca
 */
void iniciar_servidor(TabelaHash *tabela,
                      Registro *registros,
                      int total_registros,
                      IndicePorData *indice_data,
                      int porta);

#endif /* SERVIDOR_H */
