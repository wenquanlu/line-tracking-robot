
//slave
#include <SoftwareSerial.h>   //Software Serial Port
#include <Servo.h>

Servo servoLeft;
Servo servoRight;
Servo servoMiddle;
#define RxD 7
#define TxD 6
#define ConnStatus A1

#define DEBUG_ENABLED  1

// ##################################################################################
// ### EDIT THE LINES BELOW TO MATCH YOUR SHIELD NUMBER AND CONNECTION PIN OPTION ###
// ##################################################################################
int lastStep; //last step, forward 0, backward 1, left 2, right 3
int shieldPairNumber = 29;
int status = 0; //0 for manual, 1 for automation 
int maxLength = 0;
// CAUTION: If ConnStatusSupported = true you MUST NOT use pin A1 otherwise "random" reboots will occur
// CAUTION: If ConnStatusSupported = true you MUST set the PIO[1] switch to A1 (not NC)

char data[800];
int pointer = 0;
boolean ConnStatusSupported = true;   // Set to "true" when digital connection status is available on Arduino pin
int found = 0;
// #######################################################

// The following two string variable are used to simplify adaptation of code to different shield pairs

String slaveNameCmd = "\r\n+STNA=Slave";   // This is concatenated with shieldPairNumber later

SoftwareSerial blueToothSerial(RxD,TxD);

void setup()
{
    Serial.begin(9600);
    blueToothSerial.begin(38400);                    // Set Bluetooth module to default baud rate 38400
    
    pinMode(RxD, INPUT);
    pinMode(TxD, OUTPUT);
    pinMode(ConnStatus, INPUT);

    //  Check whether Master and Slave are already connected by polling the ConnStatus pin (A1 on SeeedStudio v1 shield)
    //  This prevents running the full connection setup routine if not necessary.

    if(ConnStatusSupported) Serial.println("Checking Slave-Master connection status.");

    if(ConnStatusSupported && digitalRead(ConnStatus)==1)
    {
        Serial.println("Already connected to Master - remove USB cable if reboot of Master Bluetooth required.");
    }
    else
    {
        Serial.println("Not connected to Master.");
        
        setupBlueToothConnection();   // Set up the local (slave) Bluetooth module

        delay(1000);                  // Wait one second and flush the serial buffers
        Serial.flush();
        blueToothSerial.flush();
    }
    pinMode(10,INPUT);
    pinMode(9,OUTPUT);
    pinMode(3,INPUT);
    pinMode(2,OUTPUT);
    servoLeft.attach(13); 
    servoRight.attach(12);
}

