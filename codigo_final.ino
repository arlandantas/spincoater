#include <LiquidCrystal.h>

#define INICIAL 123
#define PIN_MOTOR 8
int ESTADO_ATUAL = 0;
int ESTADO_ACELERACAO = 0;
int tamanho = 0, velocidade = 0, tempo = 0;
unsigned long millis_atual = 0, millis_anterior = 0, millis_inicio = 0, tempo_millis = 0;
int PWM_ATUAL = 0, PWM_DESEJADO = 0;

const byte kb_cols = 3;
const byte kb_rows = 4;
char keys[kb_rows][kb_cols] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

byte kb_colPins[kb_cols] = {24,22,26};
byte kb_rowPins[kb_rows] = {23,28,27,25};

int VELOCIDADES[][14] = {
  {2500,3700,4800,5300,6000,6500,6900,7000,7300,7500,7700,8000,8300,8370},
  {2000,2200,2600,4000,4400,5000,6000,0,0,0,0,0,0,0},
  {3300,0,0,0,0,0,0,0,0,0,0,0,0,0}
};

int QTD_VELOCIDADES[] = {14, 7, 1};

int LIMITES_MIN[] = {2500,2000,1700};
int LIMITES_MAX[] = {8370,6000,4000};

int var = -1, n_var = -1;

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
  delay(3000);
  analogWrite(PIN_MOTOR, 115);
  delay(5000);
  analogWrite(PIN_MOTOR, 0);
}
void loop() {
  millis_atual = millis();
  switch (ESTADO_ATUAL) {
    case 0:
      resetLcd();
      lcd.print("Tamanho (1-3):");
      tamanho = lerTeclado();
      ++ESTADO_ATUAL;
      break;
    case 1:
      if (tamanho > 0 && tamanho < 4) {
        --tamanho;
        ESTADO_ATUAL = 3;
      } else {
        ESTADO_ATUAL = 2;
        millis_anterior = millis_atual;
        resetLcd();
        lcd.print("    TAMANHO     ");
        lcd.setCursor(0, 1);
        lcd.print("    INVALIDO    ");
      }
      break;
    case 2:
      if(millis_atual - millis_anterior >= 2000 || tecladoPressionado()) {
        ESTADO_ATUAL = 0;
        while(tecladoPressionado()) {}
      }
      break;
    case 3:
      resetLcd();
      lcd.print("Velocidade:");
      velocidade = lerTeclado();
      ++ESTADO_ATUAL;
      break;
    case 4:
      if (velocidade >= LIMITES_MIN[tamanho] && velocidade <= LIMITES_MAX[tamanho]) {
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
      if(millis_atual - millis_anterior >= 2000 || tecladoPressionado()) {
        ESTADO_ATUAL = 3;
        while(tecladoPressionado()) {}
      }
      break;
    case 6:
      resetLcd();
      lcd.print("Tempo Max:");
      tempo = lerTeclado();
      tempo_millis = tempo*1000;
      ++ESTADO_ATUAL;
      break;
    case 7:
      resetLcd();
      lcd.print("    AGUARDE,    ");
      lcd.setCursor(0, 1);
      lcd.print("  ACELERANDO... ");
      ESTADO_ACELERACAO = 0;
      
      var = -1;
      n_var = -1;
      for (int i = 0; i < QTD_VELOCIDADES[tamanho]; i++) {
        n_var = VELOCIDADES[tamanho][i] - velocidade;
        n_var = abs(n_var);
        if (var == -1 || n_var < var) {
          var = n_var;
          PWM_DESEJADO = (27+i)*5;
        }
      }
      var = 0;
      n_var = -1;
      /*lcd.setCursor(7, 0);
      lcd.print(tempo);
      lcd.setCursor(0, 1);
      lcd.print("segundos...");*/
      
      millis_inicio = millis_atual;
      ++ESTADO_ATUAL;
      break;
    case 8:
      if (PWM_ATUAL < PWM_DESEJADO && !tecladoFuncaoPressionado()) {
        executarAceleracao();
      } else {
        ++ESTADO_ATUAL;
      }
      break;
    case 9:
      if (millis_atual - millis_inicio >= tempo_millis || tecladoFuncaoPressionado()) {
        PWM_ATUAL = 115;
        analogWrite(PIN_MOTOR, PWM_ATUAL);
        resetLcd();
        lcd.print("   FINALIZANDO  ");
        lcd.setCursor(0, 1);
        lcd.print("    EXECUCAO    ");
        ++ESTADO_ATUAL;
        while(tecladoFuncaoPressionado()) {}
        millis_anterior = millis_atual;
      } else {
        int n_var = (millis_atual - millis_inicio)/1000;
        if (n_var > var) {
          var = n_var;
          lcd.setCursor(7, 0);
          lcd.print("          ");
          lcd.setCursor(7, 0);
          lcd.print(tempo - var);
        }
      }
      break;
    case 10:
      if (millis_atual - millis_anterior >= 4000) {
        ESTADO_ATUAL = 0;
      }
      break;
    default:
      break;
  }
  /*lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Insira um valor:");
  // Serial.print("Insira um valor: ");
  int lido = lerTeclado();
  // Serial.print("Valor lido do teclado: ");
  lcd.setCursor(0,1);
  lcd.print(lido);
  // Serial.println(lido);
  millis_atual = millis();*/
}

void executarAceleracao() {
  if (PWM_ATUAL < PWM_DESEJADO) {
    switch (ESTADO_ACELERACAO) {
      case 0:
        PWM_ATUAL = 123;
        analogWrite(PIN_MOTOR, PWM_ATUAL);
        millis_anterior = millis_atual;
        Serial.println(PWM_ATUAL);
        ESTADO_ACELERACAO = 1;
        break;
      case 1:
      /*Serial.print(millis_atual);
          Serial.print(" - ");
          Serial.print(millis_anterior);
          Serial.print(" = ");
          Serial.println(millis_atual - millis_anterior);*/
        if (millis_atual - millis_anterior >= 5000) {
          millis_anterior = 0;
          ++ESTADO_ACELERACAO;
        }
        break;
      case 2:
        switch (tamanho) {
          case 0:
            if (millis_anterior == 0 || millis_atual - millis_anterior >= 3000) {
              PWM_ATUAL += 5;
              //Serial.println(PWM_ATUAL);
              if (PWM_ATUAL > PWM_DESEJADO) {
                PWM_ATUAL = PWM_DESEJADO;
              }
              if (PWM_ATUAL > 145) {
                ++ESTADO_ACELERACAO;
              }
              millis_anterior = millis_atual;
            }
            break;
          case 1:
            break;
          case 2:
            break;
        }
        analogWrite(PIN_MOTOR, PWM_ATUAL);
        break;
      case 3:
        switch (tamanho) {
          case 0:
            if (millis_anterior == 0 || millis_atual - millis_anterior >= 5000) {
              PWM_ATUAL += 2;
              if (PWM_ATUAL > PWM_DESEJADO) {
                PWM_ATUAL = PWM_DESEJADO;
              }
              millis_anterior = millis_atual;
            }
            break;
          case 1:
            break;
          case 2:
            break;
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
              // Serial.write(ch);
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
  // Serial.println();
  // Serial.print("Valor: ");
  // Serial.println(ret);
  //delay(2000);
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
