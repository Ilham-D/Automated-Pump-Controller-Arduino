#include <Fuzzy.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>

// Pin data DS18B20 terhubung ke pin digital D2
#define ONE_WIRE_BUS 2
#define Relay_Pin 8

// Inisiasi LCD 
LiquidCrystal_I2C lcd(0x3F, 16, 4); // 0x3F (Rangkaian Asli) ^ 0x20 (Simulasi Proteus)

// Inisialisasi instance OneWire untuk berkomunikasi dengan perangkat OneWire
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

const int mq135Pin = A0; // pin sensor mq-135
const float RLOAD = 10.0;  // Beban resistor (dalam kilo-ohm)
const float RZERO = 76.63;  // Nilai R0 yang diperoleh dari kalibrasi (dalam kilo-ohm)
const float PARA = 116.6020682;  // Konstanta dari datasheet
const float PARB = 2.769034857;  // Konstanta dari datasheet

float TimerPompaValue = 0;
float temperatureC = 0;
float AmmoniaValue = 0;

// Membuat objek fuzzy
Fuzzy *fuzzy = new Fuzzy();

// Input variable suhu
FuzzySet *Dingin = new FuzzySet(0, 0, 20, 25);
FuzzySet *Normal = new FuzzySet(20, 25, 30, 35);
FuzzySet *Panas = new FuzzySet(30, 35, 100, 100);

// Input variable NH3 (dikali 1000 biar nilainya integer)
FuzzySet *Kurang = new FuzzySet(0, 0, 25, 50);
FuzzySet *Cukup = new FuzzySet(25, 50, 75, 100);
FuzzySet *Lebih = new FuzzySet(75, 100, 150, 150);

// Output variable lama waktu pompa air
FuzzySet *Off = new FuzzySet(0, 0, 0, 0);
FuzzySet *Lama = new FuzzySet(60, 60, 60, 60);
FuzzySet *Cepat = new FuzzySet(30, 30, 30, 30);

extern unsigned int __heap_start;
extern void *__brkval;

int freeMemory() {
  int free_memory;
  if ((int)__brkval == 0) {
    free_memory = ((int)&free_memory) - ((int)&__heap_start);
  } else {
    free_memory = ((int)&free_memory) - ((int)__brkval);
  }
  return free_memory;
}

void setup() {
  Serial.begin(9600);
  lcd.begin();
  lcd.setCursor(0, 0);
  lcd.print(F("Initializing"));
  Serial.println(F("Initializing"));
  pinMode(Relay_Pin, OUTPUT);
  digitalWrite(Relay_Pin, HIGH);
  sensors.begin();

  // Mendefinisikan input fuzzy untuk suhu
  FuzzyInput *temperatur = new FuzzyInput(1);
  temperatur->addFuzzySet(Dingin);
  temperatur->addFuzzySet(Normal);
  temperatur->addFuzzySet(Panas);
  fuzzy->addFuzzyInput(temperatur);

  // Mendefinisikan input fuzzy untuk NH3
  FuzzyInput *Ammonia = new FuzzyInput(2);
  Ammonia->addFuzzySet(Kurang);
  Ammonia->addFuzzySet(Cukup);
  Ammonia->addFuzzySet(Lebih);
  fuzzy->addFuzzyInput(Ammonia);

  // Mendefinisikan output fuzzy waktu pompa air
  FuzzyOutput *Timer = new FuzzyOutput(1);
  Timer->addFuzzySet(Off);
  Timer->addFuzzySet(Cepat);
  Timer->addFuzzySet(Lama);
  fuzzy->addFuzzyOutput(Timer);

  // Mendefinisikan aturan fuzzy
  defineFuzzyRules();
  delay(1000);
  lcd.clear();
  lcd.setCursor(0, 2);
  lcd.print(F("Pump is OFF"));
  Serial.println(F("Device Initialized"));
  Serial.print(F("Free Memory after setup: "));
  Serial.print(freeMemory());
  Serial.println(F(" byte"));
  Serial.println(F(" "));
}

