bash

cat > /home/claude / gerar_relatorio_v2.js << 'JSEOF'
const {
  Document, Packer, Paragraph, TextRun, Table, TableRow, TableCell,
  HeadingLevel, AlignmentType, BorderStyle, WidthType, ShadingType,
  LevelFormat, PageNumber, Footer, PageBreak
} = require('docx');
const fs = require('fs');

const BLUE = "1E3A5F";
const RED = "C0392B";
const ORANGE = "D35400";
const GREEN = "1A7A4A";
const LIGHT_YELLOW = "FFF8E1";
const LIGHT_RED = "FEECEC";
const LIGHT_GREEN = "E8F5E9";

const border = { style: BorderStyle.SINGLE, size: 1, color: "CCCCCC" };
const borders = { top: border, bottom: border, left: border, right: border };

function h1(text) {
  return new Paragraph({
    heading: HeadingLevel.HEADING_1, spacing: { before: 360, after: 120 },
    children: [new TextRun({ text, bold: true, size: 36, color: BLUE, font: "Arial" })]
  });
}
function h2(text) {
  return new Paragraph({
    heading: HeadingLevel.HEADING_2, spacing: { before: 280, after: 80 },
    children: [new TextRun({ text, bold: true, size: 28, color: BLUE, font: "Arial" })]
  });
}
function h3(text) {
  return new Paragraph({
    spacing: { before: 200, after: 60 },
    children: [new TextRun({ text, bold: true, size: 24, color: "2C3E50", font: "Arial" })]
  });
}
function p(text, opts = {}) {
  return new Paragraph({
    spacing: { before: 60, after: 60 },
    children: [new TextRun({ text, size: 22, font: "Arial", ...opts })]
  });
}
function bullet(text, color) {
  return new Paragraph({
    numbering: { reference: "bullets", level: 0 }, spacing: { before: 40, after: 40 },
    children: [new TextRun({ text, size: 22, font: "Arial", color: color || "000000" })]
  });
}
function sub(text) {
  return new Paragraph({
    numbering: { reference: "subbullets", level: 0 }, spacing: { before: 30, after: 30 },
    children: [new TextRun({ text, size: 20, font: "Arial", color: "444444" })]
  });
}
function divider() {
  return new Paragraph({
    spacing: { before: 160, after: 160 },
    border: { bottom: { style: BorderStyle.SINGLE, size: 4, color: "CCCCCC", space: 1 } }, children: []
  });
}
function tableHeader(cols) {
  return new TableRow({
    tableHeader: true, children: cols.map(({ text, width }) =>
      new TableCell({
        borders, width: { size: width, type: WidthType.DXA },
        margins: { top: 80, bottom: 80, left: 120, right: 120 },
        shading: { fill: "1E3A5F", type: ShadingType.CLEAR },
        children: [new Paragraph({ children: [new TextRun({ text, bold: true, size: 20, color: "FFFFFF", font: "Arial" })] })]
      })
    )
  });
}
function row3(status, desc, detail, bg) {
  const c = status.startsWith("❌") ? RED : status.startsWith("⚠") ? ORANGE : GREEN;
  return new TableRow({
    children: [
      new TableCell({
        borders, width: { size: 2000, type: WidthType.DXA }, margins: { top: 70, bottom: 70, left: 100, right: 100 },
        shading: { fill: bg || "FFFFFF", type: ShadingType.CLEAR },
        children: [new Paragraph({ children: [new TextRun({ text: status, bold: true, size: 19, font: "Arial", color: c })] })]
      }),
      new TableCell({
        borders, width: { size: 3000, type: WidthType.DXA }, margins: { top: 70, bottom: 70, left: 100, right: 100 },
        shading: { fill: bg || "FFFFFF", type: ShadingType.CLEAR },
        children: [new Paragraph({ children: [new TextRun({ text: desc, bold: true, size: 19, font: "Arial" })] })]
      }),
      new TableCell({
        borders, width: { size: 4360, type: WidthType.DXA }, margins: { top: 70, bottom: 70, left: 100, right: 100 },
        shading: { fill: bg || "FFFFFF", type: ShadingType.CLEAR },
        children: [new Paragraph({ children: [new TextRun({ text: detail, size: 19, font: "Arial" })] })]
      }),
    ]
  });
}

