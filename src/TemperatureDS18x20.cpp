#include <TemperatureDS18x20.h>


TemperatureDS18x20::TemperatureDS18x20(ESensors *sensors)
  : ESensor(sensors, "temperature", "T", "ºC", "%.2f", 0.0625) {
  Type_s = -1;
  memset(Addr, 0, sizeof(Addr));
  Celsius = NoValue;
}


TemperatureDS18x20::TemperatureDS18x20(uint8_t pin)
  : TemperatureDS18x20() {
  begin(pin);
}


TemperatureDS18x20::TemperatureDS18x20(ESensors *sensors, uint8_t pin)
  : TemperatureDS18x20(sensors) {
  begin(pin);
}


void TemperatureDS18x20::begin(uint8_t pin) {
  Type_s = -1;
  memset(Addr, 0, sizeof(Addr));
  Celsius = NoValue;

  OW.begin(pin);

  OW.reset_search();
  if (!OW.search(Addr)) {
    Serial.println("No DS18x20 temperature sensor found.");
    Serial.println();
    return;
  }

  if (OneWire::crc8(Addr, 7) != Addr[7]) {
    Serial.println("ERROR: OneWire CRC is not valid!");
    return;
  }

  setOneWireBus(pin);

  // ROM as string:
  char *sp = Identifier;
  for(byte i=0; i<8; i++)
    sp += sprintf(sp, "%02X ", Addr[i]);
  sp--;
  *sp = '\0';
 
  // the first ROM byte indicates which chip:
  switch (Addr[0]) {
    case 0x10:
      setChip("DS18S20");  // or old DS1820
      Type_s = 1;
      break;
    case 0x28:
      setChip("DS18B20");
      Type_s = 0;
      break;
    case 0x22:
      setChip("DS1822");
      Type_s = 0;
      break;
    default:
      setChip("unknown"); // not a DS18x20 family device
      return;
  } 
}


bool TemperatureDS18x20::available() {
  return (Type_s >= 0);
}


void TemperatureDS18x20::requestData() {
  if (Type_s < 0)
    return;

  if (!OW.reset()) {
    Serial.println("ERROR: OneWire device not present\n");
    return;
  }
  OW.select(Addr);
  OW.write(0x44, 1); // start conversion, with parasite power on at the end
}


void TemperatureDS18x20::getData() {
  Celsius = NoValue;
  if (Type_s < 0)
    return;
    
  if (!OW.reset()) {
    Serial.println("ERROR: OneWire device not present\n");
    return;
  }
  OW.select(Addr);    
  OW.write(0xBE);              // read Scratchpad

  byte data[12];
  for (byte i=0; i<9; i++)     // we need 9 bytes
    data[i] = OW.read();
  if (OneWire::crc8(data, 8) != data[8]) {
    Serial.println("ERROR: OneWire CRC is not valid!");
    return;
  }

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (Type_s == 1) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    // default is 12 bit resolution, 750 ms conversion time
  }
  Celsius = (float)raw / 16.0;
  OW.depower();
}


float TemperatureDS18x20::reading() const {
  return Celsius;
}

