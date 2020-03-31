#include <HX711ADC.h>

// HX711.DOUT	- pin #A1
// HX711.PD_SCK	- pin #A0

HX711ADC scale1(A1, A0, 32);		            
HX711ADC scale2(A2, A0, 32);
HX711ADC scale3(A3, A0, 32);
HX711ADC scale4(A4, A0, 32);

/***************************************************************************************/
/*                        Global Variables                                            */
/*************************************************************************************/

double tare = 0;                                // tare value
int duration = 0;                               // nightly duration
int startingHour, finalHour;                    // Hour
int startingMinute, finalMinute;                // minute
int timeMissing = 0;                            // time missing
int theWeight = 5;
int durationHours, durationMinutes;
int upAndOkay = 0;
int firstTime = 0;
int timesOutOfBed = 0;                          // number of times out of bed overnight

/************************************************************************************/
/***********************************************************************************/

enum timeOfDay {TimeToTare, TimeToWakeUp, DayTime, NightTime, TimeToAlert};
enum bedState {inBed = 1, outOfBed = 0};

enum timeOfDay currentState;
enum bedState inBedState;

void setup() {
 
  pinMode(BATT, INPUT);
  
  // sets D2 as an input
  pinMode(D2, INPUT);
    
  Serial.begin(38400);                          // baud rate
 
  scale1.begin();                               //sets pu A1 and A0
  scale2.begin();
  scale3.begin();
  scale4.begin();

  scale1.set_scale();                           // this value is obtained by calibrating the scale with known weights; see the README for details
  scale2.set_scale();
  scale3.set_scale();
  scale4.set_scale();
  scale1.tare();				                // reset the scale to 0
  scale2.tare();
  scale3.tare();
  scale4.tare();
  
  Time.zone(-5);

}

void loop() {
    
    determineTimeOfDay();
    
    switch(currentState){
        
        // The following state tares the weight of the bed
        case TimeToTare: 
        {
            if(digitalRead(D2)){
                tare = CalculateTare();
            }
            break;
        }
        
        case NightTime: 
        {
            double currentWeight = CalculateWeight();
            firstTime = 0;
            // if already in bed
            if(inBedState == inBed){
                if(checkForMissing(currentWeight)){
                    if(checkHowLongMissing()){
                        inBedState = outOfBed;
                        calculateDuration();
                        String endingTime = "Ending time: " + String(finalHour) + ":" + String(finalMinute);
                        Particle.publish("Time: ", endingTime, PUBLIC);
                        timeMissing = 0;
                        timesOutOfBed++;
                    }
                }
            }
            
            // if not already in bed
            else{
                // if now in bed
                if(checkForPresence(currentWeight)){
                    inBedState = inBed;
                    String startingTime = "Starting Time: " + String(startingHour) + ":" + String(startingMinute);
                    Particle.publish("Time: ", startingTime, PUBLIC);
                    timeMissing = 0;
                }
                
            }
            
            break;
        }
        
        case TimeToWakeUp: 
        {
            double currentWeight = CalculateWeight();
            if(inBedState == inBed){
                if(checkForMissing(currentWeight)){
                    // If the individual is missing for more than 15 minutes
                    if(checkHowLongMissingMorning()){
                        inBedState = outOfBed;
                        calculateDuration();
                        calculateFinalNightDuration();
                        String endingTime = "Ending time: " + String(finalHour) + ":" + String(finalMinute);
                        Particle.publish("Time: ", endingTime, PUBLIC);
                        delay(1000);
                        String reportTimesUp = "{\"TimesUp\": " + String(timesOutOfBed) + "}";
                        Particle.publish("TimesUp", String(timesOutOfBed), PUBLIC);
                        delay(1000);
                       // Particle.publish("OutOfBed: " + reportTimesUp, PUBLIC);
                       // delay(1000);
                        Particle.publish("Up and OK");
                        timeMissing = 0;
                        upAndOkay = 1;
                    }
                }
            }
            
            // if not already in bed
            else{
                // if now in bed
                if(checkForPresence(currentWeight)){
                    inBedState = inBed;
                    String startingTime = "Starting Time: " + String(startingHour) + ":" + String(startingMinute);
                    Particle.publish("Time: ", startingTime, PUBLIC);
                    timeMissing = 0;
                }
                
            }
            
            break;
        }
        
        case DayTime: 
        {
            if(!firstTime){
                firstTime = 1;
                if(!upAndOkay){
                    String stillInBed = "Alert! Still in bed!";
                    Particle.publish("In the morning: ", stillInBed, PUBLIC);
                }
                nextDayReset();
                newTare();
            }
            
            CalculateWeight();
            
            break;
        }
        
        case TimeToAlert: 
        {
            break;
        }
    }
    
    // Delay for 30 seconds
   // delay(30000);
     delay(10000);
}

/*************************************************************************/
/*          The following function determines timeOfDay                 */
/***********************************************************************/

