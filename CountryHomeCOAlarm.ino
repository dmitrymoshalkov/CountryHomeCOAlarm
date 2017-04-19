
#define MY_OTA_FIRMWARE_FEATURE
#define MY_RADIO_NRF24
#define MY_RF24_CHANNEL	86
#define MY_NODE_ID 101
//#define MY_DEBUG // Enables debug messages in the serial log
#define MY_BAUD_RATE 115200




#include <MySensors.h>  
#include <SPI.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <avr/wdt.h>
#include <SimpleTimer.h>

 //#define NDEBUG                        // enable local debugging information

#define SKETCH_NAME "CO Alarm OTA"
#define SKETCH_MAJOR_VER "1"
#define SKETCH_MINOR_VER "51"
//#define NODE_ID 50 //or AUTO to let controller assign                                                         |" "

#define SMOKE_CHILD_ID 							0
#define TEMP_CHILD_ID 						 60
#define POWERONOFF_CHILD_ID 					120
#define SIRENONOFF_CHILD_ID 					121
#define SERVICELIGHT_CHILD_ID					131

#define REBOOT_CHILD_ID                       100
#define RECHECK_SENSOR_VALUES                 101 

/*****************************************************************************************************/
/*                               				Common settings									      */
/******************************************************************************************************/
#define RADIO_RESET_DELAY_TIME 50 //Задержка между сообщениями
#define MESSAGE_ACK_RETRY_COUNT 5  //количество попыток отсылки сообщения с запросом подтверждения
#define DATASEND_DELAY  10

boolean gotAck=false; //подтверждение от гейта о получении сообщения 
int iCount = MESSAGE_ACK_RETRY_COUNT;

boolean boolRecheckSensorValues = false;

#define SIREN_SENSE_PIN A0  // Arduino Digital I/O pin for optocoupler for siren
//#define DWELL_TIME 125  // this allows for radio to come back to power after a transmission, ideally 0
#define SMOKEALARM_CONTROL 4
#define SIREN_CONTROL A4
#define ALARMLIGHT_PIN 5
#define TEMPERATURE_PIN 3


#define CRITICALWORKTEMP 5



OneWire oneWire(TEMPERATURE_PIN);        // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire);  // Pass the oneWire reference to Dallas Temperature. 
float lastTemp1 = 0;

byte oldValue = 0;
byte smokeValue;


boolean bSirenOn = true;
boolean bPowerOn = true;
boolean blightON = false;
boolean bPoweredOffbyMessage = false;
boolean bLastLightState = false;
boolean bWasBlink = false;
//boolean bblinkLed = false;
int blinkTimer = -1;

boolean breportState = false;

SimpleTimer timerCheckLastMark;

long previousMillis = 0; 

//MySensor sensor_node;
MyMessage msgSmoke(SMOKE_CHILD_ID, V_TRIPPED);
MyMessage msgPowerStatus(POWERONOFF_CHILD_ID, V_STATUS);
MyMessage msgSirenStatus(SIRENONOFF_CHILD_ID, V_STATUS);

MyMessage msgAmbientTemp(TEMP_CHILD_ID, V_TEMP);

MyMessage msgServiceLightStatus(SERVICELIGHT_CHILD_ID, V_STATUS);

void before() 
{
 // This will execute before MySensors starts up 


  #ifdef NDEBUG
  Serial.println("Before");
  #endif  

  // Setup the Siren Pin HIGH
  pinMode(SIREN_SENSE_PIN, INPUT_PULLUP);//INPUT_PULLUP

  pinMode(SMOKEALARM_CONTROL, OUTPUT);
  digitalWrite(SMOKEALARM_CONTROL, HIGH);  
  bPowerOn = true;
  
  pinMode(SIREN_CONTROL, OUTPUT);
  digitalWrite(SIREN_CONTROL, LOW);  
  bSirenOn = false;

  pinMode(TEMPERATURE_PIN, INPUT);


  pinMode(ALARMLIGHT_PIN, OUTPUT);
  analogWrite(ALARMLIGHT_PIN, 0); 
  blightON = false;

  //wait(2000);

}


void presentation() 
{
  #ifdef NDEBUG
  Serial.println("Present");
  #endif  
  // Send the sketch version information to the gateway and Controller
sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER"."SKETCH_MINOR_VER);
wait(RADIO_RESET_DELAY_TIME);

