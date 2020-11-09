#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

#include <IRremote.h>

int RECV_PIN = 2;
IRrecv irrecv(RECV_PIN);
decode_results results;
int lcdPointer = 0;
int dayOfWeek = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;
long oldTime;
bool backLight = true;

#define UP "807f22dd"
#define DOWN "807f12ed"
#define RIGHT "807f02fd"
#define LEFT "807f32Cd"
#define OK "807f609f"
#define OFF "807f00ff"

//vlhkomer
#define displayOnPin A1

#define analogPin A0 
#define digitalPin 3
#define vccPin 4

#define pumpPin A2
#define checkingTime 3000

#define trigPin 5
#define echoPin 6

#define dryness 800
#define wetness 500
// proměnná pro uložení času kontroly
unsigned long cas = 0;
unsigned long check = 0;
boolean pumping = false;
double r = 5; //polomer nadoby

const int Rpin = 9;
const int Gpin = 10;
const int Bpin = 11;
int R = 255;
int G = 255;
int B = 255;

double actual = 0;
int counter = 0;
double average = 0; 
double first = 0;

void setup() {
  Serial.begin(9600);

  irrecv.enableIRIn();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Zapinam :)...");
  
  pinMode(Rpin, OUTPUT);
  pinMode(Gpin, OUTPUT);
  pinMode(Bpin, OUTPUT);
  analogWrite(Rpin, R);
  delay(300);
  analogWrite(Rpin, 0);
  delay(300);
  analogWrite(Gpin, G);
  delay(300);
  analogWrite(Gpin, 0);
  delay(300);
  analogWrite(Bpin, B);
  delay(300);
  analogWrite(Bpin, 0);
  delay(300);
  for (int i = 0; i < 200; i++) {
    R = i;
    analogWrite(Rpin, R);
  }
  delay(100);
  analogWrite(Rpin, 0);
  delay(100);
  analogWrite(Rpin, R);

  pinMode(analogPin, INPUT); //vlhkomer
  pinMode(digitalPin, INPUT); //vlhkomer
  pinMode(vccPin, OUTPUT); //vlhkomer
  pinMode(pumpPin, OUTPUT); //cerpadlo

  pinMode(trigPin, OUTPUT); // ultrasonic
  pinMode(echoPin, INPUT); // ultrasonic
  delay(150);

  
  // vypnutí napájecího napětí pro modul
  digitalWrite(vccPin, LOW);
}

double wL = 0;
void loop() {
  addTime();
  if (millis() - check > 1000) {
    if (millis() - cas > checkingTime) {
    //je sucho
      if (humidity() > dryness) {
        pumping = true;
        load();
      }
    }
    
    wL = waterLevel();
    if (pumping && wL > 1) {
      digitalWrite(pumpPin, HIGH); //watering
      if (humidity() <= wetness) {
        save();
        pumping = false;
      }
    }
    else {
      digitalWrite(pumpPin, LOW);
    }
    check = millis();

    lcdPrint();
  }

  averness(wL);
  controll();
  
  analogWrite(Rpin, R);
  analogWrite(Gpin, G);
  analogWrite(Bpin, B);

  delay(10);
}

void averness(int wL) {
  if (wL <= 5 && wL > 1) {
    warning(1);
  }
  else if (wL <= 1) {
    danger(3);
  }
  else {
    ok(1);
  }
}

double waterLevel() {
  int height = 21;

  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  //delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  int duration = pulseIn(echoPin, HIGH);

  double distance = duration*0.034/2;

  Serial.print("Hladina vody: ");
  Serial.println(height - distance);
  actual = (3.14 * r*r * (height - distance))/1000;
  return height - distance;
}

int humidity() {
  // zapneme napájecí napětí pro modul s krátkou pauzou
  // pro ustálení
  digitalWrite(vccPin, HIGH);
  delay(100);
  // načtení analogové a digitální hodnoty do proměnných
  int analog = analogRead(analogPin);
  bool digit = digitalRead(digitalPin);
  // výpis analogové hodnoty po sériové lince
  Serial.print("Vlhkost: ");
  Serial.print(analog);
  if (digit == HIGH) {
    Serial.print(" | Detekovano prekroceni hranice!");
  }
  // ukončení řádku na sériové lince
  Serial.println();
  digitalWrite(vccPin, LOW);
  cas = millis();

  return analog;
}

void load() {
  first = waterLevel();
}

void save() {
  double sum = counter * average;
  counter++; 
  double h = first - waterLevel();
  average = ((3.14 * r*r * h) + sum) / counter;
}

