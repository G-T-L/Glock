////////////////////////////fix me////////////////////////////
//舵机问题									OK
//做灯光模块									OK
//加入红外控制								OK
//音效问题,灯光与音效不同步		
//开门程序									OK 
//SIM卡短信溢出的删除操作				
//SIM卡其它功能的引入				
//红外失灵
//供电电路

//////////////////////////////////////Librarys///////////////////////////////////////
#include <IRremote.h>
#include <MFRC522Extended.h>
#include <sim900_modified.h>
#include <GPRS_Shield_Arduino_modified.h>
#include <EEPROM.h>     // We are going to read and write PICC's UIDs from/to EEPROM
#include <SPI.h>              // RC522 Module uses SPI protocol
#include <MFRC522.h> 	// Library for Mifare RC522 Devices
#include <Servo.h>
//#include<string.h>

////////////////////////////////////Components Slection////////////////////////////
///////////////////////Add and Enable here or disable and remove///////////////
//#define GPRSControl                     //GPRS模块(如Sim900A)
#define DoorStatusDetector           //门是否开启检测模块  如干簧管
#define ServoControlRelay             //用继电器来控制舵机   !!!需硬件电路支持  如果添加,去#ifdef DoorStatusDetector里面修改引脚
//#define InfraredControl                 //有无红外控制   如果添加,去#ifdef InfraredControl里面修改引脚
#define LEDIndicator
///////////////////////////////Components Settings//////////////////////////////////
//#define COMMON_ANODE                            //指示灯共阳还是共阴
//#define LOW_LEVEL_TRIGGER                       //继电器模式
//#define UNO                                                  //板子的选择(接线方式)
#define MEGA2560

////////////////////////////////arguments adjusting////////////////////////////////
#define BeepInterval 100            //提示音  
#define AlarmInterval 1000        //长鸣报警
#define DoorOpenAngle 100     //开门时舵机转动角度设置
#define DoorCloseAngle 20         //关门是舵机默认角度
char masterphone[15] = "+8618845787928";       //the num to command the system
#define OpenDoorCmd "open the door"
#define AlarmCmd "alarm"

//////////////////////////////////////////Pins layout///////////////////////////////////

#ifdef GPRSControl
boolean isGprsExist = true;//能自动检测,如果初始化失败则自动变为false,并禁用SMS服务
#define GPRSReferenceVccPin 16
#define GPRSReferenceGndPin 17
//just power on and connect to the Serial
#else
boolean isGprsExist = false;
#define GPRSReferenceVccPin 0
#define GPRSReferenceGndPin 0

#endif // GPRSControl

#ifdef ServoControlRelay
boolean isServoControlRelayExist = true;
#define ServoControlRelayGndPin 22 
#define RelayControlPin 24
#else
boolean isServoControlRelayExist = false;
#define ServoControlRelayGndPin 0 
#define RelayControlPin 0
#endif // ServoControlRelay
#ifdef LOW_LEVEL_TRIGGER     
#define POWERON LOW
#define POWEROFF HIGH
#else                                              
#define POWERON HIGH
#define POWEROFF LOW
#endif // LOW_LEVEL_TRIGGER

#ifdef DoorStatusDetector
boolean isDoorStatusDetectorExist = true;
#define DoorStatusDetectorPin   25              //干簧管
#define DoorStatusDetectorVccPin 27
#define DoorStatusDetectorGndPin 26
#else
boolean isDoorStatusDetectorExist = false;
#define DoorStatusDetectorPin   0
#define DoorStatusDetectorVccPin 0
#define DoorStatusDetectorGndPin 0
#endif // DoorStatusDetector

#ifdef InfraredControl
boolean isInfraredControlExist = true;
#define InfraredRemoteReceiverPin 44
#define InfraredRemoteReceiverVccPin 46
#define InfraredRemoteReceiverGndPin 48

#else
boolean isInfraredControlExist = false;
#define InfraredRemoteReceiverPin 0
#define InfraredRemoteReceiverVccPin 0
#define InfraredRemoteReceiverGndPin 0

#endif // InfraredControl

#ifdef LEDIndicator
boolean isLEDIndicatorExist = true;
#define redLed 47 	
#define greenLed 45
#define blueLed 43
#else
boolean isLEDIndicatorExist = false;
#define redLed 0	
#define greenLed 0
#define blueLed 0
#endif // LEDIndicator
#ifdef COMMON_ANODE
#define LED_ON LOW
#define LED_OFF HIGH
#else
#define LED_ON HIGH
#define LED_OFF LOW
#endif //COMMON_ANODE

