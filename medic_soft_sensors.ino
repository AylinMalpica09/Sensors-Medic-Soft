
//ds18b20
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>

int oneWireBus = 2;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

//ECG
int hearthBeat = 0;

//MAX30102

#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
MAX30105 particleSensor;
#define MAX_BRIGHTNESS 255

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
uint16_t irBuffer[100]; //infrared LED sensor data
uint16_t redBuffer[100];  //red LED sensor data
#else
uint32_t irBuffer[100]; //infrared LED sensor data
uint32_t redBuffer[100];  //red LED sensor data
#endif
int32_t bufferLength; //data length
int32_t spo2; //SPO2 value
int8_t validSPO2; //indicator to show if the SPO2 calculation is valid
int32_t heartRate; //heart rate value
int8_t validHeartRate; //indicator to show if the heart rate calculation is valid
byte pulseLED = 21; //Must be on PWM pin
byte readLED = 22; //Blinks with each data read


void setup() {
  
  //basic config
  Serial.begin(115200);
  
  //ds18b20
  sensors.begin();
  boolean validTemp = false;
  float internalTemp;
  while(validTemp == false){
    sensors.requestTemperatures(); 
    internalTemp = sensors.getTempCByIndex(0);
    Serial.print(".");
    if(internalTemp >36){
      Serial.print("Temperatura: ");
      Serial.println(internalTemp);
      validTemp= true;
   }
   
  }
 
  //ECG
  pinMode(32, INPUT); // Setup for leads off detection LO +
  pinMode(35, INPUT); // Setup for leads off detection LO -
  double suma =0;
  int i=0;
  while(i<10){
    
    if((digitalRead(32) == 1)||(digitalRead(35) == 1)){
      Serial.println('!');
    }
    else{
        
      hearthBeat = ((analogRead(25)*100)/3000);
      if(hearthBeat > 40 && hearthBeat <120){
        suma = suma + hearthBeat;
        i = i+1;
        delay(2000);
        
      }
    }
  }

  int hearthBeatValid = round(suma/10);
  Serial.println("Pulso: ");
  Serial.println(hearthBeatValid);

  //MAX30102
  pinMode(pulseLED, OUTPUT);
  pinMode(readLED, OUTPUT);

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println(F("MAX30105 was not found. Please check wiring/power."));
    while (1);
  }
  Serial.read();

  byte ledBrightness = 60; //Options: 0=Off to 255=50mA
  byte sampleAverage = 4; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  byte sampleRate = 100; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 4096; //Options: 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings
  boolean ended = false;
  while(ended==false){
  bufferLength = 100; //buffer length of 100 stores 4 seconds of samples running at 25sps

  //read the first 100 samples, and determine the signal range
  for (byte i = 0 ; i < bufferLength ; i++)
  {
    while (particleSensor.available() == false) //do we have new data?
      particleSensor.check(); //Check the sensor for new data

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample(); //We're finished with this sample so move to next sample
  }
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  //Continuously taking samples from MAX30102.  Heart rate and SpO2 are calculated every 1 second
  while (ended==false)
  {
    //dumping the first 25 sets of samples in the memory and shift the last 75 sets of samples to the top
    for (byte i = 25; i < 100; i++)
    {
      redBuffer[i - 25] = redBuffer[i];
      irBuffer[i - 25] = irBuffer[i];
    }

    //take 25 sets of samples before calculating the heart rate.
    for (byte i = 75; i < 100; i++)
    {
      while (particleSensor.available() == false) //do we have new data?
        particleSensor.check(); //Check the sensor for new data

      digitalWrite(readLED, !digitalRead(readLED)); //Blink onboard LED with every data read

      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample(); //We're finished with this sample so move to next sample
     
      if(spo2 > 80 && validSPO2 ==1){
      //send samples and calculation result to terminal program through UART
      
      ended = true;

      }
    }

    //After gathering 25 new samples recalculate HR and SP02
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
  }
  }
  Serial.print("Temperatura final: ");
  Serial.println(internalTemp);
  Serial.print("Pulso final: ");
  Serial.println(hearthBeatValid);
  Serial.print("Oxigenacion final: ");
  Serial.println(spo2);
}

void loop() {
   

}