present(SMOKE_CHILD_ID, S_SMOKE);
wait(RADIO_RESET_DELAY_TIME);

  //reboot sensor command
  present(REBOOT_CHILD_ID, S_BINARY); 
  wait(RADIO_RESET_DELAY_TIME);

  present(POWERONOFF_CHILD_ID, S_BINARY); 
  wait(RADIO_RESET_DELAY_TIME);

  present(SIRENONOFF_CHILD_ID, S_BINARY); 
  wait(RADIO_RESET_DELAY_TIME);


  present(SERVICELIGHT_CHILD_ID, S_BINARY); 
  wait(RADIO_RESET_DELAY_TIME);

  //reget sensor values
present(RECHECK_SENSOR_VALUES, S_LIGHT); 
wait(RADIO_RESET_DELAY_TIME);  


present(TEMP_CHILD_ID, S_TEMP);
wait(RADIO_RESET_DELAY_TIME);

  #ifdef NDEBUG
  Serial.println("Request state");
  #endif  



  request(POWERONOFF_CHILD_ID, V_STATUS);
  wait(RADIO_RESET_DELAY_TIME);


  request(SIRENONOFF_CHILD_ID, V_STATUS);
  wait(RADIO_RESET_DELAY_TIME);


  request(SERVICELIGHT_CHILD_ID, V_STATUS);
  wait(RADIO_RESET_DELAY_TIME);

  wait(5000);

  breportState = true;


bool bLastLight = blightON;


      if (blightON)
      {
        analogWrite(ALARMLIGHT_PIN, 0); 
        blightON = false;  

      }

      analogWrite(ALARMLIGHT_PIN, 190); 
      blightON = true;
      wait(300);
      analogWrite(ALARMLIGHT_PIN, 0); 
      blightON = false;  
      wait(300);     
      analogWrite(ALARMLIGHT_PIN, 190); 
      blightON = true;
      wait(300);
      analogWrite(ALARMLIGHT_PIN, 0); 
      blightON = false;  


if (bLastLight)      
{
      analogWrite(ALARMLIGHT_PIN, 190); 
      blightON = true;
}
else
{
      analogWrite(ALARMLIGHT_PIN, 0); 
      blightON = false;  
}


  	//Enable watchdog timer
  	wdt_enable(WDTO_8S);  


    

}

void setup()
{

  #ifdef NDEBUG
  Serial.println("Setup");
  #endif  

	  timerCheckLastMark.setInterval(60000, checkTemp); 
	  //blinkTimer = timerCheckLastMark.setInterval(1000, blinkLed); 
		//timerCheckLastMark.disable(blinkTimer);

      wait(5000);
}

// Loop will iterate on changes on the BUTTON_PINs
void loop()
{
  // Check to see if we have a alarm. I always want to check even if we are coming out of sleep for heartbeat.
  AlarmStatus();

 // sensor_node.process();

if ( breportState )
{

	reportState();
	breportState = false;
}



	timerCheckLastMark.run();

  //reset watchdog timer
  wdt_reset();   

  // Sleep until we get a audio power hit on the optocoupler or 9hrs
  //sensor_node.sleep(SIREN_SENSE_PIN - 2, CHANGE, SLEEP_TIME * 1000UL);
}

