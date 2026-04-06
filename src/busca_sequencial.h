#ifndef BUSCA_SEQUENCIAL_H
#define BUSCA_SEQUENCIAL_H

#include "tipos.h"

/*
 * Busca sequencial: percorre o vetor base[0..total-1] na ordem fisica,
 * aplicando os mesmos filtros semanticos da busca principal.
 *
 * Complexidade: O(n) no numero total de registros.
 * Nao utiliza tabela hash nem busca binaria.
 *
 * resultado.registros deve ser liberado com free() pelo chamador.
 */
ResultadoBusca buscar_sequencial(Registro *base,
                                 int total,
                                 const char *estado,
                                 const char *municipio,
                                 const char *produto,
                                 const char *bandeira,
                                 const char *data_inicio,
                                 const char *data_fim);

#endif /* BUSCA_SEQUENCIAL_H */
