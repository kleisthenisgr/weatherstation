#include <Arduino.h>
#include "Arduino_SensorKit.h"

#define Environment Environment_I2C 
#define BUZZER 5
#define light A3
#define button 4    

int sound_sensor = A2; 
const int ledPin = 6; 

const int minRawValue = 135; 
const int maxRawValue = 500; 
const double minDB = 18.5; 
const double maxDB = 82.0; 

unsigned long previousMillis = 0; 
const long interval = 50; 
double maxDBValue = 0; 
double maxDBperm = 0; 

int button_state = 0;         
int btn_cnt = 0;

void setup(void) {
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT); 
  Wire.begin(); 

  pinMode(BUZZER, OUTPUT); 

  Pressure.begin(); 

  pinMode(button, INPUT);

  Oled.begin();
  Oled.setFlipMode(true); 

  Environment.begin();
}

void loop(void) {
    unsigned long currentMillis = millis(); 

    static unsigned long lastButtonCheck = 0;
    const long buttonCheckInterval = 15; 
    if (currentMillis - lastButtonCheck >= buttonCheckInterval) {
        int new_button_state = digitalRead(button);
        static int lastButtonState = LOW;
        if (new_button_state == HIGH && lastButtonState == LOW) {
            if (btn_cnt < 2) {
                btn_cnt++;
            } else {
                btn_cnt = 0;
            }
            Serial.print("Button counter: ");
            Serial.println(btn_cnt);
        }
        lastButtonState = new_button_state; 
        lastButtonCheck = currentMillis; 
    }

    int soundValue = 0;
    for (int i = 0; i < 32; i++) {
        soundValue += analogRead(sound_sensor);
    }
    soundValue >>= 5;

    double dB = mapToDB(soundValue);

    if (dB > maxDBValue) {
        maxDBValue = dB;
    }

    static unsigned long previousMillis = 0;
    const long interval = 500; 
    if (currentMillis - previousMillis >= interval) {
        Serial.print("Maximum Sound Level in the last second: ");
        Serial.print(maxDBValue);
        Serial.println(" dB");
        maxDBperm = maxDBValue;
        maxDBValue = 0;
        previousMillis = currentMillis;
    }

    float temp = Environment.readTemperature();
    float humid = Environment.readHumidity();
    float fahrenheit = temp * 1.8 + 32;

    static bool buzzerOn = false;
    static unsigned long buzzerMillis = 0;
    const long buzzerOnDuration = 2000; 
    const long buzzerCycleDuration = 60000; 
    if (temp >= 25.0) {
        if (!buzzerOn && (currentMillis - buzzerMillis >= buzzerCycleDuration)) {
            tone(BUZZER, 85); 
            buzzerOn = true;
            buzzerMillis = currentMillis; 
        } else if (buzzerOn && (currentMillis - buzzerMillis >= buzzerOnDuration)) {
            noTone(BUZZER); 
            buzzerOn = false;
        }
    } else {
        noTone(BUZZER); 
        buzzerOn = false;
        buzzerMillis = currentMillis; 
    }

    static unsigned long ledStartMillis = 0;
    static bool ledActive = false;
    if (maxDBperm > 50 && !ledActive) {
        digitalWrite(ledPin, HIGH); 
        ledStartMillis = currentMillis;
        ledActive = true;
    }
    if (ledActive && (currentMillis - ledStartMillis >= 4000)) {
        digitalWrite(ledPin, LOW); 
        ledActive = false;
    }

    static unsigned long lastScreenUpdate = 0;
    const long screenUpdateInterval = 500; 
    if (currentMillis - lastScreenUpdate >= screenUpdateInterval) {
        int analogValue = analogRead(light);
        String lstatus = "";
        if (analogValue < 10) {
            lstatus = "Dark";
        } else if (analogValue < 200) {
            lstatus = "Dim";
        } else if (analogValue < 500) {
            lstatus = "Light";
        } else if (analogValue < 800) {
            lstatus = "Bright";
        } else {
            lstatus = "Extreme";
        }

        float hPa = Pressure.readPressure() / 100;
        float meters = Pressure.readAltitude();
        float feet = meters * 3.2808399;
        float psi = hPa * 0.01450377;

        Oled.setFont(u8x8_font_amstrad_cpc_extended_f);

        if (btn_cnt == 0 || btn_cnt == 1) {
            Oled.setCursor(0, 0);
            Oled.print("Temp: ");
            if (btn_cnt == 0) {
                Oled.print(temp);
                Oled.print(" C   "); 
            } else {
                Oled.print(fahrenheit);
                Oled.print(" F   ");
            }

            Oled.setCursor(0, 1);
            Oled.print("Humid: ");
            Oled.print(humid);
            Oled.print(" %  ");

            Oled.setCursor(0, 2);
            Oled.print("Noise: ");
            Oled.print(maxDBperm);
            Oled.print(" dB");

            Oled.setCursor(0, 3);
            Oled.print("Air: ");
            if (btn_cnt == 0) {
                Oled.print(hPa);
                Oled.print(" hPa ");
            } else {
                Oled.print(psi);
                Oled.print(" Psi  ");
            }

            Oled.setCursor(0, 4);
            Oled.print("Alt: ");
            if (btn_cnt == 0) {
                Oled.print(meters);
                Oled.print(" m   ");
            } else {
                Oled.print(feet);
                Oled.print(" ft  ");
            }

            Oled.setCursor(0, 5);
            Oled.print("Light: ");
            Oled.print(lstatus);
            Oled.print("   ");
        } else {
            Oled.setCursor(0, 0);
            Oled.print("Weather Station ");
            Oled.setCursor(0, 1);
            Oled.print("----------------");
            Oled.setCursor(0, 2);
            Oled.print("Koilias Alexios");
            Oled.setCursor(0, 3);
            Oled.print("Tsiamalos Kostas");
            Oled.setCursor(0, 4);
            Oled.print("Mental Wealth 2");
            Oled.setCursor(0, 5);
            Oled.print("2024-2025     ");
        }

        Oled.setCursor(0, 6);
        Oled.print("Data page: ");
        Oled.print(btn_cnt + 1);
        Oled.print("/3");

        lastScreenUpdate = currentMillis; 
    }
}

double mapToDB(int rawValue) {
    if (rawValue < minRawValue) {
        return minDB + (maxDB - minDB) * (rawValue - minRawValue) / (maxRawValue - minRawValue); 
    } else if (rawValue > maxRawValue) {
        return maxDB; 
    } else {
        return minDB + (maxDB - minDB) * (rawValue - minRawValue) / (maxRawValue - minRawValue);
    }
}