const doc = new Document({
  numbering: {
    config: [
      {
        reference: "bullets", levels: [{
          level: 0, format: LevelFormat.BULLET, text: "•", alignment: AlignmentType.LEFT,
          style: { paragraph: { indent: { left: 720, hanging: 360 } } }
        }]
      },
      {
        reference: "subbullets", levels: [{
          level: 0, format: LevelFormat.BULLET, text: "–", alignment: AlignmentType.LEFT,
          style: { paragraph: { indent: { left: 1080, hanging: 360 } } }
        }]
      },
    ]
  },
  styles: {
    default: { document: { run: { font: "Arial", size: 22 } } },
    paragraphStyles: [
      {
        id: "Heading1", name: "Heading 1", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { size: 36, bold: true, font: "Arial", color: BLUE },
        paragraph: { spacing: { before: 360, after: 120 }, outlineLevel: 0 }
      },
      {
        id: "Heading2", name: "Heading 2", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { size: 28, bold: true, font: "Arial", color: BLUE },
        paragraph: { spacing: { before: 280, after: 80 }, outlineLevel: 1 }
      },
    ]
  },
  sections: [{
    properties: { page: { size: { width: 12240, height: 15840 }, margin: { top: 1440, right: 1440, bottom: 1440, left: 1440 } } },
    footers: {
      default: new Footer({
        children: [new Paragraph({
          alignment: AlignmentType.CENTER, children: [
            new TextRun({ text: "rlight Portaria Autônoma v8 — Análise de Pendências  |  Página ", size: 18, font: "Arial", color: "888888" }),
            new TextRun({ children: [PageNumber.CURRENT], size: 18, font: "Arial", color: "888888" }),
            new TextRun({ text: " de ", size: 18, font: "Arial", color: "888888" }),
            new TextRun({ children: [PageNumber.TOTAL_PAGES], size: 18, font: "Arial", color: "888888" }),
          ]
        })]
      })
    },
    children: [

      // CAPA
      new Paragraph({
        spacing: { before: 1440, after: 360 }, alignment: AlignmentType.CENTER,
        children: [new TextRun({ text: "rlight", bold: true, size: 72, font: "Arial", color: BLUE })]
      }),
      new Paragraph({
        spacing: { before: 0, after: 120 }, alignment: AlignmentType.CENTER,
        children: [new TextRun({ text: "Portaria Autônoma Inteligente", size: 32, font: "Arial", color: "555555" })]
      }),
      new Paragraph({
        spacing: { before: 120, after: 720 }, alignment: AlignmentType.CENTER,
        children: [new TextRun({ text: "Análise de Pendências para Conclusão do Projeto — v8", bold: true, size: 26, font: "Arial", color: "222222" })]
      }),
      new Paragraph({
        spacing: { before: 0, after: 80 }, alignment: AlignmentType.CENTER,
        border: { bottom: { style: BorderStyle.SINGLE, size: 6, color: BLUE, space: 1 } }, children: []
      }),
      new Paragraph({
        spacing: { before: 120, after: 60 }, alignment: AlignmentType.CENTER,
        children: [new TextRun({ text: "Jaboatão dos Guararapes, PE — Maio de 2026", size: 22, font: "Arial", color: "666666" })]
      }),

      // SUMÁRIO EXECUTIVO
      new Paragraph({ children: [new PageBreak()] }),
      h1("1. Sumário Executivo"),
      p("Análise completa do repositório rlight v8, identificando inconsistências, funcionalidades incompletas, bugs e itens de organização para concluir o projeto com qualidade de produção."),
      p(""),
      p("Arquitetura real implementada:"),
      bullet("Firmware ESP32-S3: C++ FreeRTOS AMP, FSM de 19 estados, JWT mbedTLS, KeypadHandler PCF8574"),
      bullet("Host Orange Pi: Python 3 com FastAPI + WebSocket, MQTT HA, OpenCV, SQLite"),
      bullet("Interface Kiosk: Web SPA (HTML/CSS/JS) servida pelo OPi via WebSocket — sem pygame"),
      bullet("Backend Oracle Cloud: Node.js/NGINX com SSL para validação de comprovantes"),
      p(""),
      new Table({
        width: { size: 9360, type: WidthType.DXA }, columnWidths: [3120, 3120, 3120],
        rows: [new TableRow({
          children: [
            new TableCell({
              borders, width: { size: 3120, type: WidthType.DXA }, margins: { top: 100, bottom: 100, left: 160, right: 160 },
              shading: { fill: LIGHT_RED, type: ShadingType.CLEAR }, children: [
                new Paragraph({ children: [new TextRun({ text: "🔴 7 itens CRÍTICOS", bold: true, size: 24, font: "Arial", color: RED })] }),
                new Paragraph({ children: [new TextRun({ text: "Sistema não funciona corretamente sem eles", size: 20, font: "Arial" })] }),
              ]
            }),
            new TableCell({
              borders, width: { size: 3120, type: WidthType.DXA }, margins: { top: 100, bottom: 100, left: 160, right: 160 },
              shading: { fill: LIGHT_YELLOW, type: ShadingType.CLEAR }, children: [
                new Paragraph({ children: [new TextRun({ text: "🟡 7 itens ALTOS", bold: true, size: 24, font: "Arial", color: ORANGE })] }),
                new Paragraph({ children: [new TextRun({ text: "Bugs que comprometem integridade ou segurança", size: 20, font: "Arial" })] }),
              ]
            }),
            new TableCell({
              borders, width: { size: 3120, type: WidthType.DXA }, margins: { top: 100, bottom: 100, left: 160, right: 160 },
              shading: { fill: LIGHT_GREEN, type: ShadingType.CLEAR }, children: [
                new Paragraph({ children: [new TextRun({ text: "🟢 10 itens MÉDIOS/BAIXOS", bold: true, size: 24, font: "Arial", color: GREEN })] }),
                new Paragraph({ children: [new TextRun({ text: "Qualidade, completude e organização", size: 20, font: "Arial" })] }),
              ]
            }),
          ]
        })]
      }),

      // SEÇÃO 2: FIRMWARE
      new Paragraph({ children: [new PageBreak()] }),
      h1("2. Firmware ESP32-S3 — Bugs e Inconsistências"),

      h2("2.1 CRÍTICO — keypad_granted nunca é populado no taskSensorHub"),
      p("Esta é a falha mais grave do firmware: o fluxo de acesso via teclado está completamente conectado na FSM (tick() verifica world.keypad_granted, _handleWaitingPass valida o código), mas o taskSensorHub nunca preenche esse campo. O resultado é que nenhum código digitado no teclado jamais aciona a FSM.", { color: RED }),
      p(""),
      p("Situação atual em taskSensorHub (main.cpp):"),
      new Table({
        width: { size: 9360, type: WidthType.DXA }, columnWidths: [4680, 4680],
        rows: [new TableRow({
          children: [
            new TableCell({
              borders, width: { size: 4680, type: WidthType.DXA }, margins: { top: 80, bottom: 80, left: 120, right: 120 },
              shading: { fill: LIGHT_RED, type: ShadingType.CLEAR }, children: [
                new Paragraph({ children: [new TextRun({ text: "❌ Atual — campo nunca preenchido", bold: true, size: 20, font: "Arial", color: RED })] }),
                new Paragraph({ children: [new TextRun({ text: "KeypadHandler::instance().update();\ns.keypad_granted = false; // Reset sempre!\n// keypad_access nunca é populado", size: 20, font: "Courier New", color: "8B0000" })] }),
              ]
            }),
            new TableCell({
              borders, width: { size: 4680, type: WidthType.DXA }, margins: { top: 80, bottom: 80, left: 120, right: 120 },
              shading: { fill: LIGHT_GREEN, type: ShadingType.CLEAR }, children: [
                new Paragraph({ children: [new TextRun({ text: "✅ Correção necessária", bold: true, size: 20, font: "Arial", color: GREEN })] }),
                new Paragraph({ children: [new TextRun({ text: "KeypadHandler::instance().update();\nString code = KeypadHandler::instance().getCode();\nif (code.length() > 0) {\n  AccessResult r = AccessController::instance()\n    .validate(code.c_str());\n  s.keypad_granted = (r.type != AccessType::NONE\n    && r.type != AccessType::DENIED);\n  s.keypad_access.type = r.type;\n  strlcpy(s.keypad_access.label, r.label,\n    sizeof(s.keypad_access.label));\n} else {\n  s.keypad_granted = false;\n}", size: 19, font: "Courier New", color: "004000" })] }),
              ]
            }),
          ]
        })]
      }),

      h2("2.2 CRÍTICO — TWDT não registrado em nenhuma das tasks"),
      p("O setup() chama esp_task_wdt_deinit() para limpar qualquer configuração anterior, mas nunca chama esp_task_wdt_init() para reconfigurar. As tasks taskLogicBrain e taskSensorHub nunca chamam esp_task_wdt_add(NULL) nem esp_task_wdt_reset(). O TWDT — uma das proteções mais importantes contra travamentos de I2C e HX711 — está completamente desativado.", { color: RED }),
      bullet("No setup(): após esp_task_wdt_deinit(), adicionar esp_task_wdt_init(TWDT_TIMEOUT_S, true)"),
      bullet("Início de taskLogicBrain: adicionar esp_task_wdt_add(NULL)"),
      bullet("Início de taskSensorHub: adicionar esp_task_wdt_add(NULL)"),
      bullet("Primeira linha do while(true) de cada task: esp_task_wdt_reset()"),

      h2("2.3 CRÍTICO — delay() proibido em _handleAborted"),
      p("Violação direta da Regra Absoluta #2: zero delay() em handlers FSM. O vTaskDelay(3000ms) bloqueia o Core 0 inteiro durante 3 segundos, paralisando o processamento de comandos USB, telemetria e a FSM.", { color: RED }),
      new Table({
        width: { size: 9360, type: WidthType.DXA }, columnWidths: [4680, 4680],
        rows: [new TableRow({
          children: [
            new TableCell({
              borders, width: { size: 4680, type: WidthType.DXA }, margins: { top: 80, bottom: 80, left: 120, right: 120 },
              shading: { fill: LIGHT_RED, type: ShadingType.CLEAR }, children: [
                new Paragraph({ children: [new TextRun({ text: "❌ Atual", bold: true, size: 20, font: "Arial", color: RED })] }),
                new Paragraph({ children: [new TextRun({ text: "void _handleAborted(const PhysicalState& w) {\n  sendEvent(\"DELIVERY_ABORTED\", _ctx);\n  vTaskDelay(pdMS_TO_TICKS(3000)); // PROIBIDO\n  transition(State::IDLE);\n}", size: 19, font: "Courier New", color: "8B0000" })] }),
              ]
            }),
            new TableCell({
              borders, width: { size: 4680, type: WidthType.DXA }, margins: { top: 80, bottom: 80, left: 120, right: 120 },
              shading: { fill: LIGHT_GREEN, type: ShadingType.CLEAR }, children: [
                new Paragraph({ children: [new TextRun({ text: "✅ Correção: timer não-bloqueante", bold: true, size: 20, font: "Arial", color: GREEN })] }),
                new Paragraph({ children: [new TextRun({ text: "// _onEnter(ABORTED): envia o evento\nvoid _handleAborted(const PhysicalState& w) {\n  (void)w;\n  // Aguarda 3s via state_enter — já populado\n  // pelo transition() ao entrar no estado\n  if (millis() - _ctx.state_enter >= 3000)\n    transition(State::IDLE);\n}", size: 19, font: "Courier New", color: "004000" })] }),
              ]
            }),
          ]
        })]
      }),

      h2("2.4 CRÍTICO — INA219 não lido no taskSensorHub"),
      p("O PhysicalState possui ina_p1_ma e ina_p2_ma, e a FSM os usa em _handleAuthorized (validação de corrente do strike) e _handleResidentP2 (interbloqueio). Mas o taskSensorHub nunca instancia nem lê o INA219. Os campos ficam eternamente em 0.0f, fazendo com que a validação de corrente nunca dispare e o evento STRIKE_P1_FAIL nunca seja gerado.", { color: RED }),
      bullet("Criar driver INA219 (ou integrar biblioteca Adafruit_INA219)"),
      bullet("Instanciar no setup() e chamar .begin() com os endereços I2C corretos (0x40, 0x41)"),
      bullet("Ler no taskSensorHub: s.ina_p1_ma = ina219_p1.getCurrent_mA()"),
      bullet("Adicionar ao HealthMonitor com SensorID::INA219_P1 e INA219_P2"),

      h2("2.5 CRÍTICO — String do Arduino em KeypadHandler e UsbBridge"),
      p("KeypadHandler.h/.cpp usa String do Arduino extensivamente (_current_code, _final_code, getCode() retorna String). UsbBridge.cpp usa String code = doc[\"code\"].as<String>() nos handlers CMD_ADD/REMOVE_WIEGAND_CODE. Viola a Regra Absoluta #1 e causa fragmentação de heap em operação contínua.", { color: RED }),
      p("Correções necessárias em KeypadHandler:"),
      bullet("_current_code[17] e _final_code[17] como char[] com strlcpy e strncat"),
      bullet("getCode() retorna const char* e limpa o buffer com memset"),
      bullet("getPendingLength() usa strlen(_current_code)"),
      p("Correções em UsbBridge:"),
      bullet("char code_str[24]; snprintf(code_str, sizeof(code_str), \"%s\", doc[\"code\"] | \"\")"),

      h2("2.6 CRÍTICO — Leitura dupla e contraditória do Reed Switch em taskSensorHub"),
      p("O código lê o mesmo pino duas vezes com lógicas opostas, sendo que a segunda leitura sobrescreve a primeira. Isso parece ser a lógica correta (pull-up, reed NO), mas a presença das duas linhas consecutivas indica código inacabado e confuso que pode introduzir bugs em refatorações futuras.", { color: ORANGE }),
      new Table({
        width: { size: 9360, type: WidthType.DXA }, columnWidths: [4680, 4680],
        rows: [new TableRow({
          children: [
            new TableCell({
              borders, width: { size: 4680, type: WidthType.DXA }, margins: { top: 80, bottom: 80, left: 120, right: 120 },
              shading: { fill: LIGHT_RED, type: ShadingType.CLEAR }, children: [
                new Paragraph({ children: [new TextRun({ text: "❌ Atual — lógica dupla confusa", bold: true, size: 20, font: "Arial", color: RED })] }),
                new Paragraph({ children: [new TextRun({ text: "// Linha 1 (sobrescrita imediatamente!)\ns.p1_open = (digitalRead(PIN_SW_P1) == LOW);\n// Linha 2 (prevalece)\ns.p1_open = (digitalRead(PIN_SW_P1) == HIGH);\ns.p2_open = (digitalRead(PIN_SW_P2) == HIGH);\ns.gate_open = (digitalRead(PIN_SW_GATE) == HIGH);", size: 19, font: "Courier New", color: "8B0000" })] }),
              ]
            }),
            new TableCell({
              borders, width: { size: 4680, type: WidthType.DXA }, margins: { top: 80, bottom: 80, left: 120, right: 120 },
              shading: { fill: LIGHT_GREEN, type: ShadingType.CLEAR }, children: [
                new Paragraph({ children: [new TextRun({ text: "✅ Unificar com comentário explícito", bold: true, size: 20, font: "Arial", color: GREEN })] }),
                new Paragraph({ children: [new TextRun({ text: "// Reed Switch NO + pull-up interno:\n// Imã presente (porta FECHADA) = LOW\n// Imã ausente (porta ABERTA)   = HIGH\ns.p1_open = (digitalRead(PIN_SW_P1) == HIGH);\ns.p2_open = (digitalRead(PIN_SW_P2) == HIGH);\ns.gate_open = (digitalRead(PIN_SW_GATE) == HIGH);", size: 19, font: "Courier New", color: "004000" })] }),
              ]
            }),
          ]
        })]
      }),

      h2("2.7 CRÍTICO — _handleResidentP2: condição INA219 impede abertura de P2"),
      p("O código verifica if (!w.p1_open && w.ina_p1_ma < cfg.ina_strike_min_ma) antes de abrir P2. Como ina_p1_ma é sempre 0.0f (INA219 não lido — item 2.4), e ina_strike_min_ma é 400mA, a condição 0 < 400 é sempre verdadeira — o que significa que P2 NUNCA ABRE pela FSM. Mesmo após corrigir o item 2.4, a lógica está semanticamente errada: a corrente do INA P1 indica se o strike P1 está energizado, não se P1 está fisicamente fechada.", { color: RED }),
      new Table({
        width: { size: 9360, type: WidthType.DXA }, columnWidths: [4680, 4680],
        rows: [new TableRow({
          children: [
            new TableCell({
              borders, width: { size: 4680, type: WidthType.DXA }, margins: { top: 80, bottom: 80, left: 120, right: 120 },
              shading: { fill: LIGHT_RED, type: ShadingType.CLEAR }, children: [
                new Paragraph({ children: [new TextRun({ text: "❌ Atual — P2 nunca abre", bold: true, size: 20, font: "Arial", color: RED })] }),
                new Paragraph({ children: [new TextRun({ text: "if (cfg.enable_strike_p2) {\n  if (!w.p1_open && w.ina_p1_ma < cfg.ina_strike_min_ma) {\n    // ina_p1_ma = 0 sempre → condição verdadeira\n    // MAS a intenção era verificar que P1 fechou!\n    Strike::P2().open(cfg.p2_open_ms);\n  }\n}", size: 19, font: "Courier New", color: "8B0000" })] }),
              ]
            }),
            new TableCell({
              borders, width: { size: 4680, type: WidthType.DXA }, margins: { top: 80, bottom: 80, left: 120, right: 120 },
              shading: { fill: LIGHT_GREEN, type: ShadingType.CLEAR }, children: [
                new Paragraph({ children: [new TextRun({ text: "✅ Correção: apenas interbloqueio físico", bold: true, size: 20, font: "Arial", color: GREEN })] }),
                new Paragraph({ children: [new TextRun({ text: "if (cfg.enable_strike_p2) {\n  // Interbloqueio: P1 física fechada é suficiente\n  if (!w.p1_open) {\n    Strike::P2().open(cfg.p2_open_ms);\n    Buzzer::beep(1, 200);\n    Led::btn().solid(255);\n  }\n}", size: 19, font: "Courier New", color: "004000" })] }),
              ]
            }),
          ]
        })]
      }),

      h2("2.8 ALTO — millis_at_sync corrompido em transition()"),
      p("Em StateMachine::transition(), o código usa _ctx.millis_at_sync = millis() - _ctx.state_enter para registrar a duração do estado anterior — reutilizando um campo semanticamente destinado ao sync de tempo RTC. Isso corrompe a extrapolação de timestamp (ts = unix_time + (millis() - millis_at_sync) / 1000) se uma transição de estado ocorrer entre sincronizações DS3231, podendo gerar JWTs com timestamps errados.", { color: RED }),
      bullet("Adicionar uint32_t prev_duration_ms ao FsmContext"),
      bullet("Em transition(): _ctx.prev_duration_ms = millis() - _ctx.state_enter"),
      bullet("Preservar millis_at_sync exclusivamente para CMD_SYNC_TIME"),

      h2("2.9 ALTO — loiter_start declarado duas vezes como static local em _handleIdle"),
      p("O handler _handleIdle declara static uint32_t loiter_start = 0 dentro de um if e novamente dentro de um else. Duas variáveis estáticas locais com o mesmo nome em escopos irmãos. Comportamento confuso e potencialmente undefined em alguns compiladores. Mover para FsmContext.", { color: ORANGE }),

      h2("2.10 ALTO — HealthMonitor::systemScore() não implementado"),
      p("Config.h define SCORE_GM861=25, SCORE_KEYPAD=25, SCORE_HX711=20, SCORE_MMWAVE=15, SCORE_INA219_P1=10, SCORE_INA219_P2=5 (total=100). HealthMonitor.h e .cpp não implementam systemScore(). A UI web mostra hardcoded '100/100 (Excelente)' ou '80/100 (Degradado)' baseado apenas no tempo desde o último heartbeat, sem nenhuma relação com o estado real dos sensores.", { color: ORANGE }),
      bullet("Implementar HealthMonitor::systemScore() que soma os SCORE_* dos sensores usable()"),
      bullet("Incluir SensorID::WIEGAND no enum como KEYPAD (renomear para refletir o hardware real)"),
      bullet("Enviar score real no sendTelemetry() para que a UI use o valor correto"),

      h2("2.11 ALTO — Wire.begin() sem pinos explícitos em KeypadHandler"),
      p("KeypadHandler::init() chama Wire.begin() sem especificar SDA/SCL. No Config.h, PIN_I2C_SDA=21 e PIN_I2C_SCL=38 são definidos, mas o Wire é inicializado com os pinos padrão do ESP32-S3 (normalmente GPIO 8/9). Se o hardware usa GPIO 21/38, nenhum dispositivo I2C (PCF8574, INA219) vai responder.", { color: ORANGE }),
      bullet("Corrigir: Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL) em KeypadHandler::init() ou no setup()"),
      bullet("Garantir que Wire.begin() seja chamado uma única vez antes de qualquer uso I2C"),

      // SEÇÃO 3: HOST PYTHON
      new Paragraph({ children: [new PageBreak()] }),
      h1("3. Host Python — Bugs e Inconsistências"),

      h2("3.1 CRÍTICO — requirements.txt desatualizado"),
      p("O requirements.txt lista pygame (arquitetura substituída pela Web SPA) e omite pacotes essenciais usados no código atual.", { color: RED }),
      new Table({
        width: { size: 9360, type: WidthType.DXA }, columnWidths: [2000, 3000, 4360],
        rows: [
          tableHeader([{ text: "Status", width: 2000 }, { text: "Pacote", width: 3000 }, { text: "Motivo", width: 4360 }]),
          row3("❌ REMOVER", "pygame>=2.5.2", "Substituído por Web SPA — nunca usado no código atual", LIGHT_RED),
          row3("❌ FALTANDO", "fastapi>=0.110.0", "Usado em local_api.py (import FastAPI)", LIGHT_RED),
          row3("❌ FALTANDO", "uvicorn>=0.29.0", "Servidor ASGI para FastAPI", LIGHT_RED),
          row3("❌ FALTANDO", "sdnotify>=0.3.2", "Watchdog systemd (import sdnotify em main.py)", LIGHT_RED),
          row3("❌ FALTANDO", "websockets>=12.0", "WebSocket manager (ws_manager)", LIGHT_RED),
          row3("⚠️ VERIFICAR", "qrcode[pil]>=7.4.2", "Confirmar se ainda é usado em algum módulo", LIGHT_YELLOW),
        ]
      }),

      h2("3.2 CRÍTICO — Caminhos de banco de dados inconsistentes"),
      p("Três arquivos usam caminhos diferentes para o SQLite, nenhum seguindo a arquitetura documentada (entregas em /var/lib/rlight/ no MicroSD, telemetria em /dev/shm).", { color: RED }),
      new Table({
        width: { size: 9360, type: WidthType.DXA }, columnWidths: [2500, 3000, 3860],
        rows: [
          tableHeader([{ text: "Arquivo", width: 2500 }, { text: "Caminho atual", width: 3000 }, { text: "Correto", width: 3860 }]),
          row3("❌ ERRADO", "host/core/db.py", "host/deliveries.db (relativo ao módulo)", LIGHT_RED),
          row3("❌ ERRADO", "host/local_api.py", "host/deliveries.db (hardcoded diferente)", LIGHT_RED),
          row3("❌ ERRADO", "host/weekly_report.py", "host/deliveries.db (relativo ao módulo)", LIGHT_RED),
          row3("✅ CORRETO", "Arquitetura documentada", "/var/lib/rlight/deliveries.db (MicroSD) + /dev/shm/telem.db (RAM)", LIGHT_GREEN),
        ]
      }),
      p("Além disso, a tabela delivery_features para ML (S15) está documentada no CHANGELOG mas não existe no schema de db.py."),

      h2("3.3 ALTO — Foto não vinculada ao token JWT"),
      p("O CHANGELOG e AGENT_PROMPT documentam claramente (S8) que a foto deve ser nomeada {token}.jpg usando o UUID gerado pelo ESP32. O código atual gera token_uuid = str(uuid.uuid4())[:8] independentemente do JWT e salva sempre no mesmo caminho fixo /ui/assets/last_delivery.jpg, sobrescrevendo a foto anterior.", { color: RED }),
      bullet("Em main.py: extrair o token do JWT recebido do ESP32 (host_fsm.get_jwt()), não gerar um novo UUID"),
      bullet("Em camera_usb.py: capture_snapshot(token: str) salvar em /tmp/rlight_photos/{token}.jpg"),
      bullet("O endpoint GET /photos/{token} em local_api.py já serve a pasta correta — apenas o caminho de gravação precisa ser corrigido"),

      h2("3.4 ALTO — serial_bridge.py não processa campos de telemetria"),
      p("O ESP32 envia TELEMETRY a cada 500ms com p1_open, p2_open, gate_open, int_button_pressed. O SerialBridge passa todos os payloads para host_fsm.update_from_payload(), mas MachineState.update_from_payload() não processa esses campos — os métodos get_gate_open() e get_p1_open() existem, mas os dados nunca são atualizados. O dashboard web fica sem informação de estado das portas.", { color: RED }),
      bullet("Em machine_state.py: adicionar tratamento de p1_open, p2_open, gate_open, int_button_pressed em update_from_payload()"),
      bullet("Publicar no MQTT: rlight/{id}/p1_status, p2_status, gate_status com binary_sensor HA"),

      h2("3.5 ALTO — HomeAssistant: DEVICE_ID v6, discovery incompleto"),
      p("O DEVICE_ID ainda é \"rlight_portaria_v6\" e o device name é 'Portaria RLight v6' numa instalação v8. Todas as entidades aparecem com identificador desatualizado no HA."),
      bullet("Corrigir DEVICE_ID = \"rlight_portaria_v8\" e device name = 'Portaria RLight v8'"),
      p("Entidades do AGENT_PROMPT (Tarefa 7) não publicadas no discovery:"),
      bullet("Binary sensors para P1, P2, gate com device_class: door"),
      bullet("Sensor 'Último Acesso' (tipo e label do último acesso via teclado)"),
      bullet("Number 'Delay P1→P2 ms' para ajustar p2_delay_ms remotamente"),
      bullet("Automações HA com alertas diferenciados por transportadora (S11)"),

      h2("3.6 ALTO — on_state_transition em main.py não mapeia estados novos"),
      p("O handler on_state_transition() só gerencia IDLE/MAINTENANCE para o DPMS da tela física e AWAKE/CONFIRMING para SCREEN_READY e foto. Os estados WAITING_PASS, LOCKOUT_KEYPAD, OUT_OF_HOURS, RESIDENT_P1, RESIDENT_P2 e REVERSE_PICKUP não têm tratamento Python — nenhum log, nenhuma ação específica, nenhuma publicação MQTT.", { color: ORANGE }),
      bullet("Adicionar lógica DPMS para WAITING_PASS e LOCKOUT_KEYPAD (manter tela ligada)"),
      bullet("Publicar MQTT separado para RESIDENT_ACCESS_COMPLETE e REVERSE_PICKUP_CONFIRMED"),
      bullet("Logar carrier e resident_label em transições relevantes"),

      // SEÇÃO 4: INTERFACE WEB
      new Paragraph({ children: [new PageBreak()] }),
      h1("4. Interface Web — Itens Pendentes"),

      h2("4.1 app.js — estados WAITING_PASS e LOCKOUT_KEYPAD sem tela"),
      p("O statesDict em app.js inclui RESIDENT_P1, RESIDENT_P2 e REVERSE_PICKUP com telas correspondentes (screen-RESIDENT_P1, etc.), mas WAITING_PASS e LOCKOUT_KEYPAD estão no enum da FSM e no stateToString() do ESP32, porém ausentes do dicionário e sem screen- correspondente no index.html. Se o ESP32 enviar esses estados, targetScreen será null e nenhuma tela será ativada.", { color: RED }),
      bullet("Adicionar ao statesDict: WAITING_PASS: 'Verificando Acesso', LOCKOUT_KEYPAD: '⚠ Teclado Bloqueado'"),
      bullet("Criar ou mapear screen-WAITING_PASS e screen-LOCKOUT_KEYPAD no index.html"),
      bullet("Opcionalmente, OUT_OF_HOURS (se for implementado) também precisa de tela"),

      h2("4.2 app.js — feedback visual do teclado não está conectado ao WebSocket"),
      p("O README e COMPOUND_MEMORY documentam que cada tecla pressionada deve mostrar * na tela em tempo real via WebSocket. O UsbBridge envia sendEvent(\"KEY_PRESSED\") a cada tecla, mas handleMessage() em app.js não tem handler para type=EVENT ou event=KEY_PRESSED — o * nunca aparece na tela.", { color: ORANGE }),
      bullet("Adicionar handler para data.type === 'event' && data.event === 'KEY_PRESSED' em handleMessage()"),
      bullet("Incrementar um contador de asteriscos para o campo de senha em screen-WAITING_PASS"),
      bullet("Limpar os asteriscos ao receber state_change saindo de WAITING_PASS"),

      h2("4.3 cloud_backend/server.js — sem validação de JWT"),
      p("O endpoint POST /api/deliveries armazena o JWT recebido sem verificar a assinatura HMAC-SHA256. GET /api/verify/:token também não implementa a verificação criptográfica documentada. Qualquer payload pode ser inserido no banco de comprovantes.", { color: ORANGE }),
      bullet("Implementar verificação HMAC-SHA256 no server.js usando o segredo compartilhado"),
      bullet("Retornar 401 para JWTs com assinatura inválida"),

      // SEÇÃO 5: ORGANIZAÇÃO
      new Paragraph({ children: [new PageBreak()] }),
      h1("5. Organização e Documentação"),

      h2("5.1 .gitignore incompleto"),
      p("O .gitignore atual só exclui .pio/, .vscode/ e .DS_Store. Não exclui arquivos que jamais devem ir ao repositório:"),
      bullet("*.db e host/*.db — bancos SQLite com dados reais"),
      bullet("host/__pycache__/ e **/__pycache__/ — bytecode Python"),
      bullet("/tmp/rlight_photos/ — fotos de entregas"),
      bullet(".env — variáveis de ambiente com senhas MQTT e tokens Oracle"),
      bullet("node_modules/ — dependências npm"),
      bullet("*.pyc"),

      h2("5.2 Sem arquivo .env.example"),
      p("config.py usa os.getenv() para MQTT_BROKER, MQTT_USER, MQTT_PASS, ORACLE_API_URL e ORACLE_BEARER. Não existe .env.example documentando as variáveis necessárias — um novo instalador não saberá o que configurar."),

      h2("5.3 ota_manager.py: device path e versão hardcoded"),
      p("DEVICE_PATH = \"/dev/ttyACM0\" ignora a regra udev /dev/rlight_esp. A versão comparada no __main__ está hardcoded como v6.0.0/v6.1.0."),
      bullet("Substituir por DEVICE_PATH = \"/dev/rlight_esp\" (ou ler de variável de ambiente)"),
      bullet("Ler versão atual de um arquivo VERSION ou variável de ambiente"),

      h2("5.4 weekly_report.py: envio de e-mail simulado"),
      p("send_email() contém apenas um print() — relatório semanal nunca é efetivamente enviado. Para produção, implementar via SMTP (smtplib) com credenciais por variável de ambiente, ou via API Telegram/WhatsApp Business."),

      h2("5.5 AccessController::listCodes() não implementada"),
      p("A função retorna um JSON de erro dizendo que precisa de nvs_entry_find da ESP-IDF de baixo nível. Para o comando CMD_LIST_WIEGAND_CODES do HA funcionar, é necessário implementar usando nvs_entry_find() ou manter um índice próprio no namespace NVS."),

      h2("5.6 README.md desincronizado"),
      p("O README menciona PCF8574 e teclado matricial como interface de acesso, mas também mistura linguagem de v7 (Wiegand/TF9S) e v8 (motor de portão, reed switches). Atualizar para refletir a arquitetura v8 final: teclado matricial PCF8574 como única interface de acesso presencial, sem Wiegand."),

      // SEÇÃO 6: CHECKLIST
      new Paragraph({ children: [new PageBreak()] }),
      h1("6. Checklist de Implementação por Prioridade"),

      h2("🔴 Prioridade CRÍTICA"),
      new Table({
        width: { size: 9360, type: WidthType.DXA }, columnWidths: [600, 5800, 2960],
        rows: [
          tableHeader([{ text: "#", width: 600 }, { text: "Item", width: 5800 }, { text: "Arquivo(s)", width: 2960 }]),
          ...[
            ["1", "Populando keypad_granted/keypad_access no taskSensorHub com código finalizado com #", "main.cpp"],
            ["2", "Registrar TWDT nas tasks: esp_task_wdt_init() + add(NULL) + reset() em cada iteração", "main.cpp"],
            ["3", "Corrigir _handleAborted: substituir vTaskDelay(3000) por timer não-bloqueante via state_enter", "StateMachine.cpp"],
            ["4", "Implementar leitura do INA219 no taskSensorHub (driver, begin, getCurrent_mA)", "main.cpp + novo driver"],
            ["5", "Substituir String por char[] em KeypadHandler e em UsbBridge (CMD_ADD/REMOVE)", "KeypadHandler.cpp/h, UsbBridge.cpp"],
            ["6", "Unificar leitura dupla do reed switch em uma linha com comentário explicativo", "main.cpp"],
            ["7", "Corrigir _handleResidentP2: abrir P2 verificando apenas !w.p1_open (remover condição INA219)", "StateMachine.cpp"],
            ["8", "Atualizar requirements.txt: remover pygame, adicionar fastapi, uvicorn, sdnotify, websockets", "host/requirements.txt"],
            ["9", "Corrigir DB path em todos os arquivos: host/ → /var/lib/rlight/ (SD) para entregas", "db.py, local_api.py, weekly_report.py"],
          ].map(([n, item, file]) => new TableRow({
            children: [
              new TableCell({
                borders, width: { size: 600, type: WidthType.DXA }, margins: { top: 60, bottom: 60, left: 80, right: 80 },
                shading: { fill: LIGHT_RED, type: ShadingType.CLEAR },
                children: [new Paragraph({ alignment: AlignmentType.CENTER, children: [new TextRun({ text: n, bold: true, size: 20, font: "Arial" })] })]
              }),
              new TableCell({
                borders, width: { size: 5800, type: WidthType.DXA }, margins: { top: 60, bottom: 60, left: 100, right: 100 },
                children: [new Paragraph({ children: [new TextRun({ text: item, size: 20, font: "Arial" })] })]
              }),
              new TableCell({
                borders, width: { size: 2960, type: WidthType.DXA }, margins: { top: 60, bottom: 60, left: 100, right: 100 },
                children: [new Paragraph({ children: [new TextRun({ text: file, size: 18, font: "Courier New", color: "555555" })] })]
              }),
            ]
          }))
        ]
      }),

      p(""),
      h2("🟡 Prioridade ALTA"),
      new Table({
        width: { size: 9360, type: WidthType.DXA }, columnWidths: [600, 5800, 2960],
        rows: [
          tableHeader([{ text: "#", width: 600 }, { text: "Item", width: 5800 }, { text: "Arquivo(s)", width: 2960 }]),
          ...[
            ["10", "Corrigir millis_at_sync em transition(): criar prev_duration_ms no FsmContext", "StateMachine.cpp/h"],
            ["11", "Corrigir loiter_start: mover de static local para campo no FsmContext", "StateMachine.cpp/h"],
            ["12", "Implementar HealthMonitor::systemScore() e enviar no sendTelemetry()", "HealthMonitor.cpp/h, UsbBridge.cpp"],
            ["13", "Corrigir Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL) em KeypadHandler::init()", "KeypadHandler.cpp"],
            ["14", "Vincular foto ao JWT: extrair token do JWT, passar para capture_snapshot(token)", "main.py, camera_usb.py"],
            ["15", "Processar campos p1_open/p2_open/gate_open em machine_state.update_from_payload()", "machine_state.py"],
            ["16", "Corrigir DEVICE_ID para v8 e atualizar on_state_transition() para novos estados", "homeassistant.py, main.py"],
          ].map(([n, item, file]) => new TableRow({
            children: [
              new TableCell({
                borders, width: { size: 600, type: WidthType.DXA }, margins: { top: 60, bottom: 60, left: 80, right: 80 },
                shading: { fill: LIGHT_YELLOW, type: ShadingType.CLEAR },
                children: [new Paragraph({ alignment: AlignmentType.CENTER, children: [new TextRun({ text: n, bold: true, size: 20, font: "Arial" })] })]
              }),
              new TableCell({
                borders, width: { size: 5800, type: WidthType.DXA }, margins: { top: 60, bottom: 60, left: 100, right: 100 },
                children: [new Paragraph({ children: [new TextRun({ text: item, size: 20, font: "Arial" })] })]
              }),
              new TableCell({
                borders, width: { size: 2960, type: WidthType.DXA }, margins: { top: 60, bottom: 60, left: 100, right: 100 },
                children: [new Paragraph({ children: [new TextRun({ text: file, size: 18, font: "Courier New", color: "555555" })] })]
              }),
            ]
          }))
        ]
      }),

      p(""),
      h2("🟢 Prioridade MÉDIA/BAIXA"),
      new Table({
        width: { size: 9360, type: WidthType.DXA }, columnWidths: [600, 5800, 2960],
        rows: [
          tableHeader([{ text: "#", width: 600 }, { text: "Item", width: 5800 }, { text: "Arquivo(s)", width: 2960 }]),
          ...[
            ["17", "Adicionar telas WAITING_PASS e LOCKOUT_KEYPAD no app.js (statesDict + screen HTML)", "app.js, index.html"],
            ["18", "Conectar KEY_PRESSED event ao display de * em tempo real na tela de senha", "app.js"],
            ["19", "Implementar verificação HMAC-SHA256 no cloud_backend/server.js", "cloud_backend/server.js"],
            ["20", "Adicionar discovery HA: binary sensors P1/P2/gate, sensor Último Acesso, Number p2_delay", "homeassistant.py"],
            ["21", "Atualizar .gitignore: *.db, __pycache__, .env, node_modules/, /tmp/rlight_photos/", ".gitignore"],
            ["22", "Criar .env.example com todas as variáveis necessárias documentadas", "Novo"],
            ["23", "Corrigir ota_manager.py: DEVICE_PATH = /dev/rlight_esp, versão via env", "ota_manager.py"],
            ["24", "Implementar AccessController::listCodes() com nvs_entry_find()", "AccessController.cpp"],
            ["25", "Implementar send_email() em weekly_report.py com SMTP real via env", "weekly_report.py"],
            ["26", "Adicionar tabela delivery_features ao schema de db.py (S15/ML)", "db.py"],
            ["27", "Atualizar README.md para refletir arquitetura v8 final (sem Wiegand/TF9S)", "README.md"],
          ].map(([n, item, file]) => new TableRow({
            children: [
              new TableCell({
                borders, width: { size: 600, type: WidthType.DXA }, margins: { top: 60, bottom: 60, left: 80, right: 80 },
                children: [new Paragraph({ alignment: AlignmentType.CENTER, children: [new TextRun({ text: n, bold: true, size: 20, font: "Arial" })] })]
              }),
              new TableCell({
                borders, width: { size: 5800, type: WidthType.DXA }, margins: { top: 60, bottom: 60, left: 100, right: 100 },
                children: [new Paragraph({ children: [new TextRun({ text: item, size: 20, font: "Arial" })] })]
              }),
              new TableCell({
                borders, width: { size: 2960, type: WidthType.DXA }, margins: { top: 60, bottom: 60, left: 100, right: 100 },
                children: [new Paragraph({ children: [new TextRun({ text: file, size: 18, font: "Courier New", color: "555555" })] })]
              }),
            ]
          }))
        ]
      }),

      // CONCLUSÃO
      new Paragraph({ children: [new PageBreak()] }),
      h1("7. Conclusão"),
      p("O rlight v8 tem uma base técnica sólida e bem pensada. A arquitetura AMP FreeRTOS, a FSM não-bloqueante, a criptografia JWT com mbedTLS, o wear-leveling NVS e o padrão char[]+snprintf são implementações de qualidade industrial. A maior parte do trabalho restante é de integração e conexão de camadas que foram desenvolvidas em paralelo mas não conectadas."),
      p(""),
      p("O problema mais crítico é o fluxo de acesso via teclado — a feature central da v8 — que está completamente implementada na FSM mas desconectada do sensor hub, fazendo com que nenhuma senha digitada produza efeito. Com os 9 itens críticos resolvidos, o sistema funcionará. Com todos os 27, estará em condições de produção."),
      p(""),
      divider(),
      new Paragraph({
        spacing: { before: 120, after: 60 }, alignment: AlignmentType.CENTER,
        children: [new TextRun({ text: "rlight.com.br — Jaboatão dos Guararapes, PE — Maio de 2026", size: 18, font: "Arial", color: "888888", italics: true })]
      }),
    ]
  }]
});

Packer.toBuffer(doc).then(buffer => {
  fs.writeFileSync('/mnt/user-data/outputs/rlight_analise_pendencias_v2.docx', buffer);
  console.log('OK');
});
JSEOF
node / home / claude / gerar_relatorio_v2.js