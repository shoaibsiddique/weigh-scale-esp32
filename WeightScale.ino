//#include <HX711_ADC.h>

#include "HX711.h"
#include "BluetoothSerial.h"
#include <Ticker.h>
String Number1 = "+123456789012";
String Number2 = "+123456789012";
String Number3 = "+123456789012";
String Number4 = "+123456789012";


HX711 scale;
Ticker myTimer;
//uint8_t dataPin = 16;
uint8_t dataPin = 2;
uint8_t clockPin = 4;

hw_timer_t *timer = NULL;
int timerInterval = 1000000; // 1 second interval in microseconds
int mint=0;
int sec=0;
int sms_counter=0;
float currentweight=0;
float send_weight =0;
bool sending_SMS = false;
bool weightStatus = false;

//#define USE_PIN // Uncomment this to use PIN during pairing. The pin is specified on the line below
const char *pin = "1234"; // Change this to more secure PIN.

String device_name = "ESP32-BT-Slave2";
/*
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run make menuconfig to and enable it
#endif

#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif
*/
BluetoothSerial SerialBT;
/*

*/

void addPhoneNumber(String input) {
	SerialBT.println(input);
  // Check if the command starts with "add " and the length is correct
  if (input.startsWith("add ")) {
    // Extract the phone number
    String newPhoneNumber = input.substring(4);

    // Check if the extracted string contains only digits
    if (newPhoneNumber.toInt() != 0 || newPhoneNumber.startsWith("0")) {
      // Update the phoneNumber variable
      Number1 =  newPhoneNumber;
      
      // Send a response over Bluetooth indicating success
      SerialBT.println("Phone number added: " + Number1);
    } else {
      SerialBT.println("Invalid phone number format");
    }
  } 
  if(input == "info"){
	SerialBT.println(Number1); 
  }
}

void processBluetoothInput() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
	
    addPhoneNumber(input);
  }
}



void test_sim800_module()
{
  Serial2.println("AT");
  updateSerial();
  Serial.println();
  Serial2.println("AT+CSQ");
  updateSerial();
  Serial2.println("AT+CCID");
  updateSerial();
  Serial2.println("AT+CREG?");
  updateSerial();
  Serial2.println("ATI");
  updateSerial();
  Serial2.println("AT+CBC");
  updateSerial();
}

void updateSerial()
{
  delay(500);

  while (Serial2.available())
  {
    Serial.write(Serial2.read());//Forward what Software Serial received to Serial Port
  }
}

// Function to determine the message based on weight range
String getMessageForWeight(float weight) {
  if (weight >= 17) {
    return "Cylinder is Full.";
  } else if (weight >= 12 && weight <17) {
    return "Cylinder is Three Quarters";
  } else if (weight >= 7 && weight <12) {
    return "Cylinder is Half";
  } else if (weight >= 2 && weight <7) {
    return "Cylinder is One Quarter";
  } else if (weight >= 0.5 && weight <2) {
    return "Cylinder is Empty";
  } else {
    return "No Cylinder";
  }
}

void send_SMS(const String& phoneNumber1, const String& phoneNumber2, const String& phoneNumber3, const String& phoneNumber4) {
  Serial2.println("AT+CMGF=1"); // Configuring TEXT mode
  updateSerial();
  delay(500);

  // Send message to phoneNumber1
  sendSMS(phoneNumber1);
  
  delay(5000);

  // Send message to phoneNumber2
  sendSMS(phoneNumber2);
  
  delay(5000);

  // Send message to phoneNumber3
  sendSMS(phoneNumber3);
  
  delay(5000);

  // Send message to phoneNumber4
  sendSMS(phoneNumber4);
  delay(5000);
  
}

void sendSMS(const String& phoneNumber) {
  Serial2.print("AT+CMGS=\"");
  Serial2.print(phoneNumber);
  Serial2.println("\"");
  updateSerial();
  delay(500);
  
  //Serial2.print("SMS Counter is: ");
  //Serial2.println(sms_counter);
  String message = getMessageForWeight(send_weight);
  Serial2.println(message);
  
  Serial2.print("(");
  Serial2.print(send_weight);
  Serial2.println(")");
  
  updateSerial();
  delay(500);
  Serial2.write(26);
  delay(1500);
}



void setup()
{
  SerialBT.println("START");
  
  Serial.begin(115200);
  Serial2.begin(9600);
  scale.begin(dataPin, clockPin);
  scale.set_offset(100367);
  // TODO find a nice solution for this calibration..
  // load cell factor 20 KG
  // scale.set_scale(127.15);
  // load cell factor 5 KG
  scale.set_scale(22.167599);       // TODO you need to calibrate this yourself.
  // reset the scale to zero = 0
  scale.tare(20);
  SerialBT.begin(device_name); //Bluetooth device name
   
  delay(5000);
  test_sim800_module();
  delay(5000);
  myTimer.attach(5, timerCallback);  // 5 seconds interval
  SerialBT.println("Setup Initiated");
}


void loop()
{
 if (SerialBT.available()) {
	String command = SerialBT.readStringUntil('\n'); // Assuming messages end with a newline character
	command.trim();
	addPhoneNumber(command);
	
	if (command == "info") {
		SerialBT.println(Number1);
	}
	
	if(command == "query"){
		SerialBT.print("Queried Weight is: ");
		SerialBT.print(currentweight);
		SerialBT.print(" Kg.");
	}
	
}
}

void timerCallback() {
  sec++;
  if (sec>=12)
	{
		mint++;
		sec=0;  
	}
 
  if (scale.is_ready())
  {
	if(sending_SMS == false){
		currentweight=(scale.get_units(1))/1000;
	}
    if(currentweight>0.02)
    {
		SerialBT.print("Weight of Object is: ");
		SerialBT.print(currentweight);
		SerialBT.println("Kg");
		send_weight = currentweight;
		
		
		
    if (((mint>=2)||(sms_counter==0) || (send_weight>0.5 && send_weight<2.00) ) && sending_SMS == false)
    {
		sending_SMS = true;
		SerialBT.println("Sending SMS");  
		send_SMS(Number1, Number2, Number3, Number4);
		sms_counter++;
		mint=0;
		sending_SMS = false;
      }
     }
    else{
      SerialBT.println("Weight of Object is: 0Kg");
    }
	 //SerialBT.print("Send SMS is: ");
	 // SerialBT.println(sending_SMS);
  }
}



