
#include <DallasTemperature.h>
#include <OneWire.h>
#include <Timer.h>

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#define ONE_WIRE_BUS 2 // GPIO2 as Temp sensor input
#define RELAY1 0 // GPIO0 as Relay output
//#define STANDBY_LED 16 //Node MCU
#define STANDBY_LED 1 //ESP01

#define thresholdTemp 33 // temp to turn on the fan
#define checkpointStopTemp 31 // temp to turn off the fan on a checkpoint
//#define fanOnTime 20 * 60 * 1000 // on time from one checkpoint to other
#define standbyTime 5 * 60 * 1000 // mc will start working after standby time
#define breakTime 5 * 60 * 1000 // After all check points if temp is not down below threshold, a short break will initiate to avoid damage to fan
#define checkpointTime 20 * 60 * 1000 // time for checkpoint invokation
#define noOfCheckpoints 6 // On each checkpoints mc checks whether temperature down or not

/* Blynk token */
char auth[] = "bLxFl-gcCBY8M0WIxEc7dNwuFKq4NLSU";

/* WiFi credentials */
char ssid[] = "Tenda";
char pass[] = "Maneesh1234";


bool isReady = false;
bool isInBreak = false;
bool isFanOn = false;
float temperature;

int oscilatorId;
int checkpointTimerId;
int checkpointCounter = noOfCheckpoints;

String tempLabel = String("Temperature is normal:");
Timer timer;

OneWire oneWire(ONE_WIRE_BUS);

DallasTemperature sensors(&oneWire);            // Pass the oneWire reference to Dallas Temperature.

void setup(void)
{
    Serial.begin(9600);
    sensors.begin();
    pinMode(RELAY1, OUTPUT);
    pinMode(STANDBY_LED, OUTPUT);
    digitalWrite(STANDBY_LED, HIGH);
    timer.after(standbyTime,powerOnReset);
    Blynk.begin(auth, ssid, pass);
    timer.every(1000, sendSensorData);
    
}

void loop(void)
{
    sensors.requestTemperatures();
    temperature = sensors.getTempCByIndex(0);
    
    if (temperature > thresholdTemp && !isReady){
        tempLabel = String("Waiting for standby-Temperature crossed threshold:");
    }else if (temperature > thresholdTemp && isInBreak) {
        tempLabel = String("Break Mode-Temperature crossed threshold:");
    }else if (temperature > thresholdTemp){
        tempLabel = String("Temperature crossed threshold:");
    }else{
        tempLabel = String("Temperature is normal:");
    }
    
    Serial.println(tempLabel);
    Serial.println(temperature);
    Blynk.run();
    
    
    
    
    if (isReady && !isInBreak) {
        monitorTempChanges();
    }
    timer.update();
    delay(100);
    
}
void sendSensorData(){
    Blynk.virtualWrite(10, temperature);
    
}
void powerOnReset(void){
    isReady = true;
    digitalWrite(STANDBY_LED, LOW);//On
    Serial.println("Standby");
}

void monitorTempChanges(void){
    
    if (temperature > thresholdTemp && !isInBreak) {
        
        if (!isFanOn) {
            
            Serial.println("Turning on fan");
            
            toggleFanOn(true);
            
        }
        
        
    }
}





void toggleFanOn(bool isOn){
    // Turn on fan for fanOnTime
    if (isOn) {
        
        digitalWrite(RELAY1, HIGH);//On
        isFanOn = true;
        
        checkpointTimerId = timer.every(checkpointTime,checkpoint);
        
    }else{
        digitalWrite(RELAY1, LOW);//Off
        isFanOn = false;
    }
    
}

void checkpoint(){
    Serial.println("Checkpoint: ");
    Serial.print(checkpointCounter);
    Serial.println("");
    if (checkpointCounter == 0 || temperature <= checkpointStopTemp) {
        
        initiateBreak();
        timer.stop(checkpointTimerId);
    }else{
        checkpointCounter--;
    }
    
}

void initiateBreak(){
    isInBreak = true;
    toggleFanOn(false);
    timer.after(breakTime,endBreak);
    oscilatorId = timer.oscillate(STANDBY_LED, 200, LOW);
    
    
}
void endBreak(){
    isInBreak = false;
    
    timer.stop(oscilatorId);
    
    digitalWrite(STANDBY_LED, LOW);
    checkpointCounter = noOfCheckpoints;
}
