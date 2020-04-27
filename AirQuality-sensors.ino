/*
 * Project Air Quality Wing Library
 */

#include "AirQualityWing.h"
#include "board.h"

#define DB_80            600     // 80 dB of sound
#define DAY_TIME         0
#define NIGHT_TIME       1
#define NORMAL_TEMP      23
#define NORMAL_HUMID     50


// Air Quliaty Data
// AirQualityWingData_t Air_data;   // Air Quality Data
AirQualityWingData_t data_AirQ;
int old_temperature = NORMAL_TEMP;
int old_Humidity = NORMAL_HUMID;

// Counter
int counter = 0;    // For detection of bathroom usage

// Battery volt
float voltage;

// Sound
int soundlevel = A0;
int soundgate  = A2;
bool sound_detected;
int sound_dB;

// Time variables
unsigned long soundwait, currentTime;
unsigned long bathroomTime, oldBathroomTime, bathroomStart, bathroomEnd;
unsigned long previousTime = 0;
unsigned long oldsoundwait = 0;

const long interval = 60 * 1000 * 2; // Delay Amount:  2 mins
const long sound_interval = 1500;    // Delay 1.5 sounds
const long Mins5    = 60 * 1000 * 5; // Delay Amount:  5 mins

//Sleep status
int sleep_check = DAY_TIME;

// Funtions
String bat_toString(float volt);  // Battery volt Json
void BATT_check();


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
void AirQualityWingEvent()
{

    Log.trace("pub");

    // Publish event
    data_AirQ = AirQual.getData();

    if ((old_temperature - (int) data_AirQ.si7021.data.temperature) > 5 ||
            (old_temperature - (int) data_AirQ.si7021.data.temperature) < -5 )
    {
        Particle.publish("temperatureSharp",
            "{\"temperatureSharp\":1}", PRIVATE, WITH_ACK);
    }

    if ((old_Humidity - (int) data_AirQ.si7021.data.humidity) > 5 ||
        (old_Humidity - (int) data_AirQ.si7021.data.humidity) < -5 )
    {
        Particle.publish("HumiditySharp",
            "{\"HumiditySharp\":1}", PRIVATE, WITH_ACK);
    }

    old_temperature = (int) data_AirQ.si7021.data.temperature;
    old_Humidity = (int) data_AirQ.si7021.data.humidity;


    Particle.publish("blob", AirQual.toString(), PRIVATE, WITH_ACK);



}

// Cloud function for setting interval
int set_interval( String period )
{
    // Set the interval with the air quality code
    AirQual.setInterval((uint32_t)period.toInt());

    return -1;
}





// setup() runs once, when the device is first turned on.
void setup() {

    bathroomTime = 0;
    sleep_check = DAY_TIME;    //initialization as Daytime

    BATT_check();       //Battery check

    // Sleep status check
    Particle.publish("SleepRuquest", "Setup_Stage");
    Particle.subscribe("SleepStatus", sleepHandler);

    //Turn off the LED
    // RGB.control(true);
    // RGB.brightness(0);
    pinMode(soundgate,INPUT);  //  pin is input (reading the photoresistor)
    // Set up PC based UART (for debugging)
    Serial.blockOnOverrun(false);
    Serial.begin();

    // Set up I2C
    Wire.setSpeed(I2C_CLK_SPEED);
    Wire.begin();

    // Default settings
    AirQualityWingSettings_t defaultSettings =
    {
        100000, //Measurement Interval
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
void loop()
{
    //Sound
    if(sleep_check == NIGHT_TIME)
    {
        if (digitalRead(soundgate) == true)
        {
            soundwait = millis();
            if(soundwait - oldsoundwait >= sound_interval)
            {
                oldsoundwait = soundwait;
                sound_dB = analogRead(soundlevel);
                // Particle.publish("SoundLevel", String(sound_dB));

                if (sound_dB > DB_80)
                {
                    Particle.publish("\"VeryLoud\"alart", "OVER 80 dB");
                }
                bathroomTime = millis();
                oldBathroomTime = bathroomTime;
                if (counter == 0)
                {
                    bathroomStart = bathroomTime;
                    counter++;
                }
                bathroomEnd = bathroomTime;

            }
        }
        if (bathroomTime != 0)
        {
            bathroomTime = millis();
        }
        if (bathroomTime - oldBathroomTime >= Mins5 && bathroomTime != 0)
        {
            Particle.publish("BathroomDuration", "{\"BathroomDuration\":"
                    + String ((bathroomEnd - bathroomStart) / 1000) + "}",
                    PRIVATE, WITH_ACK);
            if ((bathroomEnd - bathroomStart) / 1000 > 900)
            {
                Particle.publish("ToLongAlert", "{\"ToLongAlert\":"
                    + String ((bathroomEnd - bathroomStart) / 1000 / 60)
                    + "}", PRIVATE, WITH_ACK);
            }

            counter = 0;
            bathroomTime = 0;
        }
    }
    //Air Quality
    uint32_t err_code = AirQual.process();
    if( err_code != success )
    {
        switch(err_code)
        {
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
            Particle.publish("SleepRuquest", "30");
            System.sleep(soundgate, RISING, 1800);      //gate pin wakeup or 30 mins wake up
            Particle.publish("SleepRuquest", "WakeUp");     //request the info of person sleep
            counter = 0;
            previousTime = currentTime;
            BATT_check();
        }
    }else
    {
        //   Day time
        if ( currentTime - previousTime >= interval)
        {
            Particle.publish("SleepRuquest", "60");
            System.sleep(A1, RISING, 3600);  //gate pin wakeup or 60 mins wake up
            Particle.publish("SleepRuquest", "WakeUp");     //request the info of person sleep
            previousTime = currentTime;
            BATT_check();
        }
    }

}


String bat_toString(float volt)
{
    String out = "{";
    out = String( out + String::format("\"BatteryVolt\":%lf", volt) );
    return String( out + "}" );
}

void sleepHandler(const char *event, const char *data)
{
    if (strcmp(data,"1")==0)
    {
        // Client is sleeping
        sleep_check = NIGHT_TIME;
    }
    else if (strcmp(data,"0")==0) {
        // Daytime
        sleep_check = DAY_TIME;
    }
    else {
        // if the data is something else, don't do anything.
        // Really the data shouldn't be anything but those two listed above.
    }
}
void BATT_check()
{
    //Battery Status Feedback
    voltage = analogRead(BATT) * 0.0011224;
    if(voltage > 4.15)
    {
        //Battery fully Charged
        Particle.publish("LowBatteryFlagBetha", "{\"LowBatteryFlag\":2}");
    }
    else if (voltage > 3.6 && voltage <= 4.15)
    {
        // Battery is good or charging
        Particle.publish("LowBatteryFlagBetha", "{\"LowBatteryFlag\":1}");
    }
    else
    {
        // Battery is low and need to be charge
        Particle.publish("LowBatteryFlagBetha", "{\"LowBatteryFlag\":0}");
    }
}
