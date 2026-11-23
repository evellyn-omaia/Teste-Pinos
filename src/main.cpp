
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// Rede Wi-Fi criada pela ESP32
const char* NOME_REDE = "ESP32-Painel-Novo";
const char* SENHA_REDE = "TestePinos2026";

WebServer servidor(80);

// GPIOs permitidos para teste
const uint8_t PINOS[] = {
  13, 14, 18, 19, 21, 22,
  23, 25, 26, 27, 32, 33
};

const size_t TOTAL_PINOS =
  sizeof(PINOS) / sizeof(PINOS[0]);

// Aplicativo completo: HTML, CSS e JavaScript
const char PAGINA[] PROGMEM = R"ESPHTML(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport"
        content="width=device-width,initial-scale=1">

  <title>ESP32 - Painel de GPIO</title>

  <style>
    * {
      box-sizing: border-box;
    }

    body {
      margin: 0;
      font-family: Arial, sans-serif;
      background: #0b1220;
      color: #f8fafc;
    }

    main {
      max-width: 850px;
      margin: auto;
      padding: 22px 16px 36px;
    }

    header {
      display: flex;
      justify-content: space-between;
      align-items: start;
      gap: 12px;
    }

    small {
      color: #5eead4;
      font-weight: bold;
      letter-spacing: 2px;
    }

    h1 {
      margin: 8px 0;
      font-size: 28px;
    }

    p {
      color: #94a3b8;
      line-height: 1.5;
    }

    #conexao {
      color: #facc15;
      font-size: 13px;
      margin-top: 10px;
    }

    #conexao.online {
      color: #5eead4;
    }

    .resumo {
      padding: 17px;
      background: #1e293b;
      border-radius: 12px;
      margin: 18px 0 24px;
    }

    .resumo b {
      color: #5eead4;
    }

    .grade {
      display: grid;
      grid-template-columns: repeat(2, minmax(0, 1fr));
      gap: 12px;
    }

    .cartao {
      background: #1e293b;
      border: 1px solid #334155;
      border-radius: 14px;
      padding: 16px;
    }

    .linha {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 10px;
    }

    .indicador {
      display: inline-block;
      width: 11px;
      height: 11px;
      background: #64748b;
      border-radius: 50%;
    }

    .indicador.ativo {
      background: #34d399;
      box-shadow: 0 0 12px #34d399;
    }

    .estado {
      font-size: 13px;
      color: #94a3b8;
      margin: 14px 0;
    }

    .botoes {
      display: flex;
      gap: 8px;
    }

    button {
      flex: 1;
      padding: 12px 8px;
      border: 0;
      border-radius: 9px;
      font-weight: bold;
      color: #fff;
      cursor: pointer;
    }

    button:disabled {
      opacity: .5;
      cursor: wait;
    }

    .ligar {
      background: #0d9488;
    }

    .desligar {
      background: #475569;
    }

    #desligar-todos {
      width: 100%;
      background: #be123c;
      margin-top: 20px;
    }

    #mensagem {
      text-align: center;
      min-height: 22px;
    }

    footer {
      text-align: center;
      color: #64748b;
      font-size: 12px;
      margin-top: 28px;
    }

    @media (max-width: 450px) {
      .grade {
        grid-template-columns: 1fr;
      }

      h1 {
        font-size: 24px;
      }
    }
  </style>
</head>

<body>
  <main>
    <header>
      <div>
        <small>ESP32 DEVKIT V1</small>
        <h1>Painel de GPIO</h1>
        <p>Teste os pinos diretamente pelo celular.</p>
      </div>

      <strong id="conexao">Conectando...</strong>
    </header>

    <div class="resumo">
      Servidor local: <b>192.168.4.1</b>
      | 12 GPIOs disponíveis
    </div>

    <section id="grade" class="grade"></section>

    <button id="desligar-todos">
      Desligar todos os pinos
    </button>

    <p id="mensagem">Carregando estados...</p>

    <footer>
      Aplicativo armazenado no firmware da ESP32.
      Não precisa de internet.
    </footer>
  </main>

  <script>
    const pinos = [
      13, 14, 18, 19, 21, 22,
      23, 25, 26, 27, 32, 33
    ];

    const grade = document.getElementById('grade');
    const conexao = document.getElementById('conexao');
    const mensagem = document.getElementById('mensagem');

    let ocupado = false;

    grade.innerHTML = pinos.map(p => `
      <article class="cartao">
        <div class="linha">
          <b>GPIO ${p}</b>
          <span id="led-${p}" class="indicador"></span>
        </div>

        <div class="estado" id="estado-${p}">
          Consultando...
        </div>

        <div class="botoes">
          <button class="ligar"
                  data-pino="${p}"
                  data-estado="1">
            Ligar
          </button>

          <button class="desligar"
                  data-pino="${p}"
                  data-estado="0">
            Desligar
          </button>
        </div>
      </article>
    `).join('');

    async function atualizar() {
      const resposta = await fetch('/api/estados', {
        cache: 'no-store'
      });

      if (!resposta.ok) {
        throw new Error('Falha ao consultar estados');
      }

      const dados = await resposta.json();

      pinos.forEach(p => {
        const ligado = Number(dados[p]) === 1;

        document.getElementById(`led-${p}`)
          .classList.toggle('ativo', ligado);

        document.getElementById(`estado-${p}`)
          .textContent = ligado
            ? 'Estado: LIGADO'
            : 'Estado: DESLIGADO';
      });

      conexao.textContent = '● Conectado';
      conexao.classList.add('online');
    }

    async function enviar(url) {
      if (ocupado) return;

      ocupado = true;

      document.querySelectorAll('button')
        .forEach(b => b.disabled = true);

      try {
        const resposta = await fetch(url, {
          method: 'POST',
          cache: 'no-store'
        });

        if (!resposta.ok) {
          throw new Error(await resposta.text());
        }

        await atualizar();

        mensagem.textContent =
          'Comando enviado com sucesso!';
      } catch (erro) {
        conexao.textContent = '● Sem comunicação';
        conexao.classList.remove('online');

        mensagem.textContent =
          'Erro: ' + erro.message;
      } finally {
        ocupado = false;

        document.querySelectorAll('button')
          .forEach(b => b.disabled = false);
      }
    }

    document.querySelectorAll('[data-pino]')
      .forEach(botao => {
        botao.addEventListener('click', () => {
          const p = botao.dataset.pino;
          const e = botao.dataset.estado;

          enviar(`/api/pino?pino=${p}&estado=${e}`);
        });
      });

    document.getElementById('desligar-todos')
      .addEventListener('click', () => {
        enviar('/api/desligar');
      });

    async function consultar() {
      if (ocupado) return;

      try {
        await atualizar();
      } catch (erro) {
        conexao.textContent = '● Sem comunicação';
        conexao.classList.remove('online');
        mensagem.textContent =
          'Sem resposta da ESP32.';
      }
    }

    consultar();
    setInterval(consultar, 3000);
  </script>