/*
Typical pin layout used:
* -----------------------------------------------------------------------------------------
*             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
*             Reader/PCD   Uno           Mega      Nano v3    Leonardo/Micro   Pro Micro
* Signal      Pin          Pin           Pin       Pin        Pin              Pin
* -----------------------------------------------------------------------------------------
* RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
* SPI SS      SDA(SS)      10            53        D10        10               10
* SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
* SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
* SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
*/
#ifdef UNO
//Pin layout should be as follows
//SPI Pins Cannot change
//MOSI : Pin 11 / ICSP - 4
//MISO : Pin 12 / ICSP - 1
//SCK : Pin 13 / ICSP - 3

//other pins are configurable
#define redLed 5 	// Set Led Pins
#define greenLed 7
#define blueLed 6

#define servoPin 8           //Set Servo pin
#define wipeB 2		// Button pin for WipeMode

#define SS_PIN 10
#define RST_PIN 13
#endif // UNO

#ifdef MEGA2560
//Pin layout should be as follows
//SPI Pins Cannot change
//MOSI 51
//MISO 50
//SCK 52

//other pins are configurable
#define SS_PIN 53
#define RST_PIN 49

#define servoPin 23           //Set Servo pin
#define wipeB 2		// Button pin for WipeMode
#define buzzerGnd A3
#define buzzer A0

#endif // MEGA2560

boolean isUsingCustomServo = false;
boolean match = false;          // initialize card match to false
boolean programMode = false;	// initialize programming mode to false
int successRead;		// Variable integer to keep if we have Successful Read from Reader

byte storedCard[4];		// Stores an ID read from EEPROM
byte readCard[4];		// Stores scanned ID read from RFID Module
byte masterCard[4];		// Stores master card's ID read from EEPROM

MFRC522 mfrc522(SS_PIN, RST_PIN);	// Create MFRC522 instance.
Servo daServo;
GPRS gprs(&Serial1, 9600);
IRrecv irrecv(InfraredRemoteReceiverPin);

