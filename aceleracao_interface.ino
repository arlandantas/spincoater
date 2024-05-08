#include <LiquidCrystal.h>

#define INICIAL 123
#define PIN_MOTOR 8
int ESTADO_ATUAL = 3;
int ESTADO_ACELERACAO = 0;
int velocidade = 0, tempo = 0, tempo_millis = 0;
unsigned long millis_atual = 0, millis_anterior = 0, millis_inicio = 0;
int PWM_ATUAL = 0, PWM_DESEJADO = 0;

const byte kb_cols = 3; // TRÊS COLUNAS
const byte kb_rows = 4; // QUATRO LINHAS
char keys[kb_rows][kb_cols] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
//22 23 24 25 26 27 28
byte kb_colPins[kb_cols] = {24,22,26};    // PINOS CONECTADOS AOS CONTATOS DAS COLUNAS DO TECLADO 
byte kb_rowPins[kb_rows] = {23,28,27,25}; // PINOS CONECTADOS AOS CONTATOS DAS LINHAS DO TECLADO 
          //  PWM =  135, 140, 145....
int VELOCIDADES[] = {2500,3700,4800,5300,6000,6500,6900,7000,7300,7500,7700,8000,8300,8370};

int QTD_VELOCIDADES = 14, LIMITE_MIN = 2500, LIMITE_MAX = 8370;

int var = -1, n_var = -1; // 