void loop() {
  sensors.requestTemperatures();
  delay(1000);
  temperatureC = sensors.getTempCByIndex(0);


  if (temperatureC == -127.00) {
    Serial.println(F("Temperature Sensor not Ready"));
    delay(2000);
    return;
  }

  AmmoniaValue = getAmmonia();


  fuzzy->setInput(1, temperatureC);
  fuzzy->setInput(2, AmmoniaValue * 1000);

  fuzzy->fuzzify();
  TimerPompaValue = fuzzy->defuzzify(1);

  
  // debug hasil sensor
  Serial.print(F("Konsentrasi Ammonia : "));
  Serial.print(AmmoniaValue);
  Serial.println(F(" ppm"));
  Serial.print(F("Temperature: "));
  Serial.print(temperatureC);
  Serial.println(F(" C"));
  Serial.print(F("Timer : "));
  Serial.print(TimerPompaValue);
  Serial.println(F(" detik"));

  //Tampilan LCD Baris Pertama
  lcdFirstLine(temperatureC, AmmoniaValue); 

  //Tampilan LCD Baris Kedua
  lcdSecondLine(temperatureC, AmmoniaValue);
  

  debugFuzzyRules();

  if (TimerPompaValue > 0) {
    Serial.println(F("Relay ON"));
    lcd.setCursor(0, 2);
    lcd.print(F("Pump is Active"));
    digitalWrite(Relay_Pin, LOW);
    unsigned long startTime = millis();
    unsigned long delayTime = TimerPompaValue * 1000;
    while (millis() - startTime < delayTime) {
      unsigned long remainingTime = (delayTime - (millis() - startTime)) / 1000;
      lcd.setCursor(0, 3);
      lcd.print(F("Remaining : "));
      if(String(remainingTime).length() == 1){
      lcd.setCursor(14, 3);
      lcd.print(remainingTime);
      lcd.setCursor(15, 3);
      lcd.print(F("s")); 
      }else {
      lcd.setCursor(13, 3);
      lcd.print(remainingTime);
      lcd.setCursor(15, 3);
      lcd.print(F("s")); 
      }
      
      Serial.print(F("Waiting... "));
      Serial.print(remainingTime);
      Serial.println(F(" detik tersisa"));
      delay(1000); // Tunda selama 1 detik sebelum update waktu tersisa
      lcd.setCursor(13, 3);
      lcd.print(F("   "));
    }
    lcd.setCursor(0, 3);
    lcd.print(F("               "));
    Serial.println(F("Relay OFF"));
    lcd.setCursor(0, 2);
    lcd.print(F("Pump is OFF    "));
    digitalWrite(Relay_Pin, HIGH);
  }
  Serial.print(F("Free Memory in loop: "));
  Serial.print(freeMemory());
  Serial.println(F(" byte"));
  Serial.println(F(" "));
  delay(1000);
}

float getAmmonia() {
  int analogValue = analogRead(mq135Pin);
  float voltage = analogValue * (5.0 / 1023.0);
  float RS = (5.0 - voltage) / voltage * RLOAD;
  float ratio = RS / RZERO;
  float ppm = PARA * pow(ratio, -PARB);
  return ppm;
}

void defineFuzzyRules() {

  // mendefinisikan output then
  FuzzyRuleConsequent *thenTimerOff = new FuzzyRuleConsequent();
  thenTimerOff->addOutput(Off);
  FuzzyRuleConsequent *thenTimerCepat = new FuzzyRuleConsequent();
  thenTimerCepat->addOutput(Cepat);
  FuzzyRuleConsequent *thenTimerLama = new FuzzyRuleConsequent();
  thenTimerLama->addOutput(Lama);


  // Definisi aturan fuzzy
  FuzzyRuleAntecedent *If_Temp_Dingin_And_Ammonia_Kurang_Then_Cepat = new FuzzyRuleAntecedent();
  If_Temp_Dingin_And_Ammonia_Kurang_Then_Cepat->joinWithAND(Dingin, Kurang);
  FuzzyRule *FuzzyRule1 = new FuzzyRule(1, If_Temp_Dingin_And_Ammonia_Kurang_Then_Cepat, thenTimerCepat);
  fuzzy->addFuzzyRule(FuzzyRule1);

  FuzzyRuleAntecedent *If_Temp_Dingin_And_Ammonia_Cukup_Then_Cepat = new FuzzyRuleAntecedent();
  If_Temp_Dingin_And_Ammonia_Cukup_Then_Cepat->joinWithAND(Dingin, Cukup);
  FuzzyRule *FuzzyRule2 = new FuzzyRule(2, If_Temp_Dingin_And_Ammonia_Cukup_Then_Cepat,thenTimerCepat);
  fuzzy->addFuzzyRule(FuzzyRule2);

  FuzzyRuleAntecedent *If_Temp_Dingin_And_Ammonia_Lebih_Then_Lama = new FuzzyRuleAntecedent();
  If_Temp_Dingin_And_Ammonia_Lebih_Then_Lama->joinWithAND(Dingin, Lebih);
  FuzzyRule *FuzzyRule3 = new FuzzyRule(3, If_Temp_Dingin_And_Ammonia_Lebih_Then_Lama, thenTimerLama);
  fuzzy->addFuzzyRule(FuzzyRule3);

  FuzzyRuleAntecedent *If_Temp_Normal_And_Ammonia_Kurang_Then_Off = new FuzzyRuleAntecedent();
  If_Temp_Normal_And_Ammonia_Kurang_Then_Off->joinWithAND(Normal, Kurang);
  FuzzyRule *FuzzyRule4 = new FuzzyRule(4, If_Temp_Normal_And_Ammonia_Kurang_Then_Off, thenTimerOff);
  fuzzy->addFuzzyRule(FuzzyRule4);

  FuzzyRuleAntecedent *If_Temp_Normal_And_Ammonia_Cukup_Then_Off = new FuzzyRuleAntecedent();
  If_Temp_Normal_And_Ammonia_Cukup_Then_Off->joinWithAND(Normal, Cukup);
  FuzzyRule *FuzzyRule5 = new FuzzyRule(5, If_Temp_Normal_And_Ammonia_Cukup_Then_Off, thenTimerOff);
  fuzzy->addFuzzyRule(FuzzyRule5);

  FuzzyRuleAntecedent *If_Temp_Normal_And_Ammonia_Lebih_Then_Lama = new FuzzyRuleAntecedent();
  If_Temp_Normal_And_Ammonia_Lebih_Then_Lama->joinWithAND(Normal, Lebih);
  FuzzyRule *FuzzyRule6 = new FuzzyRule(6, If_Temp_Normal_And_Ammonia_Lebih_Then_Lama, thenTimerLama);
  fuzzy->addFuzzyRule(FuzzyRule6);

  FuzzyRuleAntecedent *If_Temp_Panas_And_Ammonia_Kurang_Then_Cepat = new FuzzyRuleAntecedent();
  If_Temp_Panas_And_Ammonia_Kurang_Then_Cepat->joinWithAND(Panas, Kurang);
  FuzzyRule *FuzzyRule7 = new FuzzyRule(7, If_Temp_Panas_And_Ammonia_Kurang_Then_Cepat, thenTimerCepat);
  fuzzy->addFuzzyRule(FuzzyRule7);

  FuzzyRuleAntecedent *If_Temp_Panas_And_Ammonia_Cukup_Then_Cepat = new FuzzyRuleAntecedent();
  If_Temp_Panas_And_Ammonia_Cukup_Then_Cepat->joinWithAND(Panas, Cukup);
  FuzzyRule *FuzzyRule8 = new FuzzyRule(8, If_Temp_Panas_And_Ammonia_Cukup_Then_Cepat, thenTimerCepat);
  fuzzy->addFuzzyRule(FuzzyRule8);

  FuzzyRuleAntecedent *If_Temp_Panas_And_Ammonia_Lebih_Then_Lama = new FuzzyRuleAntecedent();
  If_Temp_Panas_And_Ammonia_Lebih_Then_Lama->joinWithAND(Panas, Lebih);
  FuzzyRule *FuzzyRule9 = new FuzzyRule(9, If_Temp_Panas_And_Ammonia_Lebih_Then_Lama, thenTimerLama);
  fuzzy->addFuzzyRule(FuzzyRule9);
}