///////////////////////////////functions declearation//////////////////////////////////
void granted();
void denied();
int getID();
void ShowReaderDetails();
void cycleLeds();
void normalModeOn();
void readID(int number);
void writeID(byte a[]);
void deleteID(byte a[]);
boolean checkTwo(byte a[], byte b[]);
int findIDSLOT(byte find[]);
boolean findID(byte find[]);
void successWrite();
void failedWrite();
void successDelete();
boolean isMaster(byte test[]);
void servo(int datPos);
void beep();
void alarm();
void alarmTriggeredBySMS();
bool isDoorOpened();
bool isCommandContained(const char* cmd, char* message, int length);
void gprsFunctions();
void infraredControl();
void customServo(int angle);
///////////////////////////////////////// Setup ///////////////////////////////////
void setup()
{
	//模拟地
	pinMode(22, OUTPUT);
	pinMode(23, OUTPUT);
	digitalWrite(22, LOW);
	digitalWrite(23, LOW);

	Serial.begin(9600);	 // Initialize serial communications with PC
	if (Serial)
		Serial.println("Serial Ready");

	if (isLEDIndicatorExist)
	{
		pinMode(redLed, OUTPUT);
		pinMode(greenLed, OUTPUT);
		pinMode(blueLed, OUTPUT);
		digitalWrite(redLed, LED_OFF);	// Make sure led is off
		digitalWrite(greenLed, LED_OFF);	// Make sure led is off
		digitalWrite(blueLed, LED_OFF);	// Make sure led is off
		Serial.println("LED Indicator initiallized");
	}
	else
	{
		Serial.println("LED indicator is missing");
	}

	//DoorStatusDetector Pins initiallize
	if (isDoorStatusDetectorExist)
	{
		pinMode(DoorStatusDetectorGndPin, OUTPUT);
		pinMode(DoorStatusDetectorVccPin, OUTPUT);
		digitalWrite(DoorStatusDetectorGndPin, LOW);
		digitalWrite(DoorStatusDetectorVccPin, LOW);
		Serial.println("Door Status Detector initiallized");
	}
	else
	{
		Serial.println("Skip the Door Status Detector part");
	}

	pinMode(buzzer, OUTPUT);
	pinMode(buzzerGnd, OUTPUT);
	digitalWrite(buzzer, LOW);
	digitalWrite(buzzerGnd, LOW);
	Serial.println("Buzzer initiallized");
	
	if (isServoControlRelayExist)
	{
		pinMode(ServoControlRelayGndPin, OUTPUT);
		pinMode(RelayControlPin, OUTPUT);
		digitalWrite(ServoControlRelayGndPin, LOW);
		Serial.println("Relay initiallized");
	}
	//Be careful how servo circuit behave on while resetting or power-cycling your Arduino
	digitalWrite(RelayControlPin, POWERON);
	daServo.attach(servoPin);
	servo(DoorCloseAngle);		// Make sure door is locked
	digitalWrite(RelayControlPin, POWEROFF);
	Serial.println("Servo initiallized");

	if (isInfraredControlExist)
	{
		pinMode(InfraredRemoteReceiverVccPin, OUTPUT);
		pinMode(InfraredRemoteReceiverGndPin, OUTPUT);
		digitalWrite(InfraredRemoteReceiverVccPin, HIGH);
		digitalWrite(InfraredRemoteReceiverGndPin, LOW);
		irrecv.enableIRIn(); // Start the receiver
		Serial.println("Infrared Remote receiver initiallized");
	}
	else
	{
		Serial.println("Skip the Infrared Control part");
	}

	//Protocol Configuration
	SPI.begin();           // MFRC522 Harqadware uses SPI protocol
	mfrc522.PCD_Init();    // Initialize MFRC522 Hardware
	//ttl参考电平初始化,后检测是否连接成功
	if (isGprsExist)
	{
		pinMode(GPRSReferenceVccPin, OUTPUT);
		pinMode(GPRSReferenceGndPin, OUTPUT);
		digitalWrite(GPRSReferenceVccPin, HIGH);
		digitalWrite(GPRSReferenceGndPin, LOW);
		delay(10);
		isGprsExist = gprs.init();//if initiallize failed,disable the model and skip the SMS function
	}
	if (isGprsExist)
	{
		Serial.println("GPRS initiallized");
		gprs.sendSMS(masterphone, "Door Lock device initialized");
		//delay(1000);
		//gprs.sendSMS(masterphone, "Door Lock device initialized");
	}
	else
	{
		Serial.println("GPRS initiallize failed or not exist");
	}

	//If you set Antenna Gain to Max it will increase reading distance
	mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
	Serial.println(F("Access Control v3.3"));   // For debugging purposes
	ShowReaderDetails();	// Show details of PCD - MFRC522 Card Reader details

	//Wipe Code if Button Pressed while setup run (powered on) it wipes EEPROM
	pinMode(wipeB, INPUT_PULLUP);		// Enable pin's pull up resistor
	if (digitalRead(wipeB) == LOW)
	{	// when button pressed pin should get low, button connected to ground
		digitalWrite(redLed, LED_ON);	// Red Led stays on to inform user we are going to wipe
		Serial.println(F("Wipe Button Pressed"));                         
		Serial.println(F("You have 5 seconds to Cancel"));
		Serial.println(F("This will remove all records and cannot be undone"));
		alarm();
		delay(5000);                        // Give user enough time to cancel operation
		beep();
		if (digitalRead(wipeB) == LOW)
		{    // If button still be pressed, wipe EEPROM
			Serial.println(F("Starting Wiping EEPROM"));
			for (int x = 0; x < EEPROM.length(); x = x + 1)
			{    //Loop end of EEPROM address
				if (EEPROM.read(x) == 0)
				{              //If EEPROM address 0
							   // do nothing, already clear, go to the next address in order to save time and reduce writes to EEPROM
				}
				else
				{
					EEPROM.write(x, 0); 			// if not write 0 to clear, it takes 3.3mS
				}
			}
			Serial.println(F("EEPROM Successfully Wiped"));
			digitalWrite(redLed, LED_OFF); 	// visualize successful wipe
			delay(200);
			digitalWrite(redLed, LED_ON);
			delay(200);
			digitalWrite(redLed, LED_OFF);
			delay(200);
			digitalWrite(redLed, LED_ON);
			delay(200);
			digitalWrite(redLed, LED_OFF);
			beep();
			beep();
			beep();
			alarm();
		}
		else
		{
			Serial.println(F("Wiping Cancelled"));
			digitalWrite(redLed, LED_OFF);
			beep();
			beep();
		}
	}
	// Check if master card defined, if not let user choose a master card
	// This also useful to just redefine Master Card
	// You can keep other EEPROM records just write other than 143 to EEPROM address 1
	// EEPROM address 1 should hold magical number which is '143'
	if (EEPROM.read(1) != 143)
	{
		Serial.println(F("No Master Card Defined"));
		Serial.println(F("Scan A PICC to Define as Master Card"));
		do
		{
			successRead = getID();            // sets successRead to 1 when we get read from reader otherwise 0
			digitalWrite(blueLed, LED_ON);    // Visualize Master Card need to be defined
			delay(200);
			digitalWrite(blueLed, LED_OFF);
			delay(200);
		} while (!successRead);                  // Program will not go further while you not get a successful read
		for (int j = 0; j < 4; j++)
		{        // Loop 4 times
			EEPROM.write(2 + j, readCard[j]);  // Write scanned PICC's UID to EEPROM, start from address 3
		}
		EEPROM.write(1, 143);                  // Write to EEPROM we defined Master Card.
		Serial.println(F("Master Card Defined"));
	}
	Serial.println(F("-------------------"));
	Serial.println(F("Master Card's UID"));
	for (int i = 0; i < 4; i++)
	{          // Read Master Card's UID from EEPROM
		masterCard[i] = EEPROM.read(2 + i);    // Write it to masterCard
		Serial.print(masterCard[i], HEX);
	}
	Serial.println("");
	Serial.println(F("-------------------"));
	Serial.println(F("Everything Ready"));
	Serial.println(F("Waiting PICCs to be scanned"));
	cycleLeds();    // Everything ready lets give user some feedback by cycling leds

	//granted();
}

