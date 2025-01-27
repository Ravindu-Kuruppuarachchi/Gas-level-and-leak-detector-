#include <Arduino.h>
#include "HX711.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

#define ledPin 7
#define sensorPin A0

float day[7];  // Array to store the daily usage for the past 7 days
float previousSum = 0; // Variable to store the previous sum reading
float avgusg = 0;

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN_1 = 3;
const int LOADCELL_SCK_PIN_1 = 2;
const int LOADCELL_DOUT_PIN_2 = 5;
const int LOADCELL_SCK_PIN_2 = 4;

HX711 scale1;
HX711 scale2;
// Display
LiquidCrystal_I2C lcd(0x27, 16, 2);

unsigned long previousMillis = 0; // Stores the start time of the last 24-hour period
const unsigned long dayInterval = 86400000; // 24 hours in milliseconds

void setup() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  lcd.init();         // Initialize the LCD
  lcd.backlight();    // Turn on the LCD screen backlight
  Serial.begin(57600);
  previousMillis = millis();
  Serial.println("HX711 Demo");
  Serial.println("Initializing the scale");
  scale1.begin(LOADCELL_DOUT_PIN_1, LOADCELL_SCK_PIN_1);
  scale2.begin(LOADCELL_DOUT_PIN_2, LOADCELL_SCK_PIN_2);

  // Calibration and taring
  scale1.set_scale(421059.953); // Set the calibration factor for scale1
  scale2.set_scale(348846.63);  // Set the calibration factor for scale2

  lcd.setCursor(5, 0);
  lcd.print("start");
  delay(3000);
  lcd.clear();

  // Load the previous week's values from EEPROM
  for (int i = 0; i < 7; i++) {
    EEPROM.get(i * sizeof(float), day[i]);
    Serial.print(day[i]);
  }
  previousSum = day[0];
  EEPROM.get(100,avgusg);

  
   // Initialize previous sum to the most recent day value
}

void loop() {
  int daysRemaining=0;
  readSensor();

  unsigned long currentMillis = millis();
  float sum = 0;

  // Read values from the load cells
  float w1 = scale1.get_units(5);
  float w2 = scale2.get_units(5);
  float weight1 = ((w1 - 0.94) * 3.98);
  float weight2 = ((w2 - 1.37) * 3.44);
  sum = weight1 + weight2;

  // Print readings to Serial Monitor
  Serial.print("One reading:\t");
  Serial.print(weight1, 2);
  Serial.print("\t|Second reading:\t");
  Serial.print(weight2, 2);
  Serial.print("\t|Sum:\t");
  Serial.print(sum, 2);
  Serial.println();
  //Serial.println(daysRemaining);

  // Check if a new gas tank is replaced or the type has changed
  if (sum > previousSum + 2.0 || sum < previousSum - 2.0) { 
    Serial.println("Gas tank replaced or changed type, resetting daily usage.");

    // Reset today's usage to avoid skewing the average
    day[0] = 0 ;
    previousSum = sum;

    // Update EEPROM with the reset value
    EEPROM.put(0, day[0]);
  } else if (currentMillis - previousMillis >= dayInterval) {
    // 24 hours have passed, so record daily usage
    previousMillis = currentMillis; // Reset the timer

    // Calculate daily usage
    float dailyUsage = previousSum - sum;
    if (dailyUsage < 0) dailyUsage = 0; // Handle potential negative usage (if any error occurs)

    // Shift days to the right
    for (int i = 6; i > 0; i--) {
      day[i] = day[i - 1];
    }
    day[0] = dailyUsage; // Store today's usage

    // Store the new day values in EEPROM
    for (int i = 0; i < 7; i++) {
      EEPROM.put(i * sizeof(float), day[i]);
    }
   

    // Update the previous sum for the next calculation
    previousSum = sum;

    // Calculate the average daily usage
    float averageUsage = 0;
    for (int i = 0; i < 7; i++) {
      averageUsage += day[i];
    }
    averageUsage /= 7; 
    avgusg = averageUsage;
    EEPROM.put(100,avgusg);
    

    // Display the average daily usage on Serial Monitor
    Serial.print("Average Daily Usage: ");
    Serial.println(averageUsage, 2);

    // Predict days remaining based on current sum and average usage
    
  }
  
  if (avgusg > 0) {
      if (sum >= 12.9 && sum <= 25.4) {
        daysRemaining = (sum-12.9)/ avgusg;
      }
      else if (sum >= 4.6 && sum <= 6.9) {
        daysRemaining = (sum-4.6)/ avgusg;
      }
      else {
        daysRemaining = 0;
      }
      //EEPROM.put(100,daysRemaining);
      Serial.print("Predicted Days Remaining: ");
      Serial.println(daysRemaining);
    }
    
  String percentage;
  if (sum >= 12.9 && sum <= 25.4) {
    percentage = String((25 - sum) * 100 / 12.5, 0); 
    lcd.setCursor(5, 0);
    lcd.print(percentage);
    lcd.setCursor(7, 0);
    lcd.print("%");
  } else if (sum >= 4.6 && sum <= 6.9) {
    percentage = String((6.9 - sum) * 100 / 2.3, 0);
    lcd.setCursor(5, 0);
    lcd.print(percentage);
    lcd.setCursor(7, 0);
    lcd.print("%");
  } else {
    percentage = "No gas tank";
    lcd.setCursor(3, 0);
    lcd.print(percentage);
  }

  // Display readings on the LCD
  lcd.setCursor(2, 1);
  lcd.print(daysRemaining);
  lcd.setCursor(5, 1);
  lcd.print("Days more");

  delay(3000);
  lcd.clear();
}

int readSensor() {
  unsigned int sensorValue = analogRead(sensorPin);  // Read the analog value from the sensor
  unsigned int outputValue = map(sensorValue, 0, 1023, 0, 255); // Map the 10-bit data to 8-bit data

  if (outputValue > 65)
    analogWrite(ledPin, outputValue); // Generate PWM signal
  else
    digitalWrite(ledPin, LOW);

  return outputValue; // Return analog moisture value
}
