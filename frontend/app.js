const DUPLA = [
  { nome: "Victor Leandro Rocha de Assis", matricula: "222021826" },
  { nome: "Pedro Luiz Fonseca da Silva", matricula: "231036980" }
];

document.addEventListener("DOMContentLoaded", () => {
  document.getElementById("aluno1").textContent =
    `${DUPLA[0].nome} — ${DUPLA[0].matricula}`;
  document.getElementById("aluno2").textContent =
    `${DUPLA[1].nome} — ${DUPLA[1].matricula}`;

  carregarEstados();
  carregarBandeiras();

  document.getElementById("btn-buscar").addEventListener("click", executarBusca);
  document.getElementById("btn-limpar").addEventListener("click", limparFiltros);

  document.getElementById("inp-municipio").addEventListener("keydown", evento => {
    if (evento.key === "Enter") {
      fecharSugestoes();
      executarBusca();
    }
  });

  document.getElementById("inp-municipio").addEventListener("input", onMunicipioInput);
  document.getElementById("inp-municipio").addEventListener("blur", () => {
    setTimeout(fecharSugestoes, 150);
  });

  document.getElementById("sel-estado").addEventListener("change", () => {
    document.getElementById("inp-municipio").value = "";
    fecharSugestoes();
  });
});

async function carregarEstados() {
  try {
    const resposta  = await fetchComTimeout("/api/estados", 8000);
    const dados = await resposta.json();
    const selecao  = document.getElementById("sel-estado");
    dados.forEach(uf => {
      const opcao = document.createElement("option");
      opcao.value       = uf;
      opcao.textContent = uf;
      selecao.appendChild(opcao);
    });
  } catch (erro) {
    console.error("Erro ao carregar estados:", erro);
  }
}

async function carregarBandeiras() {
  try {
    const resposta  = await fetchComTimeout("/api/bandeiras", 8000);
    const dados = await resposta.json();
    const selecao  = document.getElementById("sel-bandeira");
    dados.forEach(bandeiraItem => {
      if (!bandeiraItem) return;
      const opcao = document.createElement("option");
      opcao.value       = bandeiraItem;
      opcao.textContent = capitalizar(bandeiraItem);
      selecao.appendChild(opcao);
    });
  } catch (erro) {
    console.error("Erro ao carregar bandeiras:", erro);
  }
}

let temporizadorDebounce = null;

function onMunicipioInput() {
  clearTimeout(temporizadorDebounce);
  const valor = document.getElementById("inp-municipio").value.trim();
  if (valor.length < 2) { fecharSugestoes(); return; }
  temporizadorDebounce = setTimeout(() => buscarSugestoes(valor), 220);
}

async function buscarSugestoes(prefixo) {
  const estado = document.getElementById("sel-estado").value;
  if (!estado) { fecharSugestoes(); return; }

  try {
    const url  = `/api/municipios?estado=${encodeURIComponent(estado)}`;
    const resposta  = await fetchComTimeout(url, 5000);
    const dados = await resposta.json();

    const maiusculo = prefixo.toUpperCase();
    const correspondencias = dados.filter(municipio => municipio.startsWith(maiusculo)).slice(0, 10);

    exibirSugestoes(correspondencias);
  } catch (erro) {
    fecharSugestoes();
  }
}

function exibirSugestoes(lista) {
  const listaNaoOrdenada = document.getElementById("sugestoes");
  listaNaoOrdenada.innerHTML = "";

  if (lista.length === 0) { fecharSugestoes(); return; }

  lista.forEach(municipio => {
    const itemLista = document.createElement("li");
    itemLista.textContent = municipio;
    itemLista.addEventListener("mousedown", () => {
      document.getElementById("inp-municipio").value = municipio;
      fecharSugestoes();
    });
    listaNaoOrdenada.appendChild(itemLista);
  });

  listaNaoOrdenada.hidden = false;
}

function fecharSugestoes() {
  document.getElementById("sugestoes").hidden = true;
}

async function fetchComTimeout(url, milissegundos) {
  const controlador = new AbortController();
  const temporizador = setTimeout(() => controlador.abort(), milissegundos);
  try {
    const resposta = await fetch(url, { signal: controlador.signal });
    clearTimeout(temporizador);
    return resposta;
  } catch (erro) {
    clearTimeout(temporizador);
    throw erro;
  }
}

async function executarBusca() {
  const estado       = document.getElementById("sel-estado").value.trim();
  const municipio    = document.getElementById("inp-municipio").value.trim().toUpperCase();
  const produto      = document.getElementById("sel-produto").value.trim();
  const bandeira     = document.getElementById("sel-bandeira").value.trim();
  const dataInicio   = document.getElementById("inp-data-inicio").value.trim();
  const dataFim      = document.getElementById("inp-data-fim").value.trim();

  if (!estado && !municipio && !produto && !bandeira && !dataInicio && !dataFim) {
    mostrarAviso("Informe pelo menos um filtro antes de buscar.");
    return;
  }

  mostrarSpinner(true);
  esconderResultados();

  const parametros = new URLSearchParams();
  if (estado)      parametros.set("estado",      estado);
  if (municipio)   parametros.set("municipio",   municipio);
  if (produto)     parametros.set("produto",     produto);
  if (bandeira)    parametros.set("bandeira",    bandeira);
  if (dataInicio)  parametros.set("data_inicio", dataInicio);
  if (dataFim)     parametros.set("data_fim",    dataFim);

  try {
    const resposta  = await fetchComTimeout(`/api/busca?${parametros.toString()}`, 10000);
    const dados = await resposta.json();
    mostrarSpinner(false);
    renderizarResultados(dados);
  } catch (erro) {
    mostrarSpinner(false);
    if (erro.name === "AbortError") {
      mostrarAviso("Tempo limite excedido. O servidor demorou para responder.");
    } else {
      mostrarAviso("Erro de conexao. Certifique-se de que busca_combustivel.exe esta em execucao.");
    }
  }
}

