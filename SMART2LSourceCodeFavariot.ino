/*
 * Author Haikal.Hafiz 
 * Since 1.0.2
 */
#include <Process.h>
#define SERIES_RESISTOR 560
#define SENSOR_PIN A0 
#define ZERO_VOLUME_RESISTANCE    1600.00    // Resistance value (in ohms) when no liquid is present.
#define CALIBRATION_RESISTANCE    320.00    // Resistance value (in ohms) when liquid is at max line.
#define CALIBRATION_VOLUME        500.00    // Volume (in any units) when liquid is at max line.
int relay = 7; //This digital pin for relay control (waterpump)
byte statusLedForFlowRate = 13; //For led in flowrate function
byte sensorPin = 3; //Ditigal pin for flowrate sensor 
byte sensorInterrupt = 0; // Interrupt number (INT0)
float calibrationFactorFlowRate = 4.5; //calibration factor (4.5 is one cycle in flowrate)
volatile byte pulseCount;  
float flowRate;
unsigned int flowMilliLitres; //Flowrate function
unsigned long totalMilliLitres;//Flowrate function
unsigned long oldTime;//Flowrate Function 
/*****************************************************************************************************************************************************************/
void setup() {
 pinMode(relay,OUTPUT); //This digital pin for relay control (waterpump)
 pinMode(2,INPUT); // This digital pin for leaking dection 
 Bridge.begin();
 Serial.begin(115200);
 pinMode(statusLedForFlowRate,OUTPUT);
 digitalWrite(statusLedForFlowRate,HIGH);
 pinMode(sensorPin,INPUT);
 digitalWrite(sensorPin,HIGH);
 // The Hall-effect sensor is connected to pin 3 which uses interrupt 0.
 // Configured to trigger on a FALLING state change (transition from HIGH
 // state to LOW state)
 attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
 pulseCount        = 0;
 flowRate          = 0.0;
 flowMilliLitres   = 0;
 totalMilliLitres  = 0;
 oldTime           = 0;

 Serial.println(F("Loading...")); 
 delay(2000);
 Serial.println(F("Start the system"));
 delay(2000);
}


/************************************************************************************************************************************************************/
void loop() {

float volume = etape();
notification(volume); 
LeakDetection();
FLOWRATE();
}


/*********************************************************************************************************************************************************/

/* In this function it will read the analog input from pin A0 (etape) 
 *  and also read the resistance input from A0
 *  and calculate the calibration bewteen  ZERO_VOLUME_RESISTANCE    1600.00 and define CALIBRATION_RESISTANCE    320.00  
 *  the result of calculation will store in virable name volume and the volume will send to firebase.
 *  It will also check the water level(volume) if below threshold it will trigger the pump (using pin 7) to active 
 *  when water is active it will send the status of the pump to firebase.
 *  when water above the threshold it will send status of the water level which is "water level is normal" to fire base.
 */
void notification (float volume){
  Serial.print(F("Print from notification function" ));
  Serial.print(volume );
  
 }