void AlarmStatus()
{

  // We will check the status now, this could be called by an interrupt or heartbeat
  byte value;
  byte wireValue;
  byte previousWireValue;
  byte siren_low_count  = 0;
  byte siren_high_count  = 0;
  unsigned long startedAt = millis();

  #ifdef NDEBUG
  //Serial.println("Status Check");
  #endif  
  
  //Read the Pin
  value = digitalRead(SIREN_SENSE_PIN);
  // If Pin returns a 0 (LOW), then we have a Alarm Condition
  if (value == 1 && bPowerOn )
  {



   	  #ifdef NDEBUG
      Serial.println("Smoke Detected");
   	  #endif       
      //Check to make sure we haven't already sent an update to controller
      if (smokeValue == 0 )
      {
  
    	  digitalWrite(SIREN_CONTROL, HIGH);  
  	  	  bSirenOn = true;

		//timerCheckLastMark.enable(blinkTimer);

            //Отсылаем состояние сенсора с подтверждением получения
            iCount = MESSAGE_ACK_RETRY_COUNT;

              while( !gotAck && iCount > 0 )
                {
                   // Send in the new temperature                  
                   send(msgSmoke.set("1"), true);
                   wait(RADIO_RESET_DELAY_TIME);
                  iCount--;
                 }

                gotAck = false;

   	    #ifdef NDEBUG
        Serial.println("Smoke Detected sent to gateway");
   	    #endif             
        
        smokeValue = 1;



            iCount = MESSAGE_ACK_RETRY_COUNT;

              while( !gotAck && iCount > 0 )
                {
                   // Send in the new temperature                  
                   send(msgSirenStatus.set(bSirenOn?"0":"1"), true);
                   wait(RADIO_RESET_DELAY_TIME);
                  iCount--;
                 }

                gotAck = false;

                bLastLightState = blightON ;

      }

    wait(100);

    blinkLed();
      
    oldValue = value;
    wdt_reset(); 
    AlarmStatus(); //run AlarmStatus() until there is no longer an alarm
  }
  //Pin returned a 1 (High) so there is no alarm.
  else
  {
    //If value has changed send update to gateway.
    if (oldValue != value || boolRecheckSensorValues)
    {
      //Send all clear msg to controller
          
        digitalWrite(SIREN_CONTROL, LOW);  
  		bSirenOn = false;    
          //Отсылаем состояние сенсора с подтверждением получения
            iCount = MESSAGE_ACK_RETRY_COUNT;

              while( !gotAck && iCount > 0 )
                {
                                   
                   send(msgSmoke.set("0"), true);
                    wait(RADIO_RESET_DELAY_TIME);
                  iCount--;
                 }
                 
                gotAck = false;

            iCount = MESSAGE_ACK_RETRY_COUNT;

              while( !gotAck && iCount > 0 )
                {
                   // Send in the new temperature                  
                   send(msgSirenStatus.set(bSirenOn?"0":"1"), true);
                   wait(RADIO_RESET_DELAY_TIME);
                  iCount--;
                 }

                gotAck = false;

		//timerCheckLastMark.disable(blinkTimer);
 		
		if (blightON && !boolRecheckSensorValues && !bLastLightState && bWasBlink)
		{
 			analogWrite(ALARMLIGHT_PIN, 0); 
  			blightON = false;

  		}
      else if ( bLastLightState )
      {
      analogWrite(ALARMLIGHT_PIN, 190); 
        blightON = true;

      }

        bWasBlink = false;
      oldValue = value;
      smokeValue = 0;
      boolRecheckSensorValues = false;

      bLastLightState = false;

   	  #ifdef NDEBUG      
      Serial.println("No Alarm");
   	  #endif        
    }
  }
}


void reportState()
{

            iCount = MESSAGE_ACK_RETRY_COUNT;

              while( !gotAck && iCount > 0 )
                {
                   // Send in the new temperature                  
                   send(msgPowerStatus.set(bPowerOn?"0":"1"), true);
                    wait(RADIO_RESET_DELAY_TIME);
                  iCount--;
                 }
                 
                gotAck = false;


            iCount = MESSAGE_ACK_RETRY_COUNT;

              while( !gotAck && iCount > 0 )
                {
                   // Send in the new temperature                  
                   send(msgSirenStatus.set(bSirenOn?"0":"1"), true);
                    wait(RADIO_RESET_DELAY_TIME);
                  iCount--;
                 }
                 
                gotAck = false;


            iCount = MESSAGE_ACK_RETRY_COUNT;

              while( !gotAck && iCount > 0 )
                {
                   // Send in the new temperature                  
                   send(msgServiceLightStatus.set(blightON?"1":"0"), true);
                    wait(RADIO_RESET_DELAY_TIME);
                  iCount--;
                 }
                 
                gotAck = false;

}