///////////////////////////////////////// Main Loop ///////////////////////////////////
void loop()
{
	do
	{
		gprsFunctions();//send message to control the device 
		infraredControl();//use infrared to Control 
		successRead = getID(); 	// sets successRead to 1 when we get read from reader otherwise 0
		if (programMode)
		{
			cycleLeds();              // Program Mode cycles through RGB waiting to read a new card
		}
		else
		{
			normalModeOn(); 		// Normal mode, blue Power LED is on, all others are off
		}
	} while (!successRead); 	//the program will not go further while you not get a successful read
	if (programMode)
	{
		if (isMaster(readCard))
		{ //If master card scanned again exit program mode
			beep();
			beep();
			beep();
			Serial.println(F("Master Card Scanned"));
			Serial.println(F("Exiting Program Mode"));
			Serial.println(F("-----------------------------"));
			programMode = false;
			return;
		}
		else
		{
			if (findID(readCard))
			{ // If scanned card is known delete it
				Serial.println(F("I know this PICC, removing..."));
				deleteID(readCard);
				Serial.println("-----------------------------");
			}
			else
			{                    // If scanned card is not known add it
				Serial.println(F("I do not know this PICC, adding..."));
				writeID(readCard);
				Serial.println(F("-----------------------------"));
			}
		}
	}
	else
	{
		if (isMaster(readCard))
		{  	// If scanned card's ID matches Master Card's ID enter program mode
			beep();
			beep();
			beep();
			programMode = true;
			Serial.println(F("Hello Master - Entered Program Mode"));
			int count = EEPROM.read(0); 	// Read the first Byte of EEPROM that
			Serial.print(F("I have "));    	// stores the number of ID's in EEPROM
			Serial.print(count);
			Serial.print(F(" record(s) on EEPROM"));
			Serial.println("");
			Serial.println(F("Scan a PICC to ADD or REMOVE"));
			Serial.println(F("-----------------------------"));
		}
		else
		{
			if (findID(readCard))
			{	// If not, see if the card is in the EEPROM
				Serial.println(F("Welcome, You shall pass"));
				granted();
			}
			else
			{			// If not, show that the ID was not valid
				Serial.println(F("You shall not pass"));
				denied();
				if (isGprsExist)
				{
					/*
					char buffer[100] = { '\0' };
					gprs.getDateTime(buffer);

					String str1 = buffer;
					String str = "Someone attempted to open the door by wrong Rfid card at " + str1;
					//gprs.getLocation
					int length = str.length;
					for (int i = 0; i < str.length; i++)
						buffer[i] = str[i];
					gprs.sendSMS(masterphone, buffer);
					*/
					gprs.sendSMS(masterphone, "Someone attempted to open the door by wrong Rfid card ");
				}
			}
		}
	}
}

