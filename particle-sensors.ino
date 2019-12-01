/*
 * Project Air Quality Wing Library
 */

#include "AirQualityWing.h"
#include "board.h"


// Counter
int i = 0;
//Sound
int soundlevel = A0;
int soundgate  = A2;
bool sound_detected;

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

}

// Cloud function for setting interval
int set_interval( String period ) {

  // Set the interval with the air quality code
  AirQual.setInterval((uint32_t)period.toInt());

  return -1;

}

// setup() runs once, when the device is first turned on.
void setup() {

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
  { MEASUREMENT_DELAY_MS, //Measurement Interval
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

}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  //Sound
//   sound_detected = digitalRead(soundgate);
  if (digitalRead(soundgate) == true){
    // Air_data = AirQual.getData();
    
    // Particle.publish("Sound Level", String(Air_data.si7021.data.temperature), PRIVATE);
    Particle.publish("Sound Level", String(analogRead(soundlevel)), PRIVATE);
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
    
}