void receive(const MyMessage &message) 
{
 // Handle incoming message 

  if (message.isAck())
  {
    gotAck = true;
    return;

  }

    if ( message.sensor == REBOOT_CHILD_ID && message.getBool() == true && strlen(message.getString())>0 ) {
    	   	  #ifdef NDEBUG      
      			Serial.println("Received reboot message");
   	  			#endif    
             //wdt_enable(WDTO_30MS);
              while(1) { 
                    //wait(50); 
                      analogWrite(ALARMLIGHT_PIN, 190); 
                      blightON = true;
                      delay(100);
                      analogWrite(ALARMLIGHT_PIN, 0); 
                      blightON = false;  
                      delay(100);                    
              };

     }
     



    if ( message.sensor == RECHECK_SENSOR_VALUES && strlen(message.getString())>0 ) {
         
         if (message.getBool() == true)
         {
            boolRecheckSensorValues = true;
            breportState = true;
         }

         return;

     }

    if ( message.sensor == POWERONOFF_CHILD_ID && strlen(message.getString())>0 ) {
         
         if (message.getBool() == true)
         {
         	digitalWrite(SMOKEALARM_CONTROL, LOW); 
            bPowerOn = false;
            bPoweredOffbyMessage = true;
         }
         else
         {
         	digitalWrite(SMOKEALARM_CONTROL, HIGH); 
            bPowerOn = true;  
            bPoweredOffbyMessage = false;  
            wait(15000);     	
         }

         reportState();

         return;	

     }

    if ( message.sensor == SIRENONOFF_CHILD_ID && strlen(message.getString())>0 ) {
         
         if (message.getBool() == true)
         {
         	digitalWrite(SIREN_CONTROL, HIGH); 
            bSirenOn = true;
         }
         else
         {
         	digitalWrite(SIREN_CONTROL, LOW); 
            bSirenOn = false;         	
         }
    	   	  #ifdef NDEBUG      
      			Serial.println("Report siren state");
   	  			#endif   

         reportState();
         return;	
     }


    if ( message.sensor == SERVICELIGHT_CHILD_ID && strlen(message.getString())>0 ) {
         
         if (message.getBool() == true)
         {
		  analogWrite(ALARMLIGHT_PIN, 190  ); 
		  blightON = true;
         }
         else
         {
		  analogWrite(ALARMLIGHT_PIN, 0); 
		  blightON = false;      	
         }

         reportState();
         return;	

     }

        return;      

}




void checkTemp()
{

byte value =2;
// Fetch temperatures from Dallas sensors
  sensors.requestTemperatures();

  // query conversion time and sleep until conversion completed
  int16_t conversionTime = sensors.millisToWaitForConversion(sensors.getResolution());
  // sleep() call can be replaced by wait() call if node need to process incoming messages (or if node is repeater)

       wait(conversionTime+5);


 float temperature = static_cast<float>(static_cast<int>(sensors.getTempCByIndex(0) * 10.)) / 10.;

          		#ifdef NDEBUG                
                Serial.print ("Temp: ");
          	    Serial.println (temperature); 
          	    #endif

         if (temperature != lastTemp1 && temperature != -127.00 && temperature != 85.00 ) {



					//report relay status
				    iCount = MESSAGE_ACK_RETRY_COUNT;

                    while( !gotAck && iCount > 0 )
                      {
            
                        send(msgAmbientTemp.set(temperature,1), true);
                         wait(RADIO_RESET_DELAY_TIME);
                        iCount--;
                       }

                      gotAck = false; 

            lastTemp1 = temperature;
        	} 


        	if ( temperature < CRITICALWORKTEMP && temperature != -127.00 && temperature != 85.00 && bPowerOn)
        	{
        		
          		#ifdef NDEBUG                
          	    Serial.println ("Power off sensor"); 
          	    #endif

  				digitalWrite(SMOKEALARM_CONTROL, LOW);  
  				bPowerOn = false;


				reportState();

				

        	}
        	else
        	{
        		if ( temperature >= CRITICALWORKTEMP && temperature != -127.00 && temperature != 85.00 && !bPowerOn && !bPoweredOffbyMessage)
        		{
  					digitalWrite(SMOKEALARM_CONTROL, HIGH);  
  					bPowerOn = true;
  					reportState();

  				#ifdef NDEBUG                
          	    Serial.println ("Power on sensor"); 
          	    #endif

        		}
        	}
    


}


void blinkLed()
{

    unsigned long currentMillis = millis();

    if(currentMillis - previousMillis > 500 ) 
    {
    	previousMillis = currentMillis;   
      bWasBlink = true;

		if (!blightON)
		{
		  analogWrite(ALARMLIGHT_PIN, 190); 
		  blightON = true;
		}
		else
		{
		  analogWrite(ALARMLIGHT_PIN, 0); 
		  blightON = false;
		}
	}

}