/////////////////////////////////////////  Access Granted    ///////////////////////////////////
void granted()
{
	if (isServoControlRelayExist)
	{
		digitalWrite(RelayControlPin, POWERON);
		digitalWrite(blueLed, LED_OFF); 	// Turn off blue LED
		digitalWrite(redLed, LED_OFF); 	// Turn off red LED
		digitalWrite(greenLed, LED_ON); 	// Turn on green LED
		beep();
		servo(DoorOpenAngle); 		// Unlock door!
		for (int i = 0; i < 100; i++)
		{
			if (!isDoorOpened())
			{
				servo(DoorOpenAngle); 		// hold the angle untill the door is pushed open
			}
			else
				break;
		} 
#ifdef DEBUG
		if (isDoorOpened())
			Serial.println("door opened");
		else
			Serial.println("door failed to open");
#endif // DEBUG

		servo(DoorCloseAngle); 		// Relock door
		digitalWrite(RelayControlPin, POWEROFF);
	}
	else
	{
		digitalWrite(blueLed, LED_OFF); 	// Turn off blue LED
		digitalWrite(redLed, LED_OFF); 	// Turn off red LED
		digitalWrite(greenLed, LED_ON); 	// Turn on green LED
		beep();
		servo(DoorOpenAngle); 		// Unlock door!
		if (!isDoorOpened())
			servo(DoorOpenAngle); 		// hold the angle untill the door is pushed open

		servo(DoorCloseAngle); 		// Relock door
	}
}

///////////////////////////////////////// Access Denied  ///////////////////////////////////
void denied()
{
	digitalWrite(greenLed, LED_OFF); 	// Make sure green LED is off
	digitalWrite(blueLed, LED_OFF); 	// Make sure blue LED is off
	digitalWrite(redLed, LED_ON); 	// Turn on red LED
	delay(1000);
	alarm();
}

///////////////////////////////////////// Get PICC's UID ///////////////////////////////////
int getID()
{
	// Getting ready for Reading PICCs
	if (!mfrc522.PICC_IsNewCardPresent())
	{ //If a new PICC placed to RFID reader continue
		return 0;
	}
	if (!mfrc522.PICC_ReadCardSerial())
	{   //Since a PICC placed get Serial and continue
		return 0;
	}
	// There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
	// I think we should assume every PICC as they have 4 byte UID
	// Until we support 7 byte PICCs
	Serial.println(F("Scanned PICC's UID:"));
	for (int i = 0; i < 4; i++)
	{  //
		readCard[i] = mfrc522.uid.uidByte[i];
		Serial.print(readCard[i], HEX);
	}
	Serial.println("");
	mfrc522.PICC_HaltA(); // Stop reading
	return 1;
}

void ShowReaderDetails()
{
	// Get the MFRC522 software version
	byte v = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
	Serial.print(F("MFRC522 Software Version: 0x"));
	Serial.print(v, HEX);
	if (v == 0x91)
		Serial.print(F(" = v1.0"));
	else if (v == 0x92)
		Serial.print(F(" = v2.0"));
	else
		Serial.print(F(" (unknown)"));
	Serial.println("");
	// When 0x00 or 0xFF is returned, communication probably failed
	if ((v == 0x00) || (v == 0xFF))
	{
		Serial.println(F("WARNING: Communication failure, is the MFRC522 properly connected?"));
		alarm();
		while (true);  // do not go further
	}
}

///////////////////////////////////////// Cycle Leds (Program Mode) ///////////////////////////////////
void cycleLeds()
{
	digitalWrite(redLed, LED_OFF); 	// Make sure red LED is off
	digitalWrite(greenLed, LED_ON); 	// Make sure green LED is on
	digitalWrite(blueLed, LED_OFF); 	// Make sure blue LED is off
	delay(200);
	digitalWrite(redLed, LED_OFF); 	// Make sure red LED is off
	digitalWrite(greenLed, LED_OFF); 	// Make sure green LED is off
	digitalWrite(blueLed, LED_ON); 	// Make sure blue LED is on
	delay(200);
	digitalWrite(redLed, LED_ON); 	// Make sure red LED is on
	digitalWrite(greenLed, LED_OFF); 	// Make sure green LED is off
	digitalWrite(blueLed, LED_OFF); 	// Make sure blue LED is off
	delay(200);
}

