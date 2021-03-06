/* SensorType info

type="sht30"
[0] = tempK
[1] = humidity

*/

os_timer_t  sensorSHTPollTimer; // repeating timer that fires ever X/time to start poll cycle
os_timer_t  sensorSHTReadTimer; // once request cycle starts, this timer set so we can send when ready

bool readytoPollSHT = false;
bool readytoReadSHT = false;


void setupSHT30() {

  // prepare SHT Timers 
  os_timer_setfn(&sensorSHTPollTimer, interuptSHTPoll, NULL);
  os_timer_setfn(&sensorSHTReadTimer, interuptSHTRead, NULL);

  Serial.print("Starting SHT polling timer at: ");
  Serial.print(sensorSHTReadDelay);  
  Serial.println("ms");
  os_timer_arm(&sensorSHTPollTimer, sensorSHTReadDelay, true); // TODO should probably confirm we actually have an sht30 connected
  
}

void handleSHT30() {
  
  if (readytoPollSHT){
    digitalWrite(LED_BUILTIN, LOW);
    pollSHT();
    digitalWrite(LED_BUILTIN, HIGH);  
  }
  
  if (readytoReadSHT){
    digitalWrite(LED_BUILTIN, LOW);
    readSHT();
    digitalWrite(LED_BUILTIN, HIGH);  
  }
  
}



void setSHTReadDelay(uint32_t newDelay) {
  os_timer_disarm(&sensorSHTPollTimer);
  sensorSHTReadDelay = newDelay;
  Serial.print("Restarting SHT30 polling timer at: ");
  Serial.print(newDelay);  
  Serial.println("ms");  
  os_timer_arm(&sensorSHTPollTimer, sensorSHTReadDelay, true);  
}

void interuptSHTPoll(void *pArg) {
  readytoPollSHT = true;
}

void interuptSHTRead(void *pArg) {
  readytoReadSHT = true;
}

void pollSHT() {
  
  SensorInfo *thisSensorInfo;
  uint8_t address;
  uint8_t errorCode;

  readytoPollSHT = false; //reset interupt

  for (int x=0;x<sensorList.size() ; x++) {
    thisSensorInfo = sensorList.get(x);
    if (strcmp(thisSensorInfo->type, "sht30") == 0) {
      //convert address string to int
      parseBytes(thisSensorInfo->address,':',&address,1,16);
      Wire.beginTransmission(address);
      Wire.write(0x2C);
      Wire.write(0x06);
      // Stop I2C transmission
      errorCode = Wire.endTransmission();
      if (errorCode != 0) {
        Serial.print("Error pollling SHT at address: ");
        Serial.print(address, HEX);
        Serial.print(" ErrorCode: ");
        Serial.println(errorCode);
      }
    }
  }
 

  os_timer_arm(&sensorSHTReadTimer, 100, false); // prepare to read after 100ms
}

void readSHT() {
  SensorInfo *thisSensorInfo;
  uint8_t errorCode;
  uint8_t data[6];
  uint8_t address;
  float tempK;
  float humidity;
  
  readytoReadSHT = false; //reset interupt

  for (int x=0;x<sensorList.size() ; x++) {
    thisSensorInfo = sensorList.get(x);
    if (strcmp(thisSensorInfo->type, "sht30") == 0) {
      //convert address string to int
      parseBytes(thisSensorInfo->address,':',&address,1,16);

      Wire.beginTransmission(address);
      Wire.requestFrom(address, (uint8_t)6); // either both int or both uint8_t...
      // Read 6 bytes of data
      // cTemp msb, cTemp lsb, cTemp crc, humidity msb, humidity lsb, humidity crc
      if (Wire.available() == 6)
      {
        data[0] = Wire.read();
        data[1] = Wire.read();
        data[2] = Wire.read();
        data[3] = Wire.read();
        data[4] = Wire.read();
        data[5] = Wire.read();
      }
      errorCode = Wire.endTransmission();
      if (errorCode != 0) {
        Serial.print("Error reading from SHT30 at address: ");
        Serial.print(address, HEX);
        Serial.print(" ErrorCode: ");
        Serial.println(errorCode);
        thisSensorInfo->valueJson[0] = "null";
        thisSensorInfo->valueJson[1] = "null";
        return;
      }
  
      // Convert the data
      tempK = (((((data[0] * 256.0) + data[1]) * 175) / 65535.0) - 45) + 273.15;
      humidity = ((((data[3] * 256.0) + data[4]) * 100) / 65535.0);

      thisSensorInfo->valueJson[0] = tempK;
      thisSensorInfo->valueJson[1] = humidity;
      thisSensorInfo->isUpdated = true;
    }
  }
}

