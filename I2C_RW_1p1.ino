/*
I2C Writer
2023 Dylanger Gutierrez

V1.0: Initial release
V1.1: Scans for active I2C addresses on Startup

This code takes Serial input and translates to I2C Read/Write commands
To use: 
  - find your SDA-SCL ports on your Arduino (A4 and A5 on Arduino Nano, respectively)
  - hook them up to I2C bus
  - make sure you have 2k pull-up resistors to supply on these rails
  
*/

#include <Wire.h>


byte TSID=0;  //Target 7-bit SID
byte TADR=0;  //Target 8-bit Device address
byte ACMD=0;  //Target Reg Address
byte WDAT=0;  //Data to write to Target
byte RDAT=0;  //Data read back from Target
char hexsid[2];   //2-char buffer for text handling
char tadr[2];
char acmd[2];
char wdat[2];

int error=0; //Error code from I2C

char readchar=0;
byte nhexchars=0;

void setup() {
  //Initialize I2C
  Wire.begin(); //Start as a controller device
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  delay(500); //to stop garbage data from dumping to serial
  //Print controller master ID
  Serial.println("I2C initialized");
  Serial.println("Read format:  \"R XX YY\"; XX=7-bit SID (00-7F), YY=8-bit CMD Address");
  Serial.println("Write format: \"W XX YY DD\"; XX=7-bit SID (00-7F), DD = byte of data");
  i2cscan();
  delay(200);
}

void loop() {
  //Read Serial Data and store as buffer
  while(!Serial.available()); //Monitor Serial Input for control characters
  char next = Serial.read();
  switch(next) {
    case 'w':  //Format for R line: "R xx yy", where xx=TSID and yy=ACMD
    case 'W':
      nhexchars=0;
      while (nhexchars<2) {   //this loop gets 2 hex chars and saves to hexsid
        while(!Serial.available());
        readchar = Serial.read();
        if(readchar=='\n') {    //New line means give up
          Serial.println("Write Syntax Error");
          break;
        }
        if((readchar>='0'&&readchar<='9')||(readchar>='A'&&readchar<='F')||(readchar>='a'&&readchar<='f')) {  //Hex Check
          hexsid[1-nhexchars]=readchar;
          nhexchars++;
        }    
      }
      if(readchar=='\n') {    //New line means give up
          break;
      }
      TSID = getHexByte(hexsid); //covert from 2-char hex string to integer
      TADR = TSID<<1; //TADR<7:1>=TSID
      nhexchars=0; //reset
      while (nhexchars<2) {   //this loop gets 2 hex chars and saves to acmd
        while(!Serial.available());
        readchar = Serial.read();
        if(readchar=='\n') {    //New line means give up
          Serial.println("Write Syntax Error");
          break;
        }
        if((readchar>='0'&&readchar<='9')||(readchar>='A'&&readchar<='F')||(readchar>='a'&&readchar<='f')) {  //Hex Check
          acmd[1-nhexchars]=readchar;
          nhexchars++;
        }    
      }
      if(readchar=='\n') {    //New line means give up
          break;
      }
      ACMD = getHexByte(acmd);
      nhexchars=0; //reset
      while (nhexchars<2) {   //this loop gets 2 hex chars and saves to acmd
        while(!Serial.available());
        readchar = Serial.read();
        if(readchar=='\n') {    //New line means give up
          Serial.println("Write Syntax Error");
          break;
        }
        if((readchar>='0'&&readchar<='9')||(readchar>='A'&&readchar<='F')||(readchar>='a'&&readchar<='f')) {  //Hex Check
          wdat[1-nhexchars]=readchar;
          nhexchars++;
        }    
      }
      if(readchar=='\n') {    //New line means give up
          break;
      }
      WDAT = getHexByte(wdat); 

      Wire.beginTransmission(TSID);   //Actual write command
      Wire.write(ACMD);
      Wire.write(WDAT);
      error = Wire.endTransmission();
      if(error) {
        Serial.print("Write error: code ");
        Serial.println(error,DEC);
      } else {
        Serial.print("Wrote: ");
        if(TADR<16){Serial.print('0');};    //This is the dumb way of making sure these bytes are always read back as 2 chars wide
        Serial.print(TADR,HEX);
        Serial.print(" ");
        if(ACMD<16){Serial.print('0');};
        Serial.print(ACMD,HEX);
        Serial.print(" ");
        if(WDAT<16){Serial.print('0');};
        Serial.println(WDAT,HEX);
      }
             //Returns "Read: aa yy = zz", where aa = SID<7:1>,1 and zz=RDAT 
      break;
    case 'r':
    case 'R':
     nhexchars=0;
      while (nhexchars<2) {   //this loop gets 2 hex chars and saves to hexsid
        while(!Serial.available());
        readchar = Serial.read();
        if(readchar=='\n') {    //New line means give up
          Serial.println("Read Syntax Error");
          break;
        }
        if((readchar>='0'&&readchar<='9')||(readchar>='A'&&readchar<='F')||(readchar>='a'&&readchar<='f')) {  //Hex Check
          hexsid[1-nhexchars]=readchar;
          nhexchars++;
        }    
      }
      if(readchar=='\n') {    //New line means give up
          break;
      }
      TSID = getHexByte(hexsid);
      TADR = TSID<<1; //TADR<7:1>=TSID
      nhexchars=0; //reset
      while (nhexchars<2) {   //this loop gets 2 hex chars and saves to acmd
        while(!Serial.available());
        readchar = Serial.read();
        if(readchar=='\n') {    //New line means give up; exit hex acquisition loop
          Serial.println("Read Syntax Error");
          break;
        }
        if((readchar>='0'&&readchar<='9')||(readchar>='A'&&readchar<='F')||(readchar>='a'&&readchar<='f')) {  //Hex Check
          acmd[1-nhexchars]=readchar;
          nhexchars++;
        }    
      }
      if(readchar=='\n') {    //New line means give up; exit this case
          break;
      }
      ACMD = getHexByte(acmd);
      Wire.beginTransmission(TSID);   //Actual read command; first start by pointing to the device
      Wire.write(ACMD);               //then the register we want to read
      Wire.endTransmission();         //End write mode
      Wire.requestFrom(TSID,1);       //request a single byte
      if(Wire.available()) {          //check if the byte is available
        RDAT = Wire.read();               //Save byte
        Serial.print("Read: SID ");
        if(TSID<16){Serial.print('0');};  
        Serial.print(TSID,HEX);
        Serial.print(" CMD ");
        if(ACMD<16){Serial.print('0');};
        Serial.print(ACMD,HEX);
        Serial.print(" = ");
        if(RDAT<16){Serial.print('0');};
        Serial.println(RDAT,HEX);         //repeat byte contents to serial
      } else {  
        Serial.print("ERROR: SID ");      //This is shown when device does not allow read
        if(TSID<16){Serial.print('0');};
        Serial.print(TSID,HEX);
        Serial.print(" CMD ");
        if(ACMD<16){Serial.print('0');};
        Serial.print(ACMD,HEX);
        Serial.println(" is unavailable.");
      }
      break;
  }
}