//////////////////////////////////////// Normal Mode Led  ///////////////////////////////////
void normalModeOn()
{
	digitalWrite(blueLed, LED_ON); 	// Blue LED ON and ready to read card
	digitalWrite(redLed, LED_OFF); 	// Make sure Red LED is off
	digitalWrite(greenLed, LED_OFF); 	// Make sure Green LED is off
	daServo.write(DoorCloseAngle);     //Make sure the door is locked
}

//////////////////////////////////////// Read an ID from EEPROM //////////////////////////////
void readID(int number)
{
	int start = (number * 4) + 2; 		// Figure out starting position
	for (int i = 0; i < 4; i++)
	{ 		// Loop 4 times to get the 4 Bytes
		storedCard[i] = EEPROM.read(start + i); 	// Assign values read from EEPROM to array
	}
}

///////////////////////////////////////// Add ID to EEPROM   ///////////////////////////////////
void writeID(byte a[])
{
	if (!findID(a))
	{ 		// Before we write to the EEPROM, check to see if we have seen this card before!
		int num = EEPROM.read(0); 		// Get the numer of used spaces, position 0 stores the number of ID cards
		int start = (num * 4) + 6; 	// Figure out where the next slot starts
		num++; 								// Increment the counter by one
		EEPROM.write(0, num); 		// Write the new count to the counter
		for (int j = 0; j < 4; j++)
		{ 	// Loop 4 times
			EEPROM.write(start + j, a[j]); 	// Write the array values to EEPROM in the right position
		}
		successWrite();
		Serial.println(F("Succesfully added ID record to EEPROM"));
	}
	else
	{
		failedWrite();
		Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
	}
}

///////////////////////////////////////// Remove ID from EEPROM   ///////////////////////////////////
void deleteID(byte a[])
{
	if (!findID(a))
	{ 		// Before we delete from the EEPROM, check to see if we have this card!
		failedWrite(); 			// If not
		Serial.println(F("Failed! There is something wrong with ID or bad EEPROM"));
	}
	else
	{
		int num = EEPROM.read(0); 	// Get the numer of used spaces, position 0 stores the number of ID cards
		int slot; 			// Figure out the slot number of the card
		int start;			// = ( num * 4 ) + 6; // Figure out where the next slot starts
		int looping; 		// The number of times the loop repeats
		int j;
		int count = EEPROM.read(0); // Read the first Byte of EEPROM that stores number of cards
		slot = findIDSLOT(a); 	// Figure out the slot number of the card to delete
		start = (slot * 4) + 2;
		looping = ((num - slot) * 4);
		num--; 			// Decrement the counter by one
		EEPROM.write(0, num); 	// Write the new count to the counter
		for (j = 0; j < looping; j++)
		{ 				// Loop the card shift times
			EEPROM.write(start + j, EEPROM.read(start + 4 + j)); 	// Shift the array values to 4 places earlier in the EEPROM
		}
		for (int k = 0; k < 4; k++)
		{ 				// Shifting loop
			EEPROM.write(start + j + k, 0);
		}
		successDelete();
		Serial.println(F("Succesfully removed ID record from EEPROM"));
	}
}

///////////////////////////////////////// Check Bytes   ///////////////////////////////////
boolean checkTwo(byte a[], byte b[])
{
	if (a[0] != NULL) 			// Make sure there is something in the array first
		match = true; 			// Assume they match at first
	for (int k = 0; k < 4; k++)
	{ 	// Loop 4 times
		if (a[k] != b[k]) 		// IF a != b then set match = false, one fails, all fail
			match = false;
	}
	if (match)
	{ 			// Check to see if if match is still true
		return true; 			// Return true
	}
	else
	{
		return false; 			// Return false
	}
}

///////////////////////////////////////// Find Slot   ///////////////////////////////////
int findIDSLOT(byte find[])
{
	int count = EEPROM.read(0); 			// Read the first Byte of EEPROM that
	for (int i = 1; i <= count; i++)
	{ 		// Loop once for each EEPROM entry
		readID(i); 								// Read an ID from EEPROM, it is stored in storedCard[4]
		if (checkTwo(find, storedCard))
		{ 	// Check to see if the storedCard read from EEPROM
			// is the same as the find[] ID card passed
			return i; 				// The slot number of the card
			break; 					// Stop looking we found it
		}
	}
}

