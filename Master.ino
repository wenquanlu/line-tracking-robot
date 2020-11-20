
//MASTER

#include <SoftwareSerial.h>   // Software Serial Port

#define RxD 7
#define TxD 6
#define ConnStatus A1         // Connection status on the SeeedStudio v1 shield is available on pin A1
                              // See also ConnStatusSupported boolean below
#define DEBUG_ENABLED  1

// ##################################################################################
// ### EDIT THE LINES BELOW TO MATCH YOUR SHIELD NUMBER AND CONNECTION PIN OPTION ###
// ##################################################################################

int shieldPairNumber = 29;
// CAUTION: If ConnStatusSupported = true you MUST NOT use pin A1 otherwise "random" reboots will occur
// CAUTION: If ConnStatusSupported = true you MUST set the PIO[1] switch to A1 (not NC)

boolean ConnStatusSupported = true;   // Set to "true" when digital connection status is available on Arduino pin

// #######################################################

// The following four string variable are used to simplify adaptation of code to different shield pairs

String slaveName = "Slave";                  // This is concatenated with shieldPairNumber later
String masterNameCmd = "\r\n+STNA=Master";   // This is concatenated with shieldPairNumber later
String connectCmd = "\r\n+CONN=";            // This is concatenated with slaveAddr later

int nameIndex = 0;
int addrIndex = 0;

String recvBuf;
String slaveAddr;
String retSymb = "+RTINQ=";   // Start symble when there's any return

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

    if(ConnStatusSupported) Serial.println("Checking Master-Slave connection status.");

    if(ConnStatusSupported && digitalRead(ConnStatus)==1)
    {
        Serial.println("Already connected to Slave - remove USB cable if reboot of Master Bluetooth required.");
    }
    else
    {
        Serial.println("Not connected to Slave.");
        
        setupBlueToothConnection();     // Set up the local (master) Bluetooth module
        getSlaveAddress();              // Search for (MAC) address of slave
        makeBlueToothConnection();      // Execute the connection to the slave

        delay(1000);                    // Wait one second and flush the serial buffers
        Serial.flush();
        blueToothSerial.flush();
    }
}

void loop()
    {
    char stateleft = 'a';
    char stateup = 'w';
    char stateright = 'd';
    char statedown = 's';
    char change = 'c';
    char replicate = 'p';
    char val = 'o';
    //a: left, s: down, d: right, w: up, o: stop, c: change
    //p: replicate
    if (Serial.available()) 
    { // If data is available to read,
        val = Serial.read();// read it and store it in val
    }
    if (blueToothSerial.available()) 
    {
        char message = blueToothSerial.read();
        Serial.print(message);
    }
    if (val == 'w') {
        blueToothSerial.print(stateup);
    } else if (val == 'a') {
        blueToothSerial.print(stateleft);
    } else if (val == 's') {
        blueToothSerial.print(statedown);
    } else if (val == 'd') {
        blueToothSerial.print(stateright);
    } else if (val == 'c') {
        blueToothSerial.print(change);
    } else if (val == 'p') {
        blueToothSerial.print(replicate);
    } else if (val == 'o') {
    }
    delay(100);
}

void setupBlueToothConnection()
{
    Serial.println("Setting up the local (master) Bluetooth module.");

    masterNameCmd += shieldPairNumber;
    masterNameCmd += "\r\n";
 
    blueToothSerial.print("\r\n+STWMOD=1\r\n");      // Set the Bluetooth to work in master mode
    blueToothSerial.print(masterNameCmd);            // Set the bluetooth name using masterNameCmd
    blueToothSerial.print("\r\n+STAUTO=0\r\n");      // Auto-connection is forbidden here

    //  print() sets up a transmit/outgoing buffer for the string which is then transmitted via interrupts one character at a time.
    //  This allows the program to keep running, with the transmitting happening in the background.
    //  Serial.flush() does not empty this buffer, instead it pauses the program until all Serial.print()ing is done.
    //  This is useful if there is critical timing mixed in with Serial.print()s.
    //  To clear an "incoming" serial buffer, use while(Serial.available()){Serial.read();}

    blueToothSerial.flush();
    delay(2000);                                     // This delay is required

    blueToothSerial.print("\r\n+INQ=1\r\n");         // Make the master Bluetooth inquire
    
    blueToothSerial.flush();
    delay(2000);                                     // This delay is required
    
    Serial.println("Master is inquiring!");
}

