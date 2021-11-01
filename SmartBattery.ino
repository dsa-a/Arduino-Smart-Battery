  
/*
 * SmartBattery.ino (https://github.com/dsa-a/Arduino-Smart-Battery)
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

#define new_capacity 4400

#define unseal_key_1 0x0414
#define unseal_key_2 0x3672

#define full_access_key_1 0xFFFF
#define full_access_key_2 0xFFFF

#define pf_clear_key_1 0x2673
#define pf_clear_key_2 0x1712

#define year (__DATE__[7]-'0')*1000+(__DATE__[8]-'0')*100+(__DATE__[9]-'0')*10+(__DATE__[10]-'0')
#define month (__DATE__[0] == 'J' && __DATE__[1] == 'a')  ? 1 : \
              (__DATE__[0] == 'F')                        ? 2 : \
              (__DATE__[0] == 'M' && __DATE__[2] == 'r')  ? 3 : \
              (__DATE__[0] == 'A' && __DATE__[1] == 'p')  ? 4 : \
              (__DATE__[0] == 'M' && __DATE__[2] == 'y')  ? 5 : \
              (__DATE__[0] == 'J' && __DATE__[2] == 'n')  ? 6 : \
              (__DATE__[0] == 'J' && __DATE__[2] == 'l')  ? 7 : \
              (__DATE__[0] == 'A' && __DATE__[1] == 'u')  ? 8 : \
              (__DATE__[0] == 'S')                        ? 9 : \
              (__DATE__[0] == 'O')                        ? 10 : \
              (__DATE__[0] == 'N')                        ? 11 : \
              (__DATE__[0] == 'D')                        ? 12 : 0
#define day (((__DATE__[4] >= '0') ? (__DATE__[4]) : '0')-'0')*10+(__DATE__[5]-'0')

byte buff[34];

#if defined (debug)
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
#endif

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
#if defined (debug)
  byte p=PEC(sp, (addr<<1)+1);
#endif
  byte b=Wire.available();
  for (byte i=0; i<b; i++) {
    buff[i]=Wire.read();
#if defined (debug)
    if (i<(b-1)) p=PEC(p, buff[i]);
    printHEX(buff[i]);
    Serial.print(",");
#endif
  }
#if defined (debug)
  printHEX(p);
  Serial.println();
#endif
}

void ReadSMB(byte comm) {
  SMBCommand(comm);
#if defined (debug)
  sp=PEC(PEC(0, addr<<1), comm);
#endif
  Read(2);
}

void ReadSMB(word comm) {
  Wire.beginTransmission(addr);
  Wire.write(00);
  Wire.write(lowByte(comm));
  Wire.write(highByte(comm));
  CheckWireStatus(Wire.endTransmission());
  ReadSMB(byte(0x00));
}

void ReadBlockSMB(byte comm) {
  SMBCommand(comm);
  Wire.requestFrom(addr,1);
  byte b=Wire.read();
  SMBCommand(comm);
#if defined (debug)
  sp=PEC(PEC(0, addr<<1), comm);
#endif
  Read(b+1);
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

void WriteSMBSubclass(byte id, byte page) {
  WriteSMBWord(0x77, word(id));
  delay(100);
  Wire.beginTransmission(addr);
  Wire.write(page);
  for (byte i=0;i<=buff[0];i++) Wire.write(buff[i]);
  CheckWireStatus(Wire.endTransmission());
  delay(100);
}

void printHEX(byte b) {
  if (b<16) Serial.print("0");
  Serial.print(b,HEX);
}

void printBlock() {
  for (byte i=1; i<=buff[0]; i++) Serial.print(char(buff[i]));
  Serial.println();
}

void Read123() {
  ReadSMB(word(0x0001)); Serial.print(F("Device Type: ")); printHEX(buff[1]); printHEX(buff[0]); Serial.println(" Hex");
  ReadSMB(word(0x0002)); Serial.print(F("Firmware Version: ")); printHEX(buff[1]); printHEX(buff[0]); Serial.println(" Hex");
  ReadSMB(word(0x0003)); Serial.print(F("Hardware Version: ")); printHEX(buff[1]); printHEX(buff[0]); Serial.println(" Hex");
}

void info() {
  ReadSMB(byte(0x18)); Serial.print(F("DesignCapacity: ")); Serial.print(buff[1]*256+buff[0]); Serial.println(" mAh");
  ReadSMB(byte(0x10)); Serial.print(F("FullChargeCapacity: ")); Serial.print(buff[1]*256+buff[0]); Serial.println(" mAh");
  ReadSMB(byte(0x17)); Serial.print(F("CycleCount: ")); Serial.println(buff[1]*256+buff[0]);
  ReadSMB(byte(0x1B)); Serial.print(F("Date: ")); Serial.print(1980+(buff[1]>>1)); Serial.print("."); Serial.print(((buff[1]&0b00000001)<<3)+(buff[0]>>5));Serial.print("."); Serial.println(buff[0]&0b00011111);
  ReadSMB(byte(0x19)); Serial.print(F("DesignVoltage: ")); Serial.print(buff[1]*256+buff[0]); Serial.println(" mV");
  ReadBlockSMB(0x20); Serial.print(F("ManufName: ")); printBlock();
  ReadBlockSMB(0x21); Serial.print(F("DeviceName: ")); printBlock(); 
  ReadSMB(byte(0x1C)); Serial.print(F("SerialNumber: ")); printHEX(buff[1]); printHEX(buff[0]); Serial.println(" Hex");
  ReadSMB(byte(0x14)); Serial.print(F("ChargingCurrent: ")); Serial.print(buff[1]*256+buff[0]); Serial.println(" mA");
  ReadSMB(byte(0x15)); Serial.print(F("ChargingVoltage: ")); Serial.print(buff[1]*256+buff[0]); Serial.println(" mV");
  ReadBlockSMB(0x22); Serial.print(F("DeviceChemistry: ")); printBlock(); 
  ReadSMB(byte(0x08)); Serial.print(F("Temperature: ")); Serial.print(float(buff[1]*256+buff[0])/10-273); Serial.println(" C");
  ReadSMB(byte(0x09)); Serial.print(F("Voltage: ")); Serial.print(buff[1]*256+buff[0]); Serial.println(" mV");
  ReadSMB(byte(0x0A)); Serial.print(F("Current: ")); Serial.print(int(buff[1]*256+buff[0])); Serial.println(" mA");
  ReadSMB(byte(0x0D)); Serial.print(F("RelativeSOC: ")); Serial.print(buff[0]); Serial.println(" %");
  ReadSMB(byte(0x0E)); Serial.print(F("AbsoluteSOC: ")); Serial.print(buff[0]); Serial.println(" %");
  ReadSMB(byte(0x0F)); Serial.print(F("RemainingCapacity: ")); Serial.print(buff[1]*256+buff[0]); Serial.println(" mAh");
  ReadSMB(byte(0x3C)); Serial.print(F("VCELL4: ")); Serial.print(buff[1]*256+buff[0]); Serial.println(" mV");
  ReadSMB(byte(0x3D)); Serial.print(F("VCELL3: ")); Serial.print(buff[1]*256+buff[0]); Serial.println(" mV");
  ReadSMB(byte(0x3E)); Serial.print(F("VCELL2: ")); Serial.print(buff[1]*256+buff[0]); Serial.println(" mV");
  ReadSMB(byte(0x3F)); Serial.print(F("VCELL1: ")); Serial.print(buff[1]*256+buff[0]); Serial.println(" mV");
  ReadSMB(byte(0x1A)); Serial.print(F("SpecificationInfo: ")); printHEX(buff[1]); printHEX(buff[0]); Serial.println(" Hex");
  ReadSMB(byte(0x16)); Serial.print(F("Battery Status: ")); printHEX(buff[1]); printHEX(buff[0]); Serial.println(" Hex");
  if (buff[1]&0b10000000) Serial.print("OCA|");
  if (buff[1]&0b01000000) Serial.print("TCA|");
  if (buff[1]&0b00010000) Serial.print("OTA|");
  if (buff[1]&0b00001000) Serial.print("TDA|");
  if (buff[1]&0b00000010) Serial.print("RCA|");
  if (buff[1]&0b00000001) Serial.print("RTA|");
  if (buff[0]&0b10000000) Serial.print("INIT|");
  if (buff[0]&0b01000000) Serial.print("DSG|");
  if (buff[0]&0b00100000) Serial.print("FC|");
  if (buff[0]&0b00010000) Serial.print("FD|");
  if (buff[0]&0b00001000) Serial.print("EC3|");
  if (buff[0]&0b00000100) Serial.print("EC2|");
  if (buff[0]&0b00000010) Serial.print("EC1|");
  if (buff[0]&0b00000001) Serial.print("EC0|");
  Serial.println();
  ReadSMB(word(0x0054)); Serial.print(F("Operation Status: ")); printHEX(buff[1]); printHEX(buff[0]); Serial.println(" Hex");
  if (buff[1]&0b10000000) Serial.print("PRES|");
  if (buff[1]&0b01000000) Serial.print("FAS|");
  if (buff[1]&0b00100000) Serial.print("SS|");
  if (buff[1]&0b00010000) Serial.print("CSV|");
  if (buff[1]&0b00000100) Serial.print("LDMD|");
  if (buff[0]&0b10000000) Serial.print("WAKE|");
  if (buff[0]&0b01000000) Serial.print("DSG|");
  if (buff[0]&0b00100000) Serial.print("XDSG|");
  if (buff[0]&0b00010000) Serial.print("XDSGI|");
  if (buff[0]&0b00000100) Serial.print("R_DIS|");
  if (buff[0]&0b00000010) Serial.print("VOK|");
  if (buff[0]&0b00000001) Serial.print("QEN|");
  Serial.println();
  if (buff[1]&0b00100000) {
    Serial.println(F("Sealed"));
    Read123();
  } else {
    Serial.println(F("Unsealed"));
    if (!(buff[1]&0b01000000)) {
      Serial.println(F("Pack in Full Access mode"));
      ReadBlockSMB(0x60); Serial.print(F("UnSealKey: ")); printHEX(buff[2]); printHEX(buff[1]); printHEX(buff[4]); printHEX(buff[3]); Serial.println(" Hex");
      ReadBlockSMB(0x61); Serial.print(F("FullAccessKey: ")); printHEX(buff[2]); printHEX(buff[1]); printHEX(buff[4]); printHEX(buff[3]); Serial.println(" Hex");
      ReadBlockSMB(0x62); Serial.print(F("PFKey: ")); printHEX(buff[2]); printHEX(buff[1]); printHEX(buff[4]); printHEX(buff[3]); Serial.println(" Hex");
    };
    ReadSMB(byte(0x0C)); Serial.print(F("MaxError: ")); Serial.print(buff[0]); Serial.println(" %");
    ReadSMB(word(0x0051)); Serial.print(F("SafetyStatus: ")); 
    if ((buff[1]==0)&&(buff[0]==0)) {
      Serial.println("OK");
    } else {
      printHEX(buff[1]); printHEX(buff[0]); Serial.println(" Hex");
      if (buff[1]&0b10000000) Serial.print("OTD|");
      if (buff[1]&0b01000000) Serial.print("OTC|");
      if (buff[1]&0b00100000) Serial.print("OCD|");
      if (buff[1]&0b00010000) Serial.print("OCC|");
      if (buff[1]&0b00001000) Serial.print("OCD2|");
      if (buff[1]&0b00000100) Serial.print("OCC2|");
      if (buff[1]&0b00000010) Serial.print("PUV|");
      if (buff[1]&0b00000001) Serial.print("POV|");
      if (buff[0]&0b10000000) Serial.print("CUV|");
      if (buff[0]&0b01000000) Serial.print("COV|");
      if (buff[0]&0b00100000) Serial.print("PF|");
      if (buff[0]&0b00010000) Serial.print("HWDG|");
      if (buff[0]&0b00001000) Serial.print("WDF|");
      if (buff[0]&0b00000100) Serial.print("AOCD|");
      if (buff[0]&0b00000010) Serial.print("SCC|");
      if (buff[0]&0b00000001) Serial.print("SCD|");
      Serial.println();
    };
    ReadSMB(word(0x0053)); Serial.print(F("PFStatus: ")); 
    if ((buff[1]==0)&&(buff[0]==0)) {
      Serial.println("OK");
    } else {
      printHEX(buff[1]); printHEX(buff[0]); Serial.println(" Hex");
      if (buff[1]&0b10000000) Serial.print("FBF|");
      if (buff[1]&0b00010000) Serial.print("SOPT|");
      if (buff[1]&0b00001000) Serial.print("SOCD|");
      if (buff[1]&0b00000100) Serial.print("SOCC|");
      if (buff[1]&0b00000010) Serial.print("AFE_P|");
      if (buff[1]&0b00000001) Serial.print("AFE_C|");
      if (buff[0]&0b10000000) Serial.print("DFF|");
      if (buff[0]&0b01000000) Serial.print("DFETF|");
      if (buff[0]&0b00100000) Serial.print("CFETF|");
      if (buff[0]&0b00010000) Serial.print("CIM|");
      if (buff[0]&0b00001000) Serial.print("SOTD|");
      if (buff[0]&0b00000100) Serial.print("SOTC|");
      if (buff[0]&0b00000010) Serial.print("SOV|");
      if (buff[0]&0b00000001) Serial.print("PFIN|");
      Serial.println();
    };
    ReadSMB(word(0x0055)); Serial.print(F("Charging Status: ")); 
    if ((buff[1]==0)&&(buff[0]==0)) {
      Serial.println("OK");
    } else {
      printHEX(buff[1]); printHEX(buff[0]); Serial.println(" Hex");
      if (buff[1]&0b10000000) Serial.print("XCHG|");
      if (buff[1]&0b01000000) Serial.print("CHGSUSP|");
      if (buff[1]&0b00100000) Serial.print("PCHG|");
      if (buff[1]&0b00010000) Serial.print("MCHG|");
      if (buff[1]&0b00001000) Serial.print("TCHG1|");
      if (buff[1]&0b00000100) Serial.print("TCHG2|");
      if (buff[1]&0b00000010) Serial.print("FCHG|");
      if (buff[1]&0b00000001) Serial.print("PULSE|");
      if (buff[0]&0b10000000) Serial.print("PLSOFF|");
      if (buff[0]&0b01000000) Serial.print("CB|");
      if (buff[0]&0b00100000) Serial.print("PCMTO|");
      if (buff[0]&0b00010000) Serial.print("FCMTO|");
      if (buff[0]&0b00001000) Serial.print("OCHGV|");
      if (buff[0]&0b00000100) Serial.print("OCHGI|");
      if (buff[0]&0b00000010) Serial.print("OC|");
      if (buff[0]&0b00000001) Serial.print("XCHGLV|");
      Serial.println();
    };
    ReadSMB(byte(0x46)); Serial.print(F("FETControl: ")); 
    if (buff[0]==0) {
      Serial.println("OK");
    } else {
      printHEX(buff[0]); Serial.println(" Hex");
      if (buff[0]&0b00010000) Serial.print("OD|");
      if (buff[0]&0b00001000) Serial.print("ZVCHG|");
      if (buff[0]&0b00000100) Serial.print("CHG|");
      if (buff[0]&0b00000010) Serial.print("DSG|");
      Serial.println();
    };
    ReadSMBSubclass(82,0x78);
    Serial.print(F("Update Status: ")); Serial.println(buff[13]);
    Serial.print(F("Qmax Cell0: ")); Serial.println(buff[1]*256+buff[2]);
    Serial.print(F("Qmax Cell1: ")); Serial.println(buff[3]*256+buff[4]);
    Serial.print(F("Qmax Cell2: ")); Serial.println(buff[5]*256+buff[6]);
    Serial.print(F("Qmax Cell3: ")); Serial.println(buff[7]*256+buff[8]);
    Serial.print(F("Qmax Pack : ")); Serial.println(buff[9]*256+buff[10]);
    for (byte i=88; i<=91; i++) {
      ReadSMBSubclass(i,0x78);
      Serial.print("Cell");
      Serial.print(i-88);
      Serial.print(" R_a flag: "); printHEX(buff[1]); printHEX(buff[2]); Serial.println();
    }
    Read123();
    ReadSMB(word(0x0006)); Serial.print(F("Manufacturer Status: ")); printHEX(buff[1]); printHEX(buff[0]); Serial.println(" Hex");
    if (buff[1]&0b10000000) Serial.print("FET1|");
    if (buff[1]&0b01000000) Serial.print("FET0|");
    if (buff[1]&0b00100000) Serial.print("PF1|");
    if (buff[1]&0b00010000) Serial.print("PF0|");
    if (buff[1]&0b00001000) Serial.print("STATE3|");
    if (buff[1]&0b00000100) Serial.print("STATE2|");
    if (buff[1]&0b00000010) Serial.print("STATE1|");
    if (buff[1]&0b00000001) Serial.print("STATE0|");
    Serial.println();
    ReadSMB(word(0x0008)); Serial.print(F("Chemistry ID: ")); printHEX(buff[1]); printHEX(buff[0]); Serial.println(" Hex");
    delay(100);
    ReadSMB(byte(0x03)); Serial.print(F("BatteryMode: ")); printHEX(buff[1]); printHEX(buff[0]); Serial.println(" Hex");
    if (buff[1]&0b10000000) Serial.print("CapM|");
    if (buff[1]&0b01000000) Serial.print("ChgM|");
    if (buff[1]&0b00100000) Serial.print("AM|");
    if (buff[1]&0b00000010) Serial.print("PB|");
    if (buff[1]&0b00000001) Serial.print("CC|");
    if (buff[0]&0b10000000) Serial.print("CF|");
    if (buff[0]&0b00000010) Serial.print("PBS|");
    if (buff[0]&0b00000001) Serial.print("ICC|");
    Serial.println();
  };
  
}

void setup() {
  Wire.begin();    
  Serial.begin(9600);
  Serial.println(F("Arduino Smart Battery"));
  Serial.println(F("Several utilities for working with TI bq20z... IC"));
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
}

void loop() {
  delay(1);
  if (Serial.available()==0) {
    Serial.println(F("--------------------"));
    Serial.println(F("Select operation:"));
    Serial.println(F("1. Read pack info."));
    Serial.println(F("2. Pack Reset."));
    Serial.println(F("3. Unsealing a pack."));
    Serial.println(F("4. Move pack to Full Access mode."));
    Serial.println(F("5. Clearing a Permanent Failure."));
    Serial.println(F("6. Clearing CycleCount."));
    Serial.println(F("7. Setting current date."));
    Serial.println(F("8. Writing DesignCapacity, QMAX, Update status, Ra_table."));
    Serial.println(F("9. Begin the Impedance Track algorithm."));
    while (Serial.available()==0);
    switch (Serial.read()) {
      case 0x31:
        Serial.println(F("Pack Info..."));
        info();
        break;
      case 0x32:
        WriteSMBWord(0x00,0x0041);
        Serial.println(F("Reseting..."));
        delay(1000);
        break;
      case 0x33:
        WriteSMBWord(0x00,unseal_key_1);
        WriteSMBWord(0x00,unseal_key_2);
        Serial.println(F("Unsealing..."));
        break;
      case 0x34:
        WriteSMBWord(0x00,full_access_key_1);
        WriteSMBWord(0x00,full_access_key_2);
        Serial.println(F("Move to Full Access mode..."));
        break;
      case 0x35:
        WriteSMBWord(0x00,pf_clear_key_1);
        WriteSMBWord(0x00,pf_clear_key_2);
        Serial.println(F("Clearing a Permanent Failure..."));
        break;
      case 0x36:
        WriteSMBWord(0x17,0x0000);
        Serial.println(F("Clearing CycleCount..."));
        break;
      case 0x37:
        WriteSMBWord(0x1B,(year-1980)*512+int(month)*32+day);
        Serial.println(F("Setting current date..."));
        break;
      case 0x38:
        Serial.println(F("Writing DesignCapacity, QMAX, Update status, Ra_table..."));
        WriteSMBWord(0x18,new_capacity);
        delay(100);
        ReadSMBSubclass(82,0x78);
        buff[1]=highByte(new_capacity);
        buff[2]=lowByte(new_capacity);
        buff[3]=highByte(new_capacity);
        buff[4]=lowByte(new_capacity);
        buff[5]=highByte(new_capacity);
        buff[6]=lowByte(new_capacity);
        buff[7]=highByte(new_capacity);
        buff[8]=lowByte(new_capacity);
        buff[9]=highByte(new_capacity);
        buff[10]=lowByte(new_capacity);
        buff[13]=0x00;
        WriteSMBSubclass(82,0x78);
        for (byte i=88; i<=95; i++) {
          buff[0]=0x20;
          buff[1]=0xFF;
          buff[2]=(i<92) ? 0x55 : 0xFF;
          buff[3]=0x00;
          buff[4]=0xA0;
          buff[5]=0x00;
          buff[6]=0xA6;
          buff[7]=0x00;
          buff[8]=0x99;
          buff[9]=0x00;
          buff[10]=0x97;
          buff[11]=0x00;
          buff[12]=0x91;
          buff[13]=0x00;
          buff[14]=0x98;
          buff[15]=0x00;
          buff[16]=0xB0;
          buff[17]=0x00;
          buff[18]=0xCC;
          buff[19]=0x00;
          buff[20]=0xDE;
          buff[21]=0x00;
          buff[22]=0xFE;
          buff[23]=0x01;
          buff[24]=0x3B;
          buff[25]=0x01;
          buff[26]=0xB5;
          buff[27]=0x02;
          buff[28]=0x8B;
          buff[29]=0x03;
          buff[30]=0xE9;
          buff[31]=0x05;
          buff[32]=0xB2;
          WriteSMBSubclass(i,0x78);
        }
        break;
      case 0x39:
        WriteSMBWord(0x00,0x0021);
        Serial.println(F("Begin the Impedance Track algorithm..."));
        break;
    }
  } else Serial.read();
}