///////////////////////////////////////// Find ID From EEPROM   ///////////////////////////////////
boolean findID(byte find[])
{
	int count = EEPROM.read(0);			// Read the first Byte of EEPROM that
	for (int i = 1; i <= count; i++)
	{  	// Loop once for each EEPROM entry
		readID(i); 					// Read an ID from EEPROM, it is stored in storedCard[4]
		if (checkTwo(find, storedCard))
		{  	// Check to see if the storedCard read from EEPROM
			return true;
			break; 	// Stop looking we found it
		}
		else
		{  	// If not, return false
		}
	}
	return false;
}

///////////////////////////////////////// Write Success to EEPROM   ///////////////////////////////////
// Flashes the green LED 3 times to indicate a successful write to EEPROM
void successWrite()
{
	digitalWrite(blueLed, LED_OFF); 	// Make sure blue LED is off
	digitalWrite(redLed, LED_OFF); 	// Make sure red LED is off
	digitalWrite(greenLed, LED_OFF); 	// Make sure green LED is on
	delay(200);
	digitalWrite(greenLed, LED_ON); 	// Make sure green LED is on
	delay(200);
	digitalWrite(greenLed, LED_OFF); 	// Make sure green LED is off
	delay(200);
	digitalWrite(greenLed, LED_ON); 	// Make sure green LED is on
	delay(200);
	digitalWrite(greenLed, LED_OFF); 	// Make sure green LED is off
	delay(200);
	digitalWrite(greenLed, LED_ON); 	// Make sure green LED is on
	delay(200);
	beep();
	beep();
}

///////////////////////////////////////// Write Failed to EEPROM   ///////////////////////////////////
// Flashes the red LED 3 times to indicate a failed write to EEPROM
void failedWrite()
{
	digitalWrite(blueLed, LED_OFF); 	// Make sure blue LED is off
	digitalWrite(redLed, LED_OFF); 	// Make sure red LED is off
	digitalWrite(greenLed, LED_OFF); 	// Make sure green LED is off
	delay(200);
	digitalWrite(redLed, LED_ON); 	// Make sure red LED is on
	delay(200);
	digitalWrite(redLed, LED_OFF); 	// Make sure red LED is off
	delay(200);
	digitalWrite(redLed, LED_ON); 	// Make sure red LED is on
	delay(200);
	digitalWrite(redLed, LED_OFF); 	// Make sure red LED is off
	delay(200);
	digitalWrite(redLed, LED_ON); 	// Make sure red LED is on
	delay(200);
	alarm();
}

///////////////////////////////////////// Success Remove UID From EEPROM  ///////////////////////////////////
// Flashes the blue LED 3 times to indicate a success delete to EEPROM
void successDelete()
{
	digitalWrite(blueLed, LED_OFF); 	// Make sure blue LED is off
	digitalWrite(redLed, LED_OFF); 	// Make sure red LED is off
	digitalWrite(greenLed, LED_OFF); 	// Make sure green LED is off
	delay(200);
	digitalWrite(blueLed, LED_ON); 	// Make sure blue LED is on
	delay(200);
	digitalWrite(blueLed, LED_OFF); 	// Make sure blue LED is off
	delay(200);
	digitalWrite(blueLed, LED_ON); 	// Make sure blue LED is on
	delay(200);
	digitalWrite(blueLed, LED_OFF); 	// Make sure blue LED is off
	delay(200);
	digitalWrite(blueLed, LED_ON); 	// Make sure blue LED is on
	delay(200);
	beep();
}

////////////////////// Check readCard IF is masterCard   ///////////////////////////////////
// Check to see if the ID passed is the master programing card
boolean isMaster(byte test[])
{
	if (checkTwo(test, masterCard))
		return true;
	else
		return false;
}

/////////////////////Servo Method///////////////////////////////////////
void servo(int datPos)
{
	if (isUsingCustomServo)
		customServo(datPos);
	else
	{
		daServo.write(datPos);
		delay(1000);// Hold door lock open for given seconds
	}
}

void beep()
{
	digitalWrite(buzzer, HIGH);
	delay(BeepInterval);
	digitalWrite(buzzer, LOW);
	delay(BeepInterval);
}

void alarm()
{
	digitalWrite(buzzer, HIGH);
	delay(AlarmInterval);
	digitalWrite(buzzer, LOW);
	delay(BeepInterval);
}

////////////////////////////////////////long beep alarm//////////////////////
void alarmTriggeredBySMS()
{
	for (int i = 0; i < 10; i++)
		alarm();
}

