# 🔐 Fechadura Eletrônica com Teclado Matricial e Módulo RFID

Este projeto foi desenvolvido como parte do projeto final da disciplina **ELET0021 — Microcontroladores e Microprocessadores** da UNIVASF. O sistema simula uma fechadura eletrônica que destrava um servo motor em 90° mediante duas autenticações: a digitação da senha correta no teclado matricial 4x4 e a aproximação de uma tag/cartão RFID válido no módulo RC522.

---

## 🧰 Componentes Utilizados

- **Microcontrolador:** [Raspberry pi pico W]  
- **Teclado Matricial:** 4x4 com membrana flexível  
- **Módulo RFID:** RC522 (comunicação SPI)  
- **Servo Motor:** Micro Servo SG90  
- **Ambiente de desenvolvimento:** VS Code + Pico SDK + Wokwi
- **Linguagem de programação:** C

---

## ⚙️ Funcionalidades Implementadas

- **Autenticação via Senha no Teclado Matricial:**
  - Validação da senha "1234" e apertar * para confirmar a senha informada
  - Exibição de mensagens de sucesso ou erro.
  - fechamento do servo através da tecla #

- **Reconhecimento de Tag RFID:**
  - Leitura do UID da tag/cartão.
  - confirmação de proximidade da tag/cartão 
  - abertura do servo com a confirmação de proximidade da tag

- **Controle do Servo Motor:**
  - Liberação da fechadura com rotação de 90°.

- **Organização do Projeto:**
  - Estrutura de código modular.
  - Repositório versionado no GitHub com documentação detalhada.
  - Vídeo demonstrativo do funcionamento do sistema. (em andamento)

---

## 👥 Organização da Equipe

- **Líder de Projeto:** [JOÃO VICTOR GUIMARÃES] - responsável pela interface do teclado matricial, acionamento do servo motor e versionamento do código.
- **Desenvolvedor 1:** [RYAN FARIAS] — responsável pela organização do github, integração e validação do módulo RFID, simulação e construção do projeto. 
- **Desenvolvedor 2:** [JOÃO VICTOR TEIXEIRA] — responsável pelo o escopo do projeto, simulação e construção do projeto, desenvolvimento do relatório final.  

---

## 📹 Vídeo de funcionamento

--

## 📄 Licença

Este projeto é destinado exclusivamente para fins acadêmicos na disciplina **ELET0021 — Microcontroladores e Microprocessadores** da UNIVASF.

---
