/* 
 *    SCL = A5
 *    SDA = A4
 *    VCC = 5V
 *    GND = GND
 */
#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
void printHora(int hora, int minutos, int segundos);
void screenrefresh();
void handleTimeRollover();
void handleAlarmRollover();




LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7); //addr, EN, RW, RS, D4, D5, D6, D7, BacklightPin, POLARITY
const int relayPin = 8;
const int hsPin = 1; //el del boton del a hora!!
const int minPin = 2; //el de los minuto!!
const int switchPin = 3;

const int timeout = 20; //segs

// horas iniciales
int h = 21;
int m = 00;
int s = 00;

int al_h = 5;
int al_m = 00;
int al_s = 00;

int currTimeout = timeout;

// Time Set Buttons
int buttonHs;
int buttonMin;
int buttonSwitcher;
int apretaronAlgo = 0;

int qCambia = 0; //0 es hora, 1 es hora de alarma

// For accurate Time reading, use Arduino Real Time Clock and not just delay()
static uint32_t last_time, now = 0; // RTC

void setup() {
  pinMode(hsPin, INPUT_PULLUP);
  pinMode(minPin, INPUT_PULLUP);
  pinMode(switchPin, INPUT_PULLUP);
  lcd.setBacklightPin(3, POSITIVE); // POSITIVE = enum definido en LCD.h
  now = millis();
  lcd.begin(16, 2);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); //high es apagado because reasons || update: con mi nuevo rele de estado solido, low es apagado!
}

void loop() {
  screenrefresh();

  for (int i = 0; i < 4; i++) // 4 checks per second
  {

    while ((now - last_time) < 250) //delay250ms
      now = millis();

    last_time = now; // prepare for next loop 

    // read Setting Buttons
    buttonHs = digitalRead(hsPin);
    buttonMin = digitalRead(minPin);
    buttonSwitcher = digitalRead(switchPin);

    apretaronAlgo = ((buttonHs == 0) | (buttonMin == 0) | (buttonSwitcher == 0));

    if (apretaronAlgo & (currTimeout > 0)) {
      currTimeout=timeout;
      //handling button input

      if (buttonSwitcher == 0)
        qCambia = !qCambia;

      if (buttonHs == 0)
        if (qCambia)
          al_h++;
        else
          h++;

      if (buttonMin == 0)
        if (qCambia) {
          al_s = 0;
          al_m++;
        }
      else {
        s = 0;
        m++;
      }

      //DONE handling button input

      if (qCambia)
        handleAlarmRollover();
      else
        handleTimeRollover();

      screenrefresh();
    } else
    if (apretaronAlgo) {
      lcd.on();
      currTimeout = timeout;
    }

  } //end screen off and butt pressed if

  /* outer 1000ms loop  */
  s++; //increment sec. counting

  // ---- manage seconds, minutes, hours am/pm overflow ----

  handleTimeRollover();

  if (h == al_h && m == al_m)
    alarmTime();

  //Backlight time out 
  if (currTimeout > 0)
    currTimeout--;
  else
    lcd.off();

  // Loop end
}

void printHora(int hora, int minutos, int segundos) {
  if (hora < 10)
    lcd.print("0"); // always 2 digits
  lcd.print(hora);

  lcd.print(":");

  if (minutos < 10)
    lcd.print("0");
  lcd.print(minutos);

  lcd.print(":");

  if (segundos < 10)
    lcd.print("0");
  lcd.print(segundos);
}

void screenrefresh() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T: ");
  printHora(h, m, s);
  if (!qCambia)
    lcd.print("<- ");
  else
    lcd.print("   ");
    
  lcd.print(currTimeout);

  lcd.setCursor(0, 1);
  lcd.print("A: ");
  printHora(al_h, al_m, al_s);
  if (qCambia)
    lcd.print("<-");
}

void handleTimeRollover() {
  if (s == 60) {
    s = 0;
    m++;
  }

  if (m == 60) {
    m = 0;
    h++;
  }

  if (h == 24)
    h = 0;
}

void handleAlarmRollover() {
  if (al_m == 60) {
    al_m = 0;
    al_h++;
  }
  if (al_h == 24)
    al_h = 0;
}

void alarmTime() { //this will ruin the timekeeping when the alarm is blaring but w/e
  lcd.off();
  apretaronAlgo = ((buttonHs == 0) | (buttonMin == 0) | (buttonSwitcher == 0));
  while (!apretaronAlgo) {//todos los high y low-s fueron invertidos cuando em cambie al SSR!
    digitalWrite(relayPin, HIGH);
    delay(2000);
    for (int i = 0; i < 5; i++) {
      digitalWrite(relayPin, HIGH);
      delay(200);
      digitalWrite(relayPin, LOW);
      delay(200);
    }

    apretaronAlgo = ((buttonHs == 0) | (buttonMin == 0) | (buttonSwitcher == 0));
  }

  lcd.on();
}