///////////////////////////////////////////////estimate the door is truely opened or not///////////////
bool isDoorOpened()
{
	if (isDoorStatusDetectorExist)
	{
		//用干簧管,有磁场时为低电平,需要在门旁放一磁铁,故开门后为高
		digitalWrite(DoorStatusDetectorVccPin, HIGH);//power the detector
		delay(10);//wait the detector to initiallize
		if (digitalRead(DoorStatusDetectorPin) == HIGH)
		{
			digitalWrite(DoorStatusDetectorVccPin, LOW);//power off
			return true;
		}
		else
		{
			digitalWrite(DoorStatusDetectorVccPin, LOW);//power off
			return false;
		}
	}
	else
		return true;//by default 
}

//////////////////////tool to check income message//////////////////
bool isCommandContained(const char* cmd, char* message, int length)
{
	int cmdLength = strlen(cmd);
	int k = 0;
	for (int i = 0; i < length; i++)
	{
		k = (message[i] == cmd[k]) ? k + 1 : 0;
		if (k == cmdLength)
			return true;
	}
	return false;
}

///////////////////////////////////////////send message to control the device ///////////////////////////////////
void gprsFunctions()
{
	if (isGprsExist)
	{
#ifdef DEBUG
		Serial.println("Entering GPRS function");
#endif // DEBUG

		//int index, length;
		//char phone[15], datatime[30], message[100];//datatime and message length unknown yet and to modify later
		int index, length = 500;
		char message[100] = { '\0' };
		char phone[15], datatime[30];//datatime length unknown yet modify later
		if (index = gprs.isSMSunread())
		{
			Serial.print("Unread message : ");
			Serial.println(index);
			if (gprs.readSMS(index, message, length, phone, datatime))
			{
				Serial.println("incoming Message : ");
				Serial.print("length : ");
				Serial.println(length);
				Serial.print("phone : ");
				Serial.println(phone);
				Serial.print("datatime : ");
				Serial.println(datatime);
				Serial.print("message : ");
				Serial.println(message); 
				//handle the message
				boolean isNumEqual = true;
				for (int i = 0; i < 14; i++)
					if (phone[i] != masterphone[i])
						isNumEqual = false;
				if (isNumEqual)
				{
					Serial.println("master phone call in");
					if (isCommandContained(OpenDoorCmd, message, length))
					{
						Serial.println("open door by master's command");
						granted();
						if (isDoorOpened())
						{
							Serial.println("Door opened");
							gprs.sendSMS(masterphone, "Door opened");
						}
						else
						{
							Serial.println("Door failed to open");
							gprs.sendSMS(masterphone, "Door failed to open");
						}
					}
					else if (isCommandContained(AlarmCmd, message, length))
						alarmTriggeredBySMS();
					else
					{
						Serial.println("Command error");
						char buffer[100];
						sprintf(buffer, "Invalid Command\nSend \"%s\" to open the door\nSend \"%s\" to triggle the alarm", OpenDoorCmd, AlarmCmd);
						Serial.println(buffer);
						sim900_flush_serial();
						gprs.sendSMS(masterphone, buffer);
					}
				}
			}
		}
#ifdef DEBUG
		Serial.println("GPRS function Exited");
#endif // DEBUG

	}
}

void infraredControl()
{
	if (isInfraredControlExist)
	{
#ifdef DEBUG
		Serial.println("Entering Infrared Control Function");
#endif // DEBUG
		decode_results results;
		if (irrecv.decode(&results))
		{
			if (results.value == 1319256238 || results.value == 16726215)//4EA240AE HEX (手机)  or FF38C7 (遥控器)
			{
				Serial.println("door opened by remote control");
				granted();
			}
			if (results.value == 16756815)//   "#"
			{
				alarm();
			}
			irrecv.resume(); // Receive the next value
		}
#ifdef DEBUG
		Serial.println("Infrared Control Function Exited");
#endif // DEBUG
	}
}

//total use 1s ! ! !
void customServo(int angle)
{
	for (int i = 0; i < 50; i++)
	{
		int pulsewidth = map(angle, 0, 180, 500, 2480);
		digitalWrite(servoPin, HIGH); //将舵机接口电平至高
		delayMicroseconds(pulsewidth);//延时脉宽值的微秒数
		digitalWrite(servoPin, LOW); //将舵机接口电平至低
		delayMicroseconds(20000 - pulsewidth);

	}
}