void lcdPrint() {
  lcd.clear();
    
  String ar[] = {printTime(),
    "Zbyva: " + String(actual) + " l",
    "Prumer: " + String(average) + " ml",
    "Zalito: " + String(counter)};
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(ar[lcdPointer]);
  lcd.setCursor(0, 1);
  if (lcdPointer + 1 > 2) lcd.print(ar[0]);
  else lcd.print(ar[lcdPointer + 1]);
}

String printTime() {
  String h = "";
  String m = "";
  String s = "";
  
  if (hours < 10) h = "0" + String(hours);
  else h = String(hours);
  
  if (minutes < 10) m = "0" + String(minutes);
  else m = String(minutes);

  if (seconds < 10) s = "0" + String(seconds);
  else s = String(seconds);

  return (dayString(dayOfWeek) + " - " + h + ":" + m + ":" + s);
  
}

String dayString(int day) {
  switch (day) {
    case 0: return "PO";
    case 1: return "UT";
    case 2: return "ST";
    case 3: return "CT";
    case 4: return "PA";
    case 5: return "SO";
    case 6: return "NE";
  }
}

void addTime() {  
  int dif = millis() - oldTime;
  if (dif >= 1000) {
    seconds++;
    oldTime = millis() + dif;
  }
  if (seconds >= 60) {
    minutes++;
    seconds = 0;
  }
  if (minutes >= 60) {
    hours++;
    minutes = 0;
  }
  if (hours >= 24) {
    dayOfWeek++;
    hours = 0;
  }
  if (dayOfWeek > 6) {
    dayOfWeek = 0;
  }
}

int hoursPointer = 0;
bool settingHours = false;
void controll() {
  if (settingHours) {
    lcd.setCursor(15, 0);
    lcd.print(String(hoursPointer));
  }
  
  if (irrecv.decode(&results)) {
    String sgn = String(results.value, HEX);
    Serial.println(sgn);
    if (sgn == UP) {
      if (settingHours) {
        addHours();
      } 
      else {
        lcdPointer++;
        if (lcdPointer >= 4) lcdPointer = 0;
      }
    }
    else if (sgn == DOWN) {
      if (settingHours) {
        removeHours();
      } 
      else {
        lcdPointer--;
        if (lcdPointer < 0) lcdPointer = 3;
      }
    }
    else if (sgn == OFF) {
      if (backLight) {
        lcd.noBacklight();
        backLight = false;
      }
      else {
        lcd.backlight();
        backLight = true;
      }
    }
    else if (sgn == OK) {
      if (settingHours) settingHours = false;
      else settingHours = true;
    }
    else if (sgn == RIGHT) {
      if (settingHours) {
        hoursPointer++;
        if (hoursPointer > 3) hoursPointer = 0;
      }
    }
    else if (sgn == LEFT) {
      if (settingHours) {
        hoursPointer--;
        if (hoursPointer < 0) hoursPointer = 2;
      }
    }
    irrecv.resume(); // Receive the next value
  }
}

void addHours() {
  switch (hoursPointer) {
    case 0: //dny
      dayOfWeek++;
      if (dayOfWeek > 6) dayOfWeek = 0;
      break;
    case 1: //hodiny
      hours++;
      if (hours > 59) hours = 0;
      break;
    case 2: //minuty
      minutes++;
      if (minutes > 59) minutes = 0;
      break;
    case 3: //vteriny
      seconds++;
      if (seconds > 59) seconds = 0;
      break;
  }
}

void removeHours() {
  switch (hoursPointer) {
    case 0: //dny
      dayOfWeek--;
      if (dayOfWeek < 0) dayOfWeek = 6;
      break;
    case 1: //hodiny
      hours--;
      if (hours < 0) hours = 59;
      break;
    case 2: //minuty
      minutes--;
      if (minutes < 0) minutes = 59;
      break;
    case 3: //vteriny
      seconds--;
      if (seconds < 0) seconds = 59;
      break;
  }
}

bool rD = true;
void danger(int sp) {
  if (rD) R += sp;
  else R -= sp;
  G = 0;
  B = 0;

  if (R >= 255) rD = false;
  if (R <= 0) rD = true;
}

bool g = true;
void ok(int sp) {
  if (g) G += sp;
  else G -= sp; 
  R = 0;
  B = 0;

  if (G >= 200) g = false;
  if (G <= 100) g = true;
  
}

bool b = true;
void warning(int sp) {
  if (b) G += sp;
  else G -= sp; 
  R = 50;
  B = 0;

  if (G >= 150) b = false;
  if (G <= 0) b = true;
}
