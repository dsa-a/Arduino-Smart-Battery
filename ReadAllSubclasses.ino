  
/*
 * ReadAllSubclasses.ino (https://github.com/dsa-a/Arduino-Smart-Battery)
 * Copyright (C) 2021, Andrei Egorov
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <Wire.h>                 //!!! The WIRE library must be edited to increase the buffer to 34 (0x22) bytes !!!
                                  //!!! The standard Arduino Wire library needs to be changed in two places - Wire.h and utility/twi.h !!!

//#define debug

#define addr 0x0B

byte buff[34];
byte sp;

byte PEC(byte p, byte b) {
  b^=p;
  for (byte i=0; i<8; i++ ) {
    byte t=b&0x80;
    b<<=1;
    if (t!=0) b^=0x07;
  }
  return b;
}

void CheckWireStatus(byte wire_status) {
  if (wire_status!=0) {
    Serial.print(F("Wire error - "));
    Serial.println(wire_status);
    while (true) ;
  }
}

void SMBCommand(byte comm) {
  Wire.beginTransmission(addr);
  Wire.write(comm);
  CheckWireStatus(Wire.endTransmission(false));
}

void Read(byte n) {
  Wire.requestFrom(addr,n+1);
  byte p=PEC(sp, (addr<<1)+1);
  byte b=Wire.available();
  for (byte i=0; i<b; i++) {
    buff[i]=Wire.read();
    if (i<(b-1)) p=PEC(p, buff[i]);
#if defined (debug)
    printHEX(buff[i]);
    Serial.print(",");
#endif
  }
#if defined (debug)
  printHEX(p);
  Serial.println();
#endif
  if (p!=buff[b-1]) {
    Serial.println();
    Serial.println(F("PEC Error !!!"));
    printHEX(p); Serial.print("!="); printHEX(buff[b-1]);
    while (true) ;
  }
}

void ReadSMB(byte comm) {
  SMBCommand(comm);
  sp=PEC(PEC(0, addr<<1), comm);
  Read(2);
}

void ReadSMB(word comm) {
  Wire.beginTransmission(addr);
  Wire.write(00);
  Wire.write(lowByte(comm));
  Wire.write(highByte(comm));
  sp=PEC(PEC(PEC(0, addr<<1), lowByte(comm)),highByte(comm));
  CheckWireStatus(Wire.endTransmission());
  ReadSMB(byte(0x00));
}

void ReadBlockSMB(byte comm) {
  Wire.beginTransmission(addr);
  Wire.write(comm);
  if (Wire.endTransmission(false)==0) {
    Wire.requestFrom(addr,1);
    byte b=Wire.read();
    SMBCommand(comm);
    sp=PEC(PEC(0, addr<<1), comm);
    Read(b+1);
  }
}

void WriteSMBWord(byte comm, word data) {
  Wire.beginTransmission(addr);
  Wire.write(comm);
  Wire.write(lowByte(data));
  Wire.write(highByte(data));
  CheckWireStatus(Wire.endTransmission());
}

void ReadSMBSubclass(byte id, byte page) {
  WriteSMBWord(0x77, word(id));
  delay(100);
  ReadBlockSMB(page);
}

void printHEX(byte b) {
  if (b<16) Serial.print("0");
  Serial.print(b,HEX);
}

void printBlock() {
  for (byte i=1; i<=buff[0]; i++) Serial.print(char(buff[i]));
  Serial.println();
}

void setup() {
  Wire.begin();    
  Serial.begin(9600);
  Serial.println(F("Arduino Smart Battery"));
  Serial.println(F("Read All subclasses"));
  Serial.println(F("Press Enter..."));
  while (Serial.available()==0);
  Serial.print(F("Checking communication with the device at address 0x"));
  printHEX(addr);
  Serial.println("...");
  byte st;
  do {
    Wire.beginTransmission(addr);
    st=Wire.endTransmission();
    if (st!=0) Serial.println(F("The device is not responding.")); delay(1000);
  } while (st!=0);
  Serial.println(F("The device was found !!!"));
  Serial.println(F("--------------------------"));
  ReadBlockSMB(0x20); Serial.print(F("ManufName: ")); printBlock();
  ReadBlockSMB(0x21); Serial.print(F("DeviceName: ")); printBlock();
  ReadSMB(byte(0x1C)); Serial.print(F("SerialNumber: ")); printHEX(buff[1]); printHEX(buff[0]); Serial.println(" Hex");
  ReadSMB(word(0x0001)); Serial.print(F("Device Type: ")); printHEX(buff[1]); printHEX(buff[0]); Serial.println(" Hex");
  ReadSMB(word(0x0002)); Serial.print(F("Firmware Version: ")); printHEX(buff[1]); printHEX(buff[0]); Serial.println(" Hex");
  ReadSMB(word(0x0003)); Serial.print(F("Hardware Version: ")); printHEX(buff[1]); printHEX(buff[0]); Serial.println(" Hex");
  Serial.println(F("--------------------------"));
  for (byte sc=0; sc<=110; sc++){
    boolean found=false;
    byte page=0x78;
    do {
      memset(buff,0,sizeof(buff));
      ReadSMBSubclass(sc, page);
      if (buff[0]!=0) {
        if (!found) {
          found=true;
          Serial.print(F("Subclass "));
          Serial.println(sc);
        }
        for (byte i=0; i<=buff[0]; i++){
          printHEX(buff[i]);
          Serial.print(" ");
        }
        if (found) Serial.println();
      }
      page++;
    } while ((page<=0x7f)&&(buff[0]==0x20));
  }
  Serial.println(F("Done."));
}

void loop() {
}
