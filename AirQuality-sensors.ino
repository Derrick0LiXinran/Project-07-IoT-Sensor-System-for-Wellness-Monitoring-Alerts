/*
 * Project Air Quality Wing Library
 */

#include "AirQualityWing.h"
#include "board.h"

#define DB_80      600



AirQualityWingData_t data_AirQ;
// Counter
int i = 0;
int counter = 0;
// Battery volt
float voltage;
//Sound
int soundlevel = A0;
int soundgate  = A2;
bool sound_detected;
int sound_dB;

// time variables
unsigned long soundwait, currentTime, bathroomTime, oldBathroomTime, bathroomStart, bathroomEnd;
unsigned long previousTime = 0;
unsigned long oldsoundwait = 0;
const long interval = 60 * 1000 * 1; // Delay Amount:  2 mins
const long sound_interval = 1500;    // Delay 1.5 sounds
const long Mins5    = 60 * 1000 *1; // Delay Amount:  5 mins
//Sleep status
int sleep_check = 1;

// Battery volt Json 
String bat_toString(float volt);
void BATT_check();
// Air Quality Data
AirQualityWingData_t Air_data;

// Logger
SerialLogHandler logHandler(115200, LOG_LEVEL_ERROR, {
    { "app", LOG_LEVEL_WARN }, // enable all app messages
});

// Forward declaration of event handler
void AirQualityWingEvent();

// AirQualityWing object
AirQualityWing AirQual = AirQualityWing();

// Handler is called in main loop.
// Ok to run Particle.Publish
void AirQualityWingEvent() {

  Log.trace("pub");

  // Publish event
  Particle.publish("blob", AirQual.toString(), PRIVATE, WITH_ACK);
  data_AirQ = AirQual.getData();

}

// Cloud function for setting interval
int set_interval( String period ) {

  // Set the interval with the air quality code
  AirQual.setInterval((uint32_t)period.toInt());

  return -1;

}





