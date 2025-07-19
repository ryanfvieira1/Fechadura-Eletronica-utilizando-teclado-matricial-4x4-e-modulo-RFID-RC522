// Inclusão das bibliotecas
#include <stdio.h>                     // Biblioteca padrão de entrada/saída
#include <string.h>                    // Para manipulação de strings, como strlen
#include <stdlib.h>                    // Para funções de utilidade geral
#include <pico/stdlib.h>               // Funções básicas do Raspberry Pi Pico
#include <math.h>                      // Biblioteca matemática (para fabs())
#include <hardware/pwm.h>              // Acesso ao hardware PWM
#include <hardware/gpio.h>             // Gerenciar pinos de I/O
#include <hardware/spi.h>
#include <mfrc522.h>

// Definições dos pinos
#define servo_motor 8                   // Pino que controla o servo (G28)
uint8_t a = 0;

// 🚀 Definição dos pinos SPI
#define PIN_SCK    18  // Clock
#define PIN_MOSI   19  // Dados para o leitor
#define PIN_MISO   16  // Dados do leitor
#define PIN_CS     17  // Chip Select (SDA)
#define PIN_RST    5  // Reset (NÃO USA)

// Definições dos pinos do teclado matricial
#define Row_1 0
#define Row_2 1
#define Row_3 2
#define Row_4 3

#define Col_1 20
#define Col_2 4
#define Col_3 9
#define Col_4 6 // (NÃO USA)

// Mapeamento do teclado
char keypad_map[4][4] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'} // '*' agora para confirmar, '#' para apagar, 'A B C D' são inválidos
};

// Variáveis para a lógica da senha
char senha_digitada[5];                   // Armazena a senha digitada (4 dígitos + '\0')
const char SENHA_CORRETA[] = "1234";      // A senha correta
int senha_idx = 0;                        // Índice atual da senha digitada
char tecla;                               // Armazena a tecla lida do teclado

// Variável de tempo
static volatile uint32_t last_time = 0; // Armazena o tempo do último evento (em microssegundos)

// Função de interrupção
static void gpio_irq_handler(uint gpio, uint32_t events);

// Constantes de PWM
#define PWM_CLOCK_DIVIDER 64.0         // Divisor de clock para gerar frequência de 50Hz (servo)
#define PWM_PERIOD_US 20000.0          // Período do PWM em microssegundos (20ms = 50Hz)
#define PWM_TOP_VALUE 39063            // Valor máximo do contador PWM (para ~50Hz com divisor 64)

uint slice_servo;                        // Variável para armazenar o slice PWM do servo

// Função para comparar dois arrays de UID
bool comparar_uid(uint8_t *lido, uint8_t *referencia, uint8_t tamanho) {
    for (int i = 0; i < tamanho; i++) {
        if (lido[i] != referencia[i]) return false;
    }
    return true;
}

// Calcula o duty cycle proporcional ao tempo de pulso desejado
static uint16_t calculate_pwm_duty(float pulse_width_us) {
  if (pulse_width_us < 0) pulse_width_us = 0;
  if (pulse_width_us > PWM_PERIOD_US) pulse_width_us = PWM_PERIOD_US;
  return (uint16_t)((pulse_width_us / PWM_PERIOD_US) * PWM_TOP_VALUE);
}

// Aplica o duty cycle calculado no pino especificado
static void set_pwm_pulse(uint slice, uint gpio, float pulse_width_us) {
  uint16_t duty_cycle = calculate_pwm_duty(pulse_width_us);
  pwm_set_gpio_level(gpio, duty_cycle);
}

// Inicializa um pino GPIO para uso com PWM
static void initialize_pwm_pin(uint gpio, uint *slice_num) {
  gpio_set_function(gpio, GPIO_FUNC_PWM);                  // Define função PWM no pino
  *slice_num = pwm_gpio_to_slice_num(gpio);                // Descobre qual slice controla esse pino
  pwm_set_clkdiv(*slice_num, PWM_CLOCK_DIVIDER);           // Define o divisor do clock (64)
  pwm_set_wrap(*slice_num, PWM_TOP_VALUE);                 // Define o valor máximo do contador PWM
  pwm_set_enabled(*slice_num, true);                       // Ativa o PWM nesse slice
}