void loop()
{
  servoLeft.attach(13); 
  servoRight.attach(12);
  char recvChar = 'o';
    if (status == 0)
    {
        
        if(blueToothSerial.available())
        {
            recvChar = blueToothSerial.read();
            Serial.print(recvChar);
        }
    
        if (recvChar == 'a')
        {
            servoLeft.writeMicroseconds(1500); //turn left 
            servoRight.writeMicroseconds(1500);
            data[pointer] = recvChar;
            delay(100);
        } else if (recvChar == 'w')
        {
            servoLeft.writeMicroseconds(1500); //forward
            servoRight.writeMicroseconds(1900);
            data[pointer] = recvChar;
            delay(100);
        } else if (recvChar == 'd') 
        {
            Serial.println("haliluya");
            servoLeft.writeMicroseconds(1900); //turn right
            servoRight.writeMicroseconds(1900);
            data[pointer] = recvChar;
            delay(100);
        } else if (recvChar == 's') 
        {
            servoLeft.writeMicroseconds(1900); //backward
            servoRight.writeMicroseconds(1500);
            data[pointer] = recvChar;
            delay(100);
        } else if (recvChar == 'c') //shift mode
        {
            data[pointer] = recvChar;
            status = 1;
        } else if (recvChar == 'p') //replicate
        {
            status = 2;
            pointer = 0;        
        } else {
            servoLeft.writeMicroseconds(1700); //stop
            servoRight.writeMicroseconds(1700);
            delay(100);
            data[pointer] = recvChar;
        }
        pointer ++;
    
    } else if (status == 1) 
    {
        int irLeft = irDetect(9, 10, 38000);      
        int irRight = irDetect(2,3, 38000);
        int irMiddle = irDetect(7,8,38000);
        
        Serial.println(irLeft == 0 && irRight == 0);
        if (irLeft == 0 && irRight == 0) {
            servoLeft.writeMicroseconds(1500);  //when both sides are white, all good
            servoRight.writeMicroseconds(1900);
            delay(100);
            data[pointer] = recvChar;
        } else if (irLeft == 1 && irRight == 0) { //left side is black
            servoLeft.writeMicroseconds(1500); //turn left 
            servoRight.writeMicroseconds(1500);
            delay(100);
            data[pointer] = recvChar;
        } else if (irLeft == 0 && irRight == 1) { 
            servoLeft.writeMicroseconds(1900); //turn right
            servoRight.writeMicroseconds(1900);
            delay(100);
            data[pointer] = recvChar;
        } else if (irLeft == 1 && irRight == 1) {
            servoLeft.writeMicroseconds(1700); 
            servoRight.writeMicroseconds(1700);
            delay(100);
            data[pointer] = recvChar;
            maxLength = pointer;
            blueToothSerial.println("has already found the object");
            delay(1000);
            status = 0;
        }
        pointer ++;

    } else if (status == 2) {
        while (pointer < maxLength) {
            char recvChar = data[pointer];
            if (recvChar == 'a')
                {
                    servoLeft.writeMicroseconds(1500); //turn left 
                    servoRight.writeMicroseconds(1500);
                    delay(100);
                } else if (recvChar == 'w')
                {
                    servoLeft.writeMicroseconds(1500); //forward
                    servoRight.writeMicroseconds(1900);
                    delay(100);
                } else if (recvChar == 'd') 
                {
                    servoLeft.writeMicroseconds(1900); //turn right
                    servoRight.writeMicroseconds(1900);
                    delay(100);
                } else if (recvChar == 's') 
                {
                    servoLeft.writeMicroseconds(1900); //backward
                    servoRight.writeMicroseconds(1500);
                    delay(100);
                } else {
                    servoLeft.writeMicroseconds(1700); 
                    servoRight.writeMicroseconds(1700);
                    delay(100);
                }
             pointer++;
        }
        status = 0;
    }
}
int irDetect(int irLedPin, int irReceiverPin, long frequency)
{
  tone(irLedPin, frequency, 8);
  delay(1);                                  // Wait 1 ms
  int ir = digitalRead(irReceiverPin);       // IR receiver -> ir variable
  delay(1);                                  // Down time before recheck
  return ir;                                 // Return 1 no detect, 0 detect
}
void setupBlueToothConnection()
{
    Serial.println("Setting up the local (slave) Bluetooth module.");

    slaveNameCmd += shieldPairNumber;
    slaveNameCmd += "\r\n";

    blueToothSerial.print("\r\n+STWMOD=0\r\n");      // Set the Bluetooth to work in slave mode
    blueToothSerial.print(slaveNameCmd);             // Set the Bluetooth name using slaveNameCmd
    blueToothSerial.print("\r\n+STAUTO=0\r\n");      // Auto-connection should be forbidden here
    blueToothSerial.print("\r\n+STOAUT=1\r\n");      // Permit paired device to connect me
    
    //  print() sets up a transmit/outgoing buffer for the string which is then transmitted via interrupts one character at a time.
    //  This allows the program to keep running, with the transmitting happening in the background.
    //  Serial.flush() does not empty this buffer, instead it pauses the program until all Serial.print()ing is done.
    //  This is useful if there is critical timing mixed in with Serial.print()s.
    //  To clear an "incoming" serial buffer, use while(Serial.available()){Serial.read();}

    blueToothSerial.flush();
    delay(2000);                                     // This delay is required

    blueToothSerial.print("\r\n+INQ=1\r\n");         // Make the slave Bluetooth inquirable
    
    blueToothSerial.flush();
    delay(2000);                                     // This delay is required
    
    Serial.println("The slave bluetooth is inquirable!");
}