// setup() runs once, when the device is first turned on.
void setup() {
    bathroomTime = 0;
    
    // Time Zone Setup
    // Time.zone(-4);
    // Serial.print(Time.local()); // 1400647897
    BATT_check();
    // Sleep status check
    sleep_check = 1;
    Particle.publish("SleepRuquest", "1");
    Particle.subscribe("SleepStatus", sleepHandler);
    
    
    
   //Turn off the LED
//   RGB.control(true);
//   RGB.brightness(0);
  pinMode(soundgate,INPUT);  //  pin is input (reading the photoresistor)
  // Set up PC based UART (for debugging)
  Serial.blockOnOverrun(false);
  Serial.begin();

  // Set up I2C
  Wire.setSpeed(I2C_CLK_SPEED);
  Wire.begin();

  // Default settings
  AirQualityWingSettings_t defaultSettings =
  { 100000, //Measurement Interval
    false,                 //Has HPMA115
    true,                 //Has CCS811
    true,                 //Has Si7021
    CCS811_ADDRESS,       //CCS811 address
    CCS811_INT_PIN,       //CCS811 intpin
    CCS811_RST_PIN,       //CCS811 rst pin
    CCS811_WAKE_PIN,      //CCS811 wake pin
    HPMA1150_EN_PIN       //HPMA int pin
  };

  // Setup & Begin Air Quality
  AirQual.setup(AirQualityWingEvent, defaultSettings);
  AirQual.begin();

  // Set up cloud variable
  Particle.function("set_interval", set_interval);

  // Set up keep alive
  Particle.keepAlive(60);

  // Startup message
  Serial.println("Air Quality Wing for Particle Mesh");
  
//   Particle.subscribe("NewcO2",PublishCO2,ALL_DEVICES);


}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  //Sound

    if(sleep_check == true){

      if (digitalRead(soundgate) == true){
          
          
          
          
      
        soundwait = millis();
        if(soundwait - oldsoundwait >= sound_interval){
            oldsoundwait = soundwait;
            sound_dB = analogRead(soundlevel);
            Particle.publish("SoundLevel", String(sound_dB));
            Particle.publish("BathroomDuration", String (25), PRIVATE, WITH_ACK);
            
            if (sound_dB > DB_80){
                Particle.publish("\"VeryLoud\"alart", "OVER 80 dB");
            }
            bathroomTime = millis();
            oldBathroomTime = bathroomTime;
            if (counter == 0){
                bathroomStart = bathroomTime;
                counter++;
            }
            bathroomEnd = bathroomTime;
          
        }
      }
      if (bathroomTime != 0){
          
            bathroomTime = millis();
      }
      if (bathroomTime - oldBathroomTime >= Mins5 && bathroomTime != 0){
            
            bathroomTime = 0;
            Particle.publish("BathroomDuration", String ((bathroomEnd - bathroomStart) / 1000), PRIVATE, WITH_ACK);
            //   Particle.publish("Bathroom Duration", String (Time.minute(millis())));
            counter = 0;
            
        }
    }
  //Air Quality 
  uint32_t err_code = AirQual.process();
  if( err_code != success ) {

    switch(err_code) {
      case si7021_error:
         Particle.publish("err", "si7021" , PRIVATE, NO_ACK);
          Log.error("Error si7021");
      case ccs811_error:
         Particle.publish("err", "ccs811" , PRIVATE, NO_ACK);
          Log.error("Error ccs811");
      case hpma115_error:
         Particle.publish("err", "hpma115" , PRIVATE, NO_ACK);
          Log.error("Error hpma115");
      default:
        break;
    }
    
  }
  currentTime = millis();     //Current time in seconds
  //Daytime check
  if(sleep_check){
    //  night time 
      if ( currentTime - previousTime >= interval && bathroomTime == 0)
        {
            System.sleep(soundgate, RISING, 1800);  //gate pin wakeup or 30 mins wake up
            Particle.publish("SleepRuquest", "30 MINs");
            counter = 0;
            previousTime = currentTime;
            BATT_check();
        }
  }else{
    //   Day time
      if ( currentTime - previousTime >= interval)
        {
            System.sleep(A1, RISING, 3600);  //gate pin wakeup or 60 mins wake up
            Particle.publish("SleepRuquest", "60 MINs");
            previousTime = currentTime;
            BATT_check();
        }
  }
    
}


String bat_toString(float volt) {

  String out = "{";

  // If we have CCS811 data, concat
  
  out = String( out + String::format("\"BatteryVolt\":%lf", volt) );
  


  return String( out + "}" );

}

void sleepHandler(const char *event, const char *data)
{
  /* Particle.subscribe handlers are void functions, which means they don't return anything.
  They take two variables-- the name of your event, and any data that goes along with your event.
  In this case, the event will be "buddy_unique_event_name" and the data will be "intact" or "broken"

  Since the input here is a char, we can't do
     data=="intact"
    or
     data=="broken"

  chars just don't play that way. Instead we're going to strcmp(), which compares two chars.
  If they are the same, strcmp will return 0.
  */

  if (strcmp(data,"1")==0) {
    // Client is sleeping
    sleep_check = 1;
  }
  else if (strcmp(data,"0")==0) {
    // Daytime
    sleep_check = 0;
  }
  else {
    // if the data is something else, don't do anything.
    // Really the data shouldn't be anything but those two listed above.
  }
}
void BATT_check(){
        //Battery Status Feedback
    voltage = analogRead(BATT) * 0.0011224;
    if(voltage > 4.15){
        //Battery fully Charged
        Particle.publish("LowBatteryFlagBetha", "F");
        
    
    }else if (voltage > 3.6 && voltage <= 4.15){
        // Battery is good or charging
        Particle.publish("LowBatteryFlagBetha", "G");
        
    }else{
        // Battery is low and need to be charge
        Particle.publish("LowBatteryFlagBetha", "L");
        
    }
}