int getHexByte(char* hexchars) { //takes 2-char hex string and returns hex number 0-255
  byte outbyte=0; //Byte to return
  for(int i=0;i<=1;i++){ //for both bytes
    if(hexchars[i]>='0' && hexchars[i]<='9') {        //If char is b/n 0-9
        outbyte += (hexchars[i]-'0')<<(4*i);          //calculates digit value and saves in byte
    } else if(hexchars[i]>='A' && hexchars[i]<='F') { //If char is uppercase A-F
        outbyte += (hexchars[i]-'A'+10)<<(4*i);       //calculates digit value and saves in byte
    } else if(hexchars[i]>='a' && hexchars[i]<='f') { //If char is lowercase a-f
        outbyte += (hexchars[i]-'a'+10)<<(4*i);       //calculates digit value and saves in byte
    } else {                                          //if char is not 0-9, A-F, or a-f
      return -1;                                      //error code non-hex
    }
  }
  return (int)outbyte;
}

void i2cscan() //From i2cscanner: https://playground.arduino.cc/Main/I2cScanner/
{
  byte error, address;
  int nDevices;
 
  Serial.println("Scanning for active I2C SIDs...");
 
  nDevices = 0;
  for(address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
 
    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
      nDevices++;
    }
    else if (error==4)
    {
      Serial.print("Unknown error at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  } else {
    Serial.print(nDevices);
    Serial.print(" active ");
    if(nDevices == 1) {
      Serial.print("device "); //singular
    } else {
      Serial.print("devices "); //plural
    }
    Serial.println("Found. \n");
    }
}