void getSlaveAddress()
{
    slaveName += shieldPairNumber;
    
    Serial.print("Searching for address of slave: ");
    Serial.println(slaveName);

    slaveName = ";" + slaveName;   // The ';' must be included for the search that follows
    
    char recvChar;

    // Initially, if(blueToothSerial.available()) will loop and, character-by-character, fill recvBuf to be:
    //    +STWMOD=1 followed by a blank line
    //    +STNA=MasterTest (followed by a blank line)
    //    +S
    //    OK (followed by a blank line)
    //    OK (followed by a blank line)
    //    OK (followed by a blank line)
    //    WORK:
    //
    // It will then, character-by-character, add the result of the first device that responds to the +INQ request:
    //    +RTINQ=64,A2,F9,xx,xx,xx;OnePlus 6 (xx substituted for anonymity)
    //
    // If this does not contain slaveName, the loop will continue. If nothing else responds to the +INQ request, the
    // process will appear to have frozen BUT IT HAS NOT. Be patient. Ask yourself why your slave has not been detected.
    // Eventually your slave will respond and the loop will add:
    //    +RTINQ=0,6A,8E,16,C4,1B;SlaveTest
    //
    // nameIndex will identify "SlaveTest", return a non -1 value and progress to the if() statement.
    // This will strip 0,6A,8E,16,C4,1B from the buffer, assign it to slaveAddr, and break from the loop. 
    
    while(1)
    {
        if(blueToothSerial.available())
        {  
            recvChar = blueToothSerial.read();
            recvBuf += recvChar;
                        
            nameIndex = recvBuf.indexOf(slaveName);   // Get the position of slave name
            
            if ( nameIndex != -1 )   // ie. if slaveName was found
            {
                addrIndex = (recvBuf.indexOf(retSymb,(nameIndex - retSymb.length()- 18) ) + retSymb.length());   // Get the start position of slave address
                slaveAddr = recvBuf.substring(addrIndex, nameIndex);   // Get the string of slave address

                Serial.print("Slave address found: ");
                Serial.println(slaveAddr);
                
                break;  // Only breaks from while loop if slaveName is found
            }
        }
    }
}

void makeBlueToothConnection()
{
    Serial.println("Initiating connection with slave.");

    char recvChar;

    // Having found the target slave address, now form the full connection command
    
    connectCmd += slaveAddr;
    connectCmd += "\r\n";
  
    int connectOK = 0;       // Flag used to indicate succesful connection
    int connectAttempt = 0;  // Counter to track number of connect attempts

    // Keep trying to connect to the slave until it is connected (using a do-while loop)

    do
    {
        Serial.print("Connect attempt: ");
        Serial.println(++connectAttempt);
        
        blueToothSerial.print(connectCmd);   // Send connection command

        // Initially, if(blueToothSerial.available()) will loop and, character-by-character, fill recvBuf to be:
        //    OK (followed by a blank line)
        //    +BTSTATE:3 (followed by a blank line)(BTSTATE:3 = Connecting)
        //
        // It will then, character-by-character, add the result of the connection request.
        // If that result is "CONNECT:OK", the while() loop will break and the do() loop will exit.
        // If that result is "CONNECT:FAIL", the while() loop will break with an appropriate "FAIL" message
        // and a new connection command will be issued for the same slave address.

        recvBuf = "";
        
        while(1)
        {
            if(blueToothSerial.available())
            {
                recvChar = blueToothSerial.read();
                recvBuf += recvChar;
        
                if(recvBuf.indexOf("CONNECT:OK") != -1)
                {
                    connectOK = 1;
                    Serial.println("Connected to slave!");
                    blueToothSerial.print("Master-Slave connection established!");
                    break;
                }
                else if(recvBuf.indexOf("CONNECT:FAIL") != -1)
                {
                    Serial.println("Connection FAIL, try again!");
                    break;
                }
            }
        }
    } while (0 == connectOK);

}