// Função para inicialização do Teclado
void keypad_init() {
  gpio_init(Row_1);
  gpio_init(Row_2);
  gpio_init(Row_3);
  gpio_init(Row_4);

  gpio_set_dir(Row_1, GPIO_OUT);
  gpio_set_dir(Row_2, GPIO_OUT);
  gpio_set_dir(Row_3, GPIO_OUT);
  gpio_set_dir(Row_4, GPIO_OUT);

  gpio_put(Row_1, 1);
  gpio_put(Row_2, 1);
  gpio_put(Row_3, 1);
  gpio_put(Row_4, 1);

  gpio_init(Col_1);
  gpio_init(Col_2);
  gpio_init(Col_3);
  gpio_init(Col_4);

  gpio_set_dir(Col_1, GPIO_IN);
  gpio_set_dir(Col_2, GPIO_IN);
  gpio_set_dir(Col_3, GPIO_IN);
  gpio_set_dir(Col_4, GPIO_IN);

  gpio_set_pulls(Col_1, true, false);
  gpio_set_pulls(Col_2, true, false);
  gpio_set_pulls(Col_3, true, false);
  gpio_set_pulls(Col_4, true, false);
}

// Escaneamento do teclado
char scan_keypad() {
  for (int row = 0; row < 4; row++) {
    gpio_put(Row_1, 1);
    gpio_put(Row_2, 1);
    gpio_put(Row_3, 1);
    gpio_put(Row_4, 1);

    switch (row) {
      case 0: gpio_put(Row_1, 0); break;
      case 1: gpio_put(Row_2, 0); break;
      case 2: gpio_put(Row_3, 0); break;
      case 3: gpio_put(Row_4, 0); break;
    }

    sleep_us(50);

    for (int col = 0; col < 4; col++) {
      int col_pin;
      switch (col) {
        case 0: col_pin = Col_1; break;
        case 1: col_pin = Col_2; break;
        case 2: col_pin = Col_3; break;
        //*case 3: col_pin = Col_4; break;
      }

      if (gpio_get(col_pin) == 0) {

        sleep_ms(50); // Debounce

        if (gpio_get(col_pin) == 0) { // Espera a tecla ser solta

          while (gpio_get(col_pin) == 0) {
            sleep_ms(10);
          }

          gpio_put(Row_1, 1);
          gpio_put(Row_2, 1);
          gpio_put(Row_3, 1);
          gpio_put(Row_4, 1);

          return keypad_map[row][col];
        }
      }
    }
  }

  gpio_put(Row_1, 1);
  gpio_put(Row_2, 1);
  gpio_put(Row_3, 1);
  gpio_put(Row_4, 1);

  return 0; // Retorna 0 se nenhuma tecla foi detectada
}