LiquidCrystal lcd(12, 11, 2, 3, 4, 5);
void setup() {
  lcd.begin(16, 2);
   //Serial.begin(9600);

  pinMode(PIN_MOTOR, OUTPUT);  
  for(int i = 0; i < kb_cols; i++) {
    pinMode(kb_colPins[i], INPUT);
  }
  for(int i = 0; i < kb_rows; i++) {
    pinMode(kb_rowPins[i], OUTPUT);
    digitalWrite(kb_rowPins[i], LOW);
  }
  resetLcd();
  lcd.print("   SPIN COATER  ");
  lcd.setCursor(0, 1);
  lcd.print("    HARD DISK   ");
  analogWrite(PIN_MOTOR, 0);
  delay(1000);
  analogWrite(PIN_MOTOR, 115);
  delay(1000);
}
void loop() {
  millis_atual = millis();
  switch (ESTADO_ATUAL) {
    case 3:
      /*
      APENAS AGUARDA ATÉ QUE O USUÁRIO INSIRA O VALOR DA VELOCIDADE
      */
      resetLcd();
      lcd.print("Velocidade:");
      velocidade = lerTeclado();
      ++ESTADO_ATUAL;
      break;
    case 4:
      /*
      VALIDA A VELOCIDADE DESEJADA PELO USUÁRIO E MUDA PARA O ESTADO 6 SE VÁLIDO E 5 SE INVÁLIDO
      */
      if (velocidade >= LIMITE_MIN && velocidade <= LIMITE_MAX) {
        ESTADO_ATUAL = 6;
      } else {
        millis_anterior = millis_atual;
        resetLcd();
        lcd.print("   VELOCIDADE  ");
        lcd.setCursor(0, 1);
        lcd.print("    INVALIDA    ");
        ESTADO_ATUAL = 5;
      }
      break;
    case 5:
      /*
      ESTE ESTADO SÓ É CHAMADO CASO A VELOCIDADE DESEJADA SEJA INVÁLIDA,
      ELE ESPERA 2 SEGUNDOS OU ATÉ QUE O USUÁRIO PRESSIONE E SOLTE ALGUMA TECLA
      */
      if(millis_atual - millis_anterior >= 2000 || tecladoPressionado()) {
        ESTADO_ATUAL = 3;
        while(tecladoPressionado()) {}
      }
      break;
    case 6:
      /*
      LÊ O TEMPO DESEJADO PELO USUÁRIO
      */
      resetLcd();
      lcd.print("Tempo Max:");
      tempo = lerTeclado();
      tempo_millis = tempo*1000;
      
      ++ESTADO_ATUAL; // ESTADO_ATUAL = ESTADO_ATUAL + 1;
      break;
    case 7:
      /*
      CALCULA QUAL A VELOCIDADE POSSÍVEL (VELOCIDADES QUE FORAM MEDIDAS)
      MAIS PRÓXIMA DA VELOCIDADE DESEJADA PELO USUÁRIO E O SEU RESPECTIVO PWM
      */
      var = -1;
      n_var = -1;
      for (int i = 0; i < QTD_VELOCIDADES; i++) {
        n_var = VELOCIDADES[i] - velocidade;
        n_var = abs(n_var);
        if (var == -1 || n_var < var) {
          var = n_var;
          PWM_DESEJADO = (27+i)*5;
        }
      }

      /*
      PREPARA AS VARIÁVEIS PARA INICIAR O PROCESSO DE ACELERAÇÃO
      */
      var = 0;
      n_var = -1;
      ESTADO_ACELERACAO = 0;
      millis_inicio = millis_atual;

      /*
      PREPARA O MOTOR
      */
      resetLcd();
      lcd.print("    AGUARDE     ");
      lcd.setCursor(0, 1);
      lcd.print("    AGUARDE     ");
      analogWrite(PIN_MOTOR, 0);
      delay(4000);
      analogWrite(PIN_MOTOR, 115);
      delay(4000);

      /*
      PREPARA AS INFORMAÇÕES NO LCD
      */
      resetLcd();
      lcd.print("Restam");
      lcd.setCursor(7, 0);
      lcd.print(tempo);
      lcd.setCursor(0, 1);
      lcd.print("segundos...");

      ++ESTADO_ATUAL;
      break;
    case 8:
      /*
      CASO O TEMPO LIMITE SEJA ATINGIDO OU O USUÁRIO PRESSIONE UMA DAS TECLAS DE FUNÇÃO (#, *),
      O MOTOR É DESLIGADO E O PROGRAMA AGUARDA ATÉ QUE O USUÁRIO SOLTE A TECLA DE FUNÇÃO
      */
      if (millis_atual - millis_inicio >= tempo_millis || tecladoFuncaoPressionado()) {
        PWM_ATUAL = 115;
        analogWrite(PIN_MOTOR, PWM_ATUAL);
        resetLcd();
        lcd.print("   FINALIZANDO  ");
        lcd.setCursor(0, 1);
        lcd.print("    EXECUCAO    ");
        while(tecladoFuncaoPressionado()) {}
        millis_anterior = millis_atual;
        ++ESTADO_ATUAL;
      } else {
        /*
        FAZ-SE ESSE CÁLCULO PARA QUE A ATUALIZAÇÃO DA TELA SEJA FEITA
        A CADA SEGUNDO E NÃO A CADA EXECUÇÃO, DIMINUINDO O PROCESSAMENTO
        */
        int n_var = (millis_atual - millis_inicio)/1000;
        if (n_var > var) {
          var = n_var;
          lcd.setCursor(7, 0);
          lcd.print("          ");
          lcd.setCursor(7, 0);
          lcd.print(tempo - var);
        }
        /*
        CHAMA A FUNÇÃO RESPONSÁVEL POR FAZER TODO O PROCESSO DE ACELERAÇÃO
        */
        executarAceleracao();
      }
      break;
    case 9:
      /*
      ESPERA 4 SEGUNDOS PARA QUE O MOTOR PARE E DEPOIS VOLTE PARA O ESTADO INICIAL
      */
      if (millis_atual - millis_anterior >= 4000) {
        ESTADO_ATUAL = 3;
      }
      break;
    default:
      break;
  }
}