void debugFuzzyRules() {
  Serial.println(F("debug Fuzzy Rules:"));

  // Informasi aturan manual
  struct RuleInfo {
    int index;
    const char* description;
  };

  RuleInfo rules[] = {
    {1, "if suhu dingin and ammonia kurang then cepat"},
    {2, "if suhu dingin and ammonia cukup then cepat"},
    {3, "if suhu dingin and ammonia lebih then lama"},
    {4, "if suhu normal and ammonia kurang then Off"},
    {5, "if suhu normal and ammonia cukup then Off"},
    {6, "if suhu normal and ammonia lebih then lama"},
    {7, "if suhu panas and ammonia kurang then cepat"},
    {8, "if suhu panas and ammonia cukup then cepat"},
    {9, "if suhu panas and ammonia lebih then lama"},
  };

  for (auto& rule : rules) {
    bool fired = fuzzy->isFiredRule(rule.index);
    Serial.print(rule.index);
    Serial.print(F(". "));
    Serial.print(rule.description);
    Serial.print(F(": "));
    Serial.print(fired ? "Fired" : "Not Fired");
    Serial.println();
  }
}

void lcdFirstLine(float temperatureC, float AmmoniaValue){
  lcd.setCursor(0, 0);
  lcd.print(F("T:"));
  lcd.setCursor(2, 0);
  lcd.print(int(floor(temperatureC)));
  lcd.print(F("C"));
  if (String(int(floor(temperatureC))).length() == 2){
  lcd.print(F(" A:"));
  lcd.print(AmmoniaValue);
  lcd.print(F("ppm"));
  } else if (String(int(floor(temperatureC))).length() == 1){
  lcd.print(F("  A:"));
  lcd.print(AmmoniaValue);
  lcd.print(F("ppm"));
  }

}
  
void lcdSecondLine(float temperatureC, float AmmoniaValue){
   lcd.setCursor(0, 1);
  if (temperatureC <= 20){
    lcd.print(F("T:Dingin"));
  }else if(temperatureC >= 20 && temperatureC <= 25){
    lcd.print(F("T:D&N   "));
  }else if(temperatureC >= 25 && temperatureC <= 30){
    lcd.print(F("Normal"));
  }else if(temperatureC >= 30 && temperatureC <= 35){
    lcd.print(F("T:N&P   "));
  }else if(temperatureC >= 35){
    lcd.print(F("T:Panas "));
  }

  if(AmmoniaValue <= 0.025){
    lcd.print(F("A:Kurang"));
  }else if(AmmoniaValue >= 0.025 && AmmoniaValue <= 0.05){
    lcd.print(F("A:K&C"));
  }else if(AmmoniaValue >= 0.05 && AmmoniaValue <= 0.075){
    lcd.print(F("A:Cukup"));
  }else if(AmmoniaValue >= 0.075 && AmmoniaValue <= 0.1){
    lcd.print(F("A:C&L"));
  }else if(AmmoniaValue >= 0.1){
    lcd.print(F("A:Lebih"));
  }
}

