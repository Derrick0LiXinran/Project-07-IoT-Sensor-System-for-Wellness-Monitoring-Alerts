//This code is intended for The alpha System which goes with the LoadCell
//This code will be in the Argon for the alpha system
//The code only send a alert that you are awake every time you recieve the ask from the argon

int StartConnectionCountDown,StartConnectionCheck,StartBatteryCheck;
int EndConnectionCountDown,EndConnectionCheck,EndBatteryCheck;
int AlphaAwake,AlphaProblem,Batterycheck;
void IamAwakeAlpha(const char *event, const char *data)
{

EndConnectionCountDown = 99999999;//Sets to infinity until called again
AlphaAwake = 1;// Sets the flag to the APP
AlphaProblem = 0;

}

void Scheduler()
{
    
   //Call the battery level every 30Sec
   //Call the Check connection every 120sec
StartConnectionCountDown++;
StartConnectionCheck++;
StartBatteryCheck++;
    
}

Timer Schedule(1000,Scheduler);

void setup() {
    
    //Set up for the Clocks
    EndConnectionCountDown = 999999999;// to be set by Connection Check
    EndConnectionCheck = 60*1; // Every min
    EndBatteryCheck = 60*5; //5 min check
    Serial.begin(9600);
    Schedule.start(); 
    AlphaAwake = 0;
    AlphaProblem = 0;
    Particle.subscribe("IamAwakeAlpha",IamAwakeAlpha);
}

void loop() {

//The if statements works as Threads but a bit simpler since we dont need something big
//The demo for the Alpha will run in this threads
//This will trigger functions in the Xenon which will respond to the function
if(StartConnectionCountDown >= EndConnectionCountDown)
{
    Serial.println("Connection failature"); 
    AlphaProblem = 1;
    StartConnectionCountDown = 0;
}

if(StartConnectionCheck >= EndConnectionCheck)
{
    Serial.println("start Connection Check"); 
    Particle.publish("SignalAwakeAlpha", "F"); 
    EndConnectionCountDown = 15; // 15sec limit to reconnect 
    StartConnectionCountDown = 0;
    StartConnectionCheck = 0;
}

if(StartBatteryCheck >= EndBatteryCheck)
{
    Batterycheck = 1;
    StartBatteryCheck = 0;
}

if(AlphaAwake)
{
    Serial.println("Alpha is awake"); 
    //Send the Battery Flag
    if(Batterycheck)
    {
    Serial.println("Send Battery Level"); 
    Particle.publish("SignalBattery", "F");     
    Batterycheck = 0;
    }
    
    AlphaAwake = 0;
}

if(AlphaProblem)
{
    Serial.println("Failature to connect");
    Serial.println("There is a problem"); 
    AlphaProblem = 0;
    
}






}