// Função principal
int main() {

  //delay pequeno para corrigir bugs
  sleep_ms(300);

  // Inicializa as interfaces padrões
  stdio_init_all();

  // Inicializa o pino PWM
  initialize_pwm_pin(servo_motor, &slice_servo);

  // Inicializar SPI manualmente
  spi_init(spi0, 1 * 1000 * 1000);  // 1 MHz
  gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
  gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
  gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
  gpio_init(PIN_CS);
  gpio_set_dir(PIN_CS, GPIO_OUT);
  gpio_put(PIN_CS, 1);
  gpio_init(PIN_RST);
  gpio_set_dir(PIN_RST, GPIO_OUT);
  gpio_put(PIN_RST, 1);

  // Inicializar o MFRC522
  MFRC522Ptr_t rfid = MFRC522_Init();
  PCD_Init(rfid, spi0);

  Uid uid;                               // Estrutura que armazenará o UID do cartão
  uint8_t atqa[2];                       // Resposta curta do cartão para verificação
  uint8_t atqa_len = sizeof(atqa);       // Tamanho da resposta ATQA

  // Inicializa o teclado matricial
  keypad_init();

  // Instrução inicial para digitar senha
  printf("Sistema Iniciado. Digite 4 digitos e pressione '*' para confirmar, ou '#' para apagar.\n");

  // Reinicia o buffer da senha para garantir que está vazio no início
  memset(senha_digitada, 0, sizeof(senha_digitada));
  senha_idx = 0; // Reinicia o índice da senha

  // Função loop
  while (true) {

    tecla = scan_keypad(); // Captura a tecla pressionada

    if (tecla != 0) { // Se uma tecla foi pressionada

      if (tecla >= '0' && tecla <= '9') { // Verifica se a tecla é um dígito numérico (0-9)

        if (senha_idx < 4) { // Verifica se ainda não digitou 4 dígitos

          senha_digitada[senha_idx] = tecla; // Armazena o dígito
          senha_idx++; // Incrementa o índice
          printf("Senha atual: %.*s\n", senha_idx, senha_digitada); // Exibe a senha digitada no monitor serial

        }
        else if (senha_idx == 4){
          printf("Limite de 4 digitos atingido. Pressione '*' para confirmar.\n");
        }
        else if(senha_idx > 4){ // Quando for armazenado 4 dígitos

          printf("Entrada APAGADA. Digite 4 digitos e pressione '*' para confirmar.\n");
          memset(senha_digitada, 0, sizeof(senha_digitada)); // Limpa o buffer
          senha_idx = 0; // Reinicia o índice

        }
      }

      else if (tecla == '*') {  // Tecla '*' para CONFIRMAR a combinação digitada

        if (senha_idx == 4) { // Verifica se 4 dígitos foram digitados antes de tentar confirmar

          senha_digitada[4] = '\0'; // Adiciona o terminador nulo para a string de senha

          if (strcmp(senha_digitada, SENHA_CORRETA) == 0) { // Compara a senha digitada com a senha correta

            printf("Senha CORRETA! Acesso concedido.\n");
            set_pwm_pulse(slice_servo, servo_motor, 500);              // Servo em 0° (ABERTO)
            a++;

          }
          else {

            printf("Senha INCORRETA! Acesso negado.\n");
            set_pwm_pulse(slice_servo, servo_motor, 1470);              // Servo em 90° (FECHADO)

          }

          printf("--- Nova tentativa ---\nDigite 4 digitos e pressione '*' para confirmar.\n"); // Prepara para a próxima tentativa
          memset(senha_digitada, 0, sizeof(senha_digitada)); // Limpa o buffer
          senha_idx = 0; // Reinicia o índice

        }
        else {

          printf("Por favor, digite 4 digitos antes de confirmar. Digitos atuais: %d\n", senha_idx); //

        }
      }
      else if (tecla == '#') { // Tecla '#' para APAGAR a combinação digitada

          printf("PORTA FECHADA.\n");
          set_pwm_pulse(slice_servo, servo_motor, 1470);              // Servo em 90° (FECHADO)
          a++;

      }

      else { // Ignora outras teclas (A, B, C, D) que não são números, '*' ou '#'

        printf("Tecla invalida para senha: %c. Digite 0-9, '*' para confirmar, ou '#' para apagar.\n", tecla);

      }
    }

    if (PICC_RequestA(rfid, atqa, &atqa_len) == STATUS_OK)
    {
      printf("CARTÃO DETECTADO! PORTA ABERTA\n");
      set_pwm_pulse(slice_servo, servo_motor, 500); // Servo em 0° (ABERTO)
      sleep_ms(2000); // Espera 2 segundos para manter a porta aberta

      PICC_HaltA(rfid);
    }
  }

  return 0;
}


/* POSIÇÕES:


    // Movimento fixo para 0 graus (pulso ~500us)
    set_pwm_pulse(slice_servo, servo_motor, 500);              // Servo em 0°


    // Movimento fixo para 90 graus (pulso ~1470us)
    set_pwm_pulse(slice_servo, servo_motor, 1470);             // Servo em 90°


*/