void executarAceleracao() {
  /*
  SÓ SÃO FEITAS MODIFICAÇÕES CASO O PWM APLICADO AO MOTOR AINDA
  NÃO TENHA ATINGIDO O VALOR CALCULADO NO INÍCIO DO ESTADO 7,
  CASO JÁ TENHA SIDO ATINGIDO, NADA É FEITO
  */
  if (PWM_ATUAL < PWM_DESEJADO) {
    switch (ESTADO_ACELERACAO) {
      case 0:
        /*
        NO PRIMEIRO ESTÁGIO DA ACELERAÇÃO É APENAS APLICADO UM VALOR PADRÃO (123)
        DE PWM AO MOTOR, ENCONTRADO POR MEIO DE TESTES NA MÃO
        */
        PWM_ATUAL = 123;
        analogWrite(PIN_MOTOR, PWM_ATUAL);
        millis_anterior = millis_atual;
        Serial.println(PWM_ATUAL);
        ESTADO_ACELERACAO = 1;
        break;
      case 1:
        /*
        E AGUARDA 4 SEGUNDOS COM O VALOR PADRÃO
        */
        if (millis_atual - millis_anterior >= 4000) {
          millis_anterior = 0;
          ++ESTADO_ACELERACAO;
        }
        break;
      case 2:
        /*
        NO SEGUNDO ESTÁGIO, O VALOR DO PWM É AUMENTADO DE 3 EM 3 A CADA 3 SEGUNDOS ATÉ O VALOR DE 130
        (TAMBÉM ENCONTRADO POR TESTES), POIS COM A ACELERAÇÃO MAIS RÁPIDA O MOTOR PARA
        */
        if (millis_anterior == 0 || millis_atual - millis_anterior >= 3000) {
          PWM_ATUAL += 3;
          if (PWM_ATUAL > PWM_DESEJADO) {
            PWM_ATUAL = PWM_DESEJADO;
          }
          if (PWM_ATUAL > 130) {
            ++ESTADO_ACELERACAO;
          }
          millis_anterior = millis_atual;
        }
        analogWrite(PIN_MOTOR, PWM_ATUAL);
        break;
      case 3:
        /*
        NO TERCEIRO ESTÁGIO, O VALOR DO PWM É AUMENTADO DE 5 EM 5 A CADA SEGUNDO
        ATÉ O VALOR DE CALCULADO INICIALMENTE
        */
        if (millis_anterior == 0 || millis_atual - millis_anterior >= 1000) {
          PWM_ATUAL += 5;
          if (PWM_ATUAL > PWM_DESEJADO) {
            PWM_ATUAL = PWM_DESEJADO;
          }
          millis_anterior = millis_atual;
        }
        analogWrite(PIN_MOTOR, PWM_ATUAL);
        break;
    }
  }
}

void resetLcd() {
  lcd.clear();
  lcd.setCursor(0, 0);
}
void aguardarSerial() {
  while(Serial.available() == 0) {}
}
int lerTeclado() {
  lcd.setCursor(0,1);
  boolean continue_while = true;
  String ret = "";
  do {
    for(int i = 0; i < kb_rows; i++) {
      digitalWrite(kb_rowPins[i], HIGH);
      for(int j = 0; j < kb_cols; j++) {
        if(digitalRead(kb_colPins[j]) == HIGH) {
          char ch = keys[i][j];
          if ((i < 3 && j < 3) || (i == 3 && j == 1)) {
            if (!(ret == "" && ch == '0')) {
              ret += keys[i][j];
              lcd.print(ch);
            }
          } else if (ch == '#') {
            continue_while = false;
          } else if (ch == '*') {
            ret = ret.substring(0, ret.length() - 1);
            lcd.setCursor(0,1);
            lcd.print("                ");
            lcd.setCursor(0,1);
            lcd.print(ret);
          }
          while(digitalRead(kb_colPins[j]) == HIGH){}
        }
      }
      digitalWrite(kb_rowPins[i], LOW);
    }
  } while (continue_while);
  return ret.toInt();
}
void aguardarTeclado() {
  while (!tecladoPressionado()) {}
}
boolean tecladoPressionado() {
  for(int i = 0; i < kb_rows; i++) {
    digitalWrite(kb_rowPins[i], HIGH);
    for(int j = 0; j < kb_cols; j++) {
      if(digitalRead(kb_colPins[j]) == HIGH) {
        return true;
      }
    }
    digitalWrite(kb_rowPins[i], LOW);
  }
  return false;
}
boolean tecladoFuncaoPressionado() {
  boolean ret = false;
  digitalWrite(kb_rowPins[3], HIGH);
  ret = (digitalRead(kb_colPins[0]) == HIGH || digitalRead(kb_colPins[2]) == HIGH);
  digitalWrite(kb_rowPins[3], LOW);
  return ret;
}