void determineTimeOfDay(){
    
    // If it is time to tare
    if(digitalRead(D2)){
        currentState = TimeToTare;
    }
    
    // if it is 7 PM to 5 AM
    else if((Time.hour() > 19) || (Time.hour() < 5)){
    //else if((Time.hour() >= 8) && (Time.hour() < 9)){
        currentState = NightTime;
    }
    
    // if it is between 5 AM and 11 AM
    else if((Time.hour() >= 5) && (Time.hour() < 8)){
        currentState = TimeToWakeUp;
    }
    
    // else it is during the day
    else{
        currentState = DayTime;
    }
    
}

/*************************************************************************/
/*          The following function calculates the tare                  */
/***********************************************************************/

double CalculateTare(){
    double finalAverage;
    double value4 = (double (scale4.read())+11551)/129894;
    double value3 = (double (scale3.read())-25361)/129797;
    double value2 = (double (scale2.read())+126282)/124890;
    double value1 = (double (scale1.read())+148163)/120921;
  
    // Total weight, summed together
    double localTare = value1+value2+value3+value4;
    return localTare;
}

/*************************************************************************/
/*          The following function calculates the weight                */
/***********************************************************************/

double CalculateWeight(){
    double finalAverage;
    double value4 = (double (scale4.read())+11551)/129894;
    double value3 = (double (scale3.read())-25361)/129797;
    double value2 = (double (scale2.read())+126282)/124890;
    double value1 = (double (scale1.read())+148163)/120921;
  
    // Total weight, summed together
    double theSum = value1+value2+value3+value4-tare;
    double finalWeight = theSum;
  
    String Weight = "{\"Weight\":" + String(finalWeight) + "}";
    Particle.publish("weight", Weight, PUBLIC);
    
    return finalWeight;
}

/*************************************************************************/
/*          The following function calculates the duration               */
/*                    the person was in bed                             */
/***********************************************************************/

void calculateDuration(){
    int localduration = 0;
    
    int start_minutes = startingHour * 60 + startingMinute;
    int end_minutes = finalHour * 60 + finalMinute;
    localduration += end_minutes - start_minutes;
    
    if(localduration < 0){
        localduration += localduration + 1440;
    }
    
    duration += localduration;
    
}

/*************************************************************************/
/*          The following function determines whether an                */
/*                    individual is in bed.                             */
/***********************************************************************/

int checkForPresence(double weight){
    if(weight < (1.2 * theWeight) && weight > (0.8 * theWeight)){
        startingHour = Time.hour();
        startingMinute = Time.minute();
        return 1;
    }
    else{
        return 0;
    }
}

/**********************************************************************/
/*           The following function determines how long the           */
/*          individual has been missing from bed. If they have been   */
/*          missing for more than a minute, it returns a 1            */
/*********************************************************************/ 

int checkHowLongMissing(){
    // Gone for 2 minutes
    if(timeMissing++ == 4){
        timeMissing = 0;
        return 1;
    }
    else{
        return 0;
    }
}   

/**********************************************************************/
/*           The following function determines how long the           */
/*          individual has been missing from bed in the morning.      */
/*          If they have been missing for more than 15 minutes,       */
/*          it returns a 1                                          */
/**********************************************************************/ 

int checkHowLongMissingMorning(){
    if(timeMissing++ == 30){
        timeMissing = 0;
        return 1;
    }
    else{
        return 0;
    }      
}

/*************************************************************************/
/*          The following function calculates the duration               */
/*                    the person was in bed                             */
/***********************************************************************/

void calculateFinalNightDuration(){
    Particle.publish("total duration", String(duration));
    delay(1000);
    durationHours = duration/60;
    durationMinutes = duration%60;
    Particle.publish("duration in minutes: ", String(durationMinutes), PUBLIC);
    delay(1000);
    if(durationHours > 0){
        String duration = "{\"duration\":" + String(durationHours) + " hour(s) " + String(durationMinutes) + " minutes.}";
        Particle.publish("duration", duration, PUBLIC);
        delay(1000);
    }
}

/*************************************************************************/
/*          The following function checks if the individual             */
/*                    is out of bed                                     */
/***********************************************************************/

int checkForMissing(double weight){
    if((weight < (0.8 * theWeight))){
            finalHour = Time.hour();
            finalMinute = Time.minute();
            return 1;
    }
    else{
        return 0;
    }
}

/*************************************************************************/
/*          The following function reinitializes the system for          */
/*                    a new day                                         */
/***********************************************************************/

void nextDayReset(){
    upAndOkay = 0;
    duration = 0;
    durationHours = 0;
    durationMinutes= 0;
    timeMissing = 0;
    timesOutOfBed = 0;
    
    // calculate new tare ever morning
    newTare();
}

/*************************************************************************/
/*          The following function reinitializes the tare for            */
/*                    the  day                                          */
/***********************************************************************/

void newTare(){
    tare = CalculateTare(); 
}