float etape(){
 float resistance ,volume;
 resistance = analogRead(0);
 resistance = (1023.0 / resistance) - 1.0;
 resistance =SERIES_RESISTOR / resistance;
 Serial.print(F("Resistance:"));
 Serial.print(resistance,2);
 Serial.println(F("ohm"));
 volume = resistanceToVolume(resistance, ZERO_VOLUME_RESISTANCE, CALIBRATION_RESISTANCE, CALIBRATION_VOLUME);
 Serial.print(F("Calculated liquid volume: "));
 Serial.println(volume, 5);
 Serial.println(F(""));
 delay(500);
  Process p; 
  Process Fav;
  Fav.runShellCommand("curl -X POST -H \"Content-Type:application/json\" -H \"Accept:application/json\" -H \"apiKey:eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VybmFtZSI6ImhhaWthbCIsInJlYWRfd3JpdGUiOnRydWUsImlhdCI6MTUwODgyOTc2Mn0.f2qzj74vZaxcoHz-0bw3w3cGa8-CV5GrI0jg_r1nDzg\" -d {\"device_developer_id\":\"ArduinoYun@haikal\",\"data\":{\"WaterLevel\":"+String(volume)+"}} -g \"http://api.favoriot.com/v1/streams");
  Fav.runShellCommand("curl -k -X PUT -d '{ \"Water Level\" : "+String(volume)+" }' \'https://test1-d8e27.firebaseio.com/Water Level.json'");
   while(p.running());
   if (volume < 200.00){
    Process p8;
    p8.runShellCommand(F("cat /root/waterlevel | ssmtp **INSERT YOUR EMAIL HERE***"));
    Serial.println(F("send notification to email"));
    digitalWrite(relay,HIGH);
    Serial.println(F("Water pump is active"));  
    delay(2000);
    int water1= 1;
    Process p2;
    p2.runShellCommandAsynchronously("curl -k -X PUT -d '{ \"Status\": \"Water pump is active\", \"Water pump\" : "+String(water1)+" }' \'https://test1-d8e27.firebaseio.com/WaterPump.json'");
    while(p2.running()); 
    Serial.println(F("Water has been pumped out"));
    delay(200); 
    digitalWrite(relay,LOW); 
                 
  }
  else {
    Serial.println(F("water level is normal")); 
    int water2=2;
    Process p3;
    p3.runShellCommandAsynchronously("curl -k -X PUT -d '{ \"Status\": \"Water pump is not active\", \"Water level is normal\" : "+String(water2)+" }' \'https://test1-d8e27.firebaseio.com/WaterPump.json'");
    while(p.running()); 
    delay(200); 
 return volume;
  
}
}
float resistanceToVolume(float resistance, float zeroResistance, float calResistance, float calVolume) {
  if (resistance > zeroResistance || (zeroResistance - calResistance) == 0.0) {
    // Stop if the value is above the zero threshold, or no max resistance is set (would be divide by zero).
    return 0.0;
  }
  // Compute scale factor by mapping resistance to 0...1.0+ range relative to maxResistance value.
  float scale = (zeroResistance - resistance) / (zeroResistance - calResistance);
  // Scale maxVolume based on computed scale factor.
  return calVolume * scale;
}
/**********************************************************************************************************************************/


/**********************************************************************************************************************************/


void LeakDetection(){
   /*In this function Water Sensor Module will detect presence of any Liquid
   * It used A2 pin for Analog reading
   * It used Pin 4 for digital Input
   * All virables are decleared in local except pinMode(4) in setup
   * Current reading of the water sensor will send to Firebase
   */
  int leaking = A2;
  int leakingDitigalIn = 2;
  int leakingVal;
  boolean Isleaking = false ;
  String strLeaking;
  leakingVal =analogRead(leaking);
  Isleaking =!(digitalRead(leakingDitigalIn));

  if (Isleaking){
    strLeaking = "Leakage Detected";
    Serial.println(strLeaking);
    //random number to send data to firebase
    int rawak=random(0,4);
    Process p4;Process p9; Process LeakageFavariot;
     LeakageFavariot.runShellCommand("curl -X POST -H \"Content-Type:application/json\" -H \"Accept:application/json\" -H \"apiKey:eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VybmFtZSI6ImhhaWthbCIsInJlYWRfd3JpdGUiOnRydWUsImlhdCI6MTUwODgyOTc2Mn0.f2qzj74vZaxcoHz-0bw3w3cGa8-CV5GrI0jg_r1nDzg\" -d {\"device_developer_id\":\"ArduinoYun@haikal\",\"data\":{\"WaterLeakage\":"+String(rawak)+"}} -g \"http://api.favoriot.com/v1/streams");
     p9.runShellCommand(F("cat /root/leakage | ssmtp **INSERT YOUR EMAIL HERE***"));
     p4.runShellCommandAsynchronously("curl -k -X PUT -d '{ \"Status\": \"Leakage Detected\",\"Leakage\" : "+String(rawak)+" }' \'https://test1-d8e27.firebaseio.com/Leaking.json'"); 
    while(p4.running()); 
    delay(10000); 
  }
    else{
    strLeaking = "No Leakage";
    Serial.println(strLeaking);
    //random number to send data to firebase
    int haikal=random(5,10);
    Process p5;
     p5.runShellCommandAsynchronously("curl -k -X PUT -d '{  \"Status\": \"No Leakage Detected\",\"Leakage\" : "+String(haikal)+" }' \'https://test1-d8e27.firebaseio.com/Leaking.json'"); 
    while(p5.running()); 
    delay(10000); 
    }
  delay(500);

}
/****************************************************************************************************************************************************************************/