function renderizarResultados(dados) {
  const elementoEstatisticas   = document.getElementById("stats");
  const elementoTabela         = document.getElementById("tabela-wrapper");
  const elementoVazio          = document.getElementById("msg-vazio");
  const elementoAviso           = document.getElementById("aviso-limite");
  const elementoCorpo           = document.getElementById("tabela-corpo");

  document.getElementById("st-total").textContent    = dados.total;
  document.getElementById("st-exibindo").textContent = dados.retornados;
  document.getElementById("st-min").textContent =
    dados.preco_min ? `R$ ${dados.preco_min}` : "—";
  document.getElementById("st-med").textContent =
    dados.preco_medio ? `R$ ${dados.preco_medio}` : "—";
  document.getElementById("st-max").textContent =
    dados.preco_max ? `R$ ${dados.preco_max}` : "—";
  elementoEstatisticas.hidden = false;

  if (!dados.registros || dados.registros.length === 0) {
    elementoVazio.hidden  = false;
    elementoTabela.hidden = true;
    return;
  }

  const precos = dados.registros
    .map(registro => parseVirgula(registro.valor_venda))
    .filter(valor => valor > 0);
  const precoMinimo = Math.min(...precos);
  const precoMaximo = Math.max(...precos);
  const precoMedio = precos.reduce((soma, valorAtual) => soma + valorAtual, 0) / precos.length;

  elementoCorpo.innerHTML = "";
  dados.registros.forEach(registro => {
    const linha  = document.createElement("tr");
    const precoVirgula  = parseVirgula(registro.valor_venda);
    const classePreco = corPreco(precoVirgula, precoMinimo, precoMaximo);

    linha.innerHTML = `
      <td><strong>${registro.estado}</strong></td>
      <td>${registro.municipio}</td>
      <td>${badgeProduto(registro.produto)}</td>
      <td><span class="preco ${classePreco}">${registro.valor_venda ? "R$ " + registro.valor_venda : "—"}</span></td>
      <td>${registro.bandeira}</td>
      <td title="${registro.revenda}">${truncar(registro.revenda, 30)}</td>
      <td>${registro.bairro || "—"}</td>
      <td>${registro.data}</td>
    `;
    elementoCorpo.appendChild(linha);
  });

  elementoAviso.hidden  = dados.total <= 500;
  elementoTabela.hidden = false;
  elementoVazio.hidden  = true;
}

function mostrarSpinner(sim) {
  document.getElementById("spinner").hidden = !sim;
}

function esconderResultados() {
  document.getElementById("tabela-wrapper").hidden = true;
  document.getElementById("msg-vazio").hidden      = true;
  document.getElementById("stats").hidden          = true;
}

function mostrarAviso(mensagem) {
  const elemento = document.getElementById("msg-vazio");
  elemento.textContent = mensagem;
  elemento.hidden = false;
}

function limparFiltros() {
  document.getElementById("sel-estado").value      = "";
  document.getElementById("inp-municipio").value   = "";
  document.getElementById("sel-produto").value     = "";
  document.getElementById("sel-bandeira").value    = "";
  document.getElementById("inp-data-inicio").value = "";
  document.getElementById("inp-data-fim").value    = "";
  fecharSugestoes();
  esconderResultados();
}

function parseVirgula(texto) {
  if (!texto) return 0;
  return parseFloat(texto.replace(",", ".")) || 0;
}

function corPreco(valor, minimo, maximo) {
  if (!valor) return "";
  const intervalo = maximo - minimo;
  if (intervalo < 0.01) return "preco-medio";
  const porcentagem = (valor - minimo) / intervalo;
  if (porcentagem < 0.33) return "preco-baixo";
  if (porcentagem < 0.66) return "preco-medio";
  return "preco-alto";
}

function badgeProduto(produto) {
  if (!produto) return "";
  const textoProduto = produto.toUpperCase();
  if (textoProduto === "GASOLINA ADITIVADA")
    return `<span class="badge badge-gasolina-aditivada">Gasolina Aditivada</span>`;
  if (textoProduto === "GASOLINA")
    return `<span class="badge badge-gasolina">Gasolina</span>`;
  if (textoProduto === "ETANOL")
    return `<span class="badge badge-etanol">Etanol</span>`;
  return `<span class="badge">${capitalizar(produto)}</span>`;
}

function truncar(texto, maximo) {
  if (!texto || texto.length <= maximo) return texto || "—";
  return texto.slice(0, maximo) + "…";
}

function capitalizar(texto) {
  if (!texto) return "";
  return texto.charAt(0).toUpperCase() + texto.slice(1).toLowerCase();
}
