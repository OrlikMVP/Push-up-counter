#include <HCMAX7219.h>
#include "SPI.h"
#define LOAD 10
#include "Adafruit_VL53L0X.h"
HCMAX7219 HCMAX7219(LOAD);
Adafruit_VL53L0X lox = Adafruit_VL53L0X();
int count=0, POWN=0,steck=25;
float dist_3[3] = {0.0, 0.0, 0.0};   // массив для хранения трёх последних измерений
float top,middle, dist, dist_filtered;
float k;
byte i, delta;

int flag1=0;

#define button1B A3  // пин кнопки button1

boolean button1S;   // храним состояния кнопок (S - State)
boolean button1F;   // флажки кнопок (F - Flag)
boolean button1R;   // флажки кнопок на отпускание (R - Release)
boolean button1P;   // флажки кнопок на нажатие (P - Press)
boolean button1H;   // флажки кнопок на удержание (многократный вызов) (H - Hold)
boolean button1HO;  // флажки кнопок на удержание (один вызов при нажатии) (HO - Hold Once)
boolean button1D;   // флажки кнопок на двойное нажатие (D - Double)
boolean button1DP;  // флажки кнопок на двойное нажатие и отпускание (D - Double Pressed)

#define double_timer 1000   // время (мс), отведённое на второе нажатие
#define hold 1000           // время (мс), после которого кнопка считается зажатой
#define debounce 1000        // (мс), антидребезг
unsigned long button1_timer; // таймер последнего нажатия кнопки
unsigned long button1_double; // таймер двойного нажатия кнопки


void setup() {
  Serial.begin(9600);

  pinMode(button1B, INPUT_PULLUP);
  pinMode(2,OUTPUT);
  
  if (!lox.begin()) {
    Serial.println(F("Failed to boot VL53L0X"));
    while(1);
  }
  // power 
  Serial.println(F("VL53L0X API Simple Ranging example\n\n")); 
}

void loop() {
  
  //-------опрос кнопок--------
  button1S = !digitalRead(button1B);
  digitalWrite(2,!digitalRead(button1B));
  buttons(); //отработка кнопок
  //-------опрос кнопок--------

  // отработка режимов (опускание флага обязательно!)
 if (button1H && button1HO) {
    Serial.println("hold once");
    button1HO = 0;
    POWN=0;
    if (flag1 == 1)
      flag1 = 0;
    else
      flag1 = 1;
  }
    
    
 if (flag1==0){ HCMAX7219.Clear();
 HCMAX7219.Refresh(); }   
 if (flag1==1){work();} 

}

void work() {
if (button1H && button1HO) {
    Serial.println("hold once");
    POWN=0;
    button1HO = 0;
    if (flag1 == 1)
      flag1 = 0;
    else
      flag1 = 1;
      
  }

if (button1P) {
    Serial.println("pressed");
    HCMAX7219.Clear();
    button1P = 0;
    count=0;
  }
    
  top=dist_filtered;
VL53L0X_RangingMeasurementData_t measure;
     Serial.println("Reading a measurement... ");
  lox.rangingTest(&measure, false); // pass in 'true' to get debug data printout!

  if (measure.RangeStatus != 4) {  // phase failures have incorrect data
    
  } else {
    Serial.println(" out of range ");
  }
    dist_3[0] = measure.RangeMilliMeter;                 // получить расстояние в текущую ячейку массива
    delay(50);
    dist_3[1] = measure.RangeMilliMeter;
    delay(50);
    dist_3[2] = measure.RangeMilliMeter;
    delay(50);
    
    dist = middle_of_3(dist_3[0], dist_3[1], dist_3[2]);    // фильтровать медианным фильтром из 3ёх последних измерений

    delta = abs(dist_filtered - dist);                      // расчёт изменения с предыдущим
    if (delta > 1) k = 0.7;                                 // если большое - резкий коэффициент
    else k = 0.1;                                           // если маленькое - плавный коэффициент

    dist_filtered = dist * k + dist_filtered * (1 - k);     // фильтр "бегущее среднее"

     
    
if( (top - dist_filtered > 100)&&( dist_filtered < 450 )){
 count=count+1;
 delay(150);
  }
 HCMAX7219.print7Seg(count,8);

 HCMAX7219.print7Seg(POWN,5);
 HCMAX7219.Refresh();
if (count == steck){
  HCMAX7219.Clear();
  count=0;
  HCMAX7219.print7Seg("GOOD", 4);
  POWN=POWN+1;
  HCMAX7219.Refresh();
  delay(1000);
  }
if (POWN == 3){steck=steck-5;}
if (POWN == 5){steck=steck-10;}
if (POWN == 6){POWN=0;} 
delay(500);
}
float middle_of_3(float a, float b, float c) {
  if ((a <= b) && (a <= c)) {
    middle = (b <= c) ? b : c;
  }
  else {
    if ((b <= a) && (b <= c)) {
      middle = (a <= c) ? a : c;
    }
    else {
      middle = (a <= b) ? a : b;
    }
  }
  return middle;
}


//------------------------ОТРАБОТКА КНОПОК-------------------------
void buttons() {
  //-------------------------button1--------------------------
  // нажали (с антидребезгом)
  if (button1S && !button1F && millis() - button1_timer > debounce) {
    button1F = 1;
    button1HO = 1;
    button1_timer = millis();
  }
  // если отпустили до hold, считать отпущенной
  if (!button1S && button1F && !button1R && !button1DP && millis() - button1_timer < hold) {
    button1R = 1;
    button1F = 0;
    button1_double = millis();
  }
  // если отпустили и прошло больше double_timer, считать 1 нажатием
  if (button1R && !button1DP && millis() - button1_double > double_timer) {
    button1R = 0;
    button1P = 1;
  }
  // если отпустили и прошло меньше double_timer и нажата снова, считать что нажата 2 раз
  if (button1F && !button1DP && button1R && millis() - button1_double < double_timer) {
    button1F = 0;
    button1R = 0;
    button1DP = 1;
  }
  // если была нажата 2 раз и отпущена, считать что была нажата 2 раза
  if (button1DP && millis() - button1_timer < hold) {
    button1DP = 0;
    button1D = 1;
    button1_timer = millis();
  }
  // Если удерживается более hold, то считать удержанием
  if (button1F && !button1D && !button1H && millis() - button1_timer > hold) {
    button1H = 1;
  }
  // Если отпущена после hold, то считать, что была удержана
  if (!button1S && button1F && millis() - button1_timer > hold) {
    button1F = 0;
    button1H = 0;
    button1_timer = millis();
  }
  //-------------------------button1--------------------------
}
//------------------------ОТРАБОТКА КНОПОК-------------------------
  