/****************************************************************************************************************************************************************************/

void FLOWRATE(){
  /*In this function it will calculate the output of the liqud has passed trough flow rate sensor and 
   * it reading will send to Firebase and it also notifiy user if flowrate drop
   */
    if((millis() - oldTime) > 1000)    // Only process counters once per second
  { 
    // Disable the interrupt while calculating flow rate and sending the value to
    // the host
    detachInterrupt(sensorInterrupt);
        
     /*Because this loop may not complete in exactly 1 second intervals we calculate
     the number of milliseconds that have passed since the last execution and use
     that to scale the output. it also apply the calibrationFactor to scale the output
     based on the number of pulses per second per units of measure (litres/minute in
     this case) coming from the sensor.
     */

     
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactorFlowRate;
    
    // Note the time this processing pass was executed.
    // disabled interrupts the millis() function won't actually be incrementing right
    // at this point, but it will still return the value it was set to just before
    // interrupts went away.
    oldTime = millis();
    
    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;
    
    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;
      
    unsigned int frac;
    
    // Print the flow rate for this second in litres / minute
    Serial.print("Flow rate: ");
    Serial.print(int(flowRate));  // Print the integer part of the variable
    Serial.print(".");             // Print the decimal point
    // Determine the fractional part. The 10 multiplier gives us 1 decimal place.
    frac = (flowRate - int(flowRate)) * 10;
    Serial.print(frac, DEC) ;      // Print the fractional part of the variable
    Serial.print("L/min");
    // Print the number of litres flowed in this second
    Serial.print("  Current Liquid Flowing: ");             // Output separator
    Serial.print(flowMilliLitres);
    Serial.print("mL/Sec");

    // Print the cumulative total of litres flowed since starting
    Serial.print("  Output Liquid Quantity: ");             // Output separator
    Serial.print(totalMilliLitres);
    Serial.println(F("mL")); 

    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;
    
    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  }

  Process p6; Process flow;
    flow.runShellCommand("curl -X POST -H \"Content-Type:application/json\" -H \"Accept:application/json\" -H \"apiKey:eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VybmFtZSI6ImhhaWthbCIsInJlYWRfd3JpdGUiOnRydWUsImlhdCI6MTUwODgyOTc2Mn0.f2qzj74vZaxcoHz-0bw3w3cGa8-CV5GrI0jg_r1nDzg\" -d {\"device_developer_id\":\"ArduinoYun@haikal\",\"data\":{\"FlowRate\":"+String(flowMilliLitres)+"}} -g \"http://api.favoriot.com/v1/streams");
    p6.runShellCommandAsynchronously("curl -k -X PUT -d '{  \"Current Flow Rate\" : "+String(flowMilliLitres)+" }' \'https://test1-d8e27.firebaseio.com/FlowRate.json'"); 
    while(p6.running());
    Serial.println(F("Current Flow Rate are send to fireBase")); 
    delay(500);
    Process p7; Process totalFlow;
    totalFlow.runShellCommand("curl -X POST -H \"Content-Type:application/json\" -H \"Accept:application/json\" -H \"apiKey:eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VybmFtZSI6ImhhaWthbCIsInJlYWRfd3JpdGUiOnRydWUsImlhdCI6MTUwODgyOTc2Mn0.f2qzj74vZaxcoHz-0bw3w3cGa8-CV5GrI0jg_r1nDzg\" -d {\"device_developer_id\":\"ArduinoYun@haikal\",\"data\":{\"TotalFlow\":"+String(totalMilliLitres)+"}} -g \"http://api.favoriot.com/v1/streams");
    p7.runShellCommandAsynchronously("curl -k -X PUT -d '{ \"Output Liquid Quantity\" : "+String(totalMilliLitres)+" }' \'https://test1-d8e27.firebaseio.com/OutputLiquid.json'"); 
    while(p7.running());
    Serial.println(F("Output liquid quantity are send to fireBase")); 
    delay(500);

}

void pulseCounter(){
  // Increment the pulse counter
  pulseCount++;
}
/*************************************************************************************************************************************/