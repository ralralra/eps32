#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#define DHTTYPE DHT11 
LiquidCrystal_I2C lcd(0x20, 16, 2);


int light = A3;  //빛센서
int temp = 9;
DHT dht(temp, DHTTYPE);
void setup() {
  lcd.init();
  lcd.backlight();
}

void loop() {
  int v = analogRead(light);
  int h = dht.readHumidity();
  float t = dht.readTemperature();

  lcd.setCursor(0, 0);  //칸 번호, 줄번호
  lcd.print("now light:");
  lcd.print(v);
  lcd.setCursor(0, 1);  //칸 번호, 줄번호
  lcd.print("Hum:");
  lcd.print(h);
  lcd.print(" tem:");
  lcd.print(t);
}
