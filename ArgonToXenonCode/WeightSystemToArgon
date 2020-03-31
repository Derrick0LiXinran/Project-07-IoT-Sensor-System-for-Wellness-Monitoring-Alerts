//This Code is intended to be added into the Xenon in order to Track Connection
//It is a small snipped which it will ask for a connection check every 1min
//The 1 min check is purely for Final Demo purposes

//The only purpose is to Acknowledge the fact that you are awake
int IAmAwake = 0;
int BatteryCheck = 0;
void SignalAwakeAlpha(const char *event, const char *data)
{

//Set a flag and send a signal that you are awake
IAmAwake = 1;
}
void SignalBattery(const char *event, const char *data)
{

//Set a flag and send a signal that you are awake
BatteryCheck = 1;
}

void setup() {

 Particle.subscribe("SignalAwakeAlpha",SignalAwakeAlpha);
 Particle.subscribe("SignalBattery",SignalBattery);
 Serial.begin(9600);
 
 
}

void loop() {
    
    //Put this loop in your program 
    //The reasoning behind the loop is that it will send a signal that you are wake
    //So that argon doensnt send a problem 
    if(IAmAwake)
    {
        Serial.println("Sending Signal I am Awake"); 
        Particle.publish("IamAwakeAlpha", "F"); 
        IAmAwake = 0;
        
    }
    if(BatteryCheck)
    {
        Serial.println("Sending battery level to app"); 
        BatteryCheck = 0;
    }

}