</body>
</html>
)ESPHTML";

// Confere se o GPIO pode ser controlado
bool permitido(int pino) {
  for (size_t i = 0; i < TOTAL_PINOS; i++) {
    if (pino == PINOS[i]) return true;
  }

  return false;
}

// Página principal
void mostrarPagina() {
  servidor.sendHeader("Cache-Control", "no-store");

  servidor.send_P(
    200,
    "text/html; charset=utf-8",
    PAGINA
  );
}

// Retorna os estados de todos os GPIOs
void consultarEstados() {
  String resposta = "{";

  for (size_t i = 0; i < TOTAL_PINOS; i++) {
    if (i > 0) resposta += ",";

    resposta += "\"" + String(PINOS[i]) + "\":"
      + String(digitalRead(PINOS[i]) == HIGH ? 1 : 0);
  }

  resposta += "}";

  servidor.sendHeader("Cache-Control", "no-store");
  servidor.send(200, "application/json", resposta);
}

// Liga ou desliga um GPIO
void controlarPino() {
  if (!servidor.hasArg("pino") ||
      !servidor.hasArg("estado")) {
    servidor.send(
      400, "text/plain", "Faltam parametros"
    );
    return;
  }

  String textoPino = servidor.arg("pino");
  String estado = servidor.arg("estado");
  int pino = textoPino.toInt();

  if (textoPino != String(pino) ||
      !permitido(pino) ||
      (estado != "0" && estado != "1")) {
    servidor.send(
      400, "text/plain", "Pino ou estado invalido"
    );
    return;
  }

  digitalWrite(
    pino,
    estado == "1" ? HIGH : LOW
  );

  Serial.printf(
    "GPIO %d: %s\n",
    pino,
    estado == "1" ? "LIGADO" : "DESLIGADO"
  );

  servidor.send(
    200,
    "application/json",
    "{\"ok\":true}"
  );
}

// Desliga todos os GPIOs
void desligarTodos() {
  for (size_t i = 0; i < TOTAL_PINOS; i++) {
    digitalWrite(PINOS[i], LOW);
  }

  Serial.println("Todos os GPIOs desligados");

  servidor.send(
    200,
    "application/json",
    "{\"ok\":true}"
  );
}

void setup() {
  Serial.begin(115200);
  delay(300);

  // Inicia todos os GPIOs desligados
  for (size_t i = 0; i < TOTAL_PINOS; i++) {
    digitalWrite(PINOS[i], LOW);
    pinMode(PINOS[i], OUTPUT);
  }

  // Cria a rede Wi-Fi local
  WiFi.mode(WIFI_AP);

  if (!WiFi.softAP(NOME_REDE, SENHA_REDE)) {
    Serial.println("ERRO ao criar a rede Wi-Fi!");
    return;
  }

  // Rotas do aplicativo
  servidor.on("/", HTTP_GET, mostrarPagina);

  servidor.on(
    "/api/estados",
    HTTP_GET,
    consultarEstados
  );

  servidor.on(
    "/api/pino",
    HTTP_POST,
    controlarPino
  );

  servidor.on(
    "/api/desligar",
    HTTP_POST,
    desligarTodos
  );

  servidor.on("/favicon.ico", HTTP_GET, []() {
    servidor.send(204);
  });

  servidor.onNotFound([]() {
    servidor.send(
      404,
      "text/plain",
      "Endereco inexistente"
    );
  });

  servidor.begin();

  Serial.println("==========================");
  Serial.println("ESP32 - PAINEL DE PINOS");
  Serial.print("Wi-Fi: ");
  Serial.println(NOME_REDE);
  Serial.print("IP: http://");
  Serial.println(WiFi.softAPIP());
  Serial.println("Servidor iniciado. Sem LittleFS!");
  Serial.println("==========================");
}

void loop() {
  servidor.handleClient();
  delay(2);
}
