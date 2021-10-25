
/*** Pin mapping ***/
#define PROG_PIN 8
#define ONEWIRE_PIN 9
#define BUTTON_0 2
#define BUTTON_1 6
#define LED0 10
#define LED1 A0

// This is the magic string Dell laptops need to see from the device in order to not go into limp home mode
// The interesting bit is the wattage 090 followed by the voltage 1950 and the amperage 4.6
char *progStr = "DELL00AC090195046CN09T";
#define ProgLocation (0x00)

#include <OneWire.h>

#define DS2502DevID 0x09

#define READ_ROM   0x33
#define MATCH_ROM  0x55
#define SKIP_ROM   0xCC
#define SEARCH_ROM 0xF0
#define ROM_SZ 8  // 8 bytes of laser etched ROM data , LSB = 0x09 == DS2502

// DS250x Memory commands -- can only be executed after one of the above ROM commands has been executed properly
#define READ_MEMORY  0xF0
#define READ_STATUS  0xAA
#define WRITE_STATUS 0x55
#define WRITE_MEMORY 0x0F

#define ProgPulseUs 500    // length of time 12V prog pulse needs to be applied for 

OneWire ds(ONEWIRE_PIN);             // OneWire bus on digital pin 6

void setup() {

  Serial.begin(115200);
  /*delay(2000);
    Serial.println("Hello !");*/
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);

  pinMode(PROG_PIN, OUTPUT);
  digitalWrite(PROG_PIN, LOW);

  if (isDSPresent())
  {
    digitalWrite(LED1, HIGH);
    readStatus();
  }

}
void readStatus()
{

  byte leemem[12];

  Serial.println("Reading Status");

  ds.reset();
  ds.skip();                      // Skip ROM search, address the device no matter what
  leemem[0] = READ_STATUS;        // command
  leemem[1] = 0;       // low address byte
  leemem[2] = 0;                  // High address byte, always 0 for 2502
  ds.write(leemem[0], 1);       // Read data command, leave ghost power on
  ds.write(leemem[1], 1);       // LSB starting address, leave ghost power on
  ds.write(leemem[2], 1);       // MSB starting address, leave ghost power on

  byte crc = ds.read();             // DS250x generates a CRC for the command we sent, we assign a read slot and store it's value
  byte crc_calc = OneWire::crc8(leemem, 3);  // We calculate the CRC of the commands we sent using the library function and store it

  if (crc_calc != crc)        // Then we compare it to the value the ds250x calculated, if it fails, we print debug messages and abort
  {
    Serial.println("Invalid command CRC!");
    Serial.print("Calculated CRC:");
    Serial.println(crc_calc, HEX);   // HEX makes it easier to observe and compare
    Serial.print("DS250x readback CRC:");
    Serial.println(crc, HEX);
  }

  Serial.println("Status is: ");

  byte data[8];

  for (int i = 0; i < 8; i++)
  {
    data[i] = ds.read();
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");
}
void loop() {

  if (digitalRead(BUTTON_0) == LOW)
  {
    readDSMemory();
    while (digitalRead(BUTTON_0) == LOW);
    delay(100);
  }

  if (digitalRead(BUTTON_1) == LOW)
  {
    writeDSMemory();
    while (digitalRead(BUTTON_1) == LOW);
    delay(100);
  }
}

void readDSMemory()
{

  byte leemem[12];

  Serial.println("Reading Memory");

  ds.reset();
  ds.skip();                      // Skip ROM search, address the device no matter what
  leemem[0] = READ_MEMORY;        // command
  leemem[1] = ProgLocation;       // low address byte
  leemem[2] = 0;                  // High address byte, always 0 for 2502
  ds.write(leemem[0], 1);       // Read data command, leave ghost power on
  ds.write(leemem[1], 1);       // LSB starting address, leave ghost power on
  ds.write(leemem[2], 1);       // MSB starting address, leave ghost power on

  byte crc = ds.read();             // DS250x generates a CRC for the command we sent, we assign a read slot and store it's value
  byte crc_calc = OneWire::crc8(leemem, 3);  // We calculate the CRC of the commands we sent using the library function and store it

  if (crc_calc != crc)        // Then we compare it to the value the ds250x calculated, if it fails, we print debug messages and abort
  {
    Serial.println("Invalid command CRC!");
    Serial.print("Calculated CRC:");
    Serial.println(crc_calc, HEX);   // HEX makes it easier to observe and compare
    Serial.print("DS250x readback CRC:");
    Serial.println(crc, HEX);
  }

  Serial.println("Data is: ");

  byte data[32];

  for (int i = 0; i < 32; i++)
  {
    data[i] = ds.read();
    //Serial.print(data[i], HEX);
    Serial.write(data[i]);
    Serial.print(" ");
  }
  Serial.println("");
}

void writeDSMemory()
{

  uint8_t leemem[12];

  for (uint8_t i = 0; i < strlen(progStr); i++) {
    
    ds.reset();
    ds.skip();

    Serial.println("PROG Memory");

    leemem[0] = WRITE_MEMORY;       // command to initiate a write seq using array for later CRC calc
    leemem[2] = 0;                  // high address is always 0
    leemem[1] = ProgLocation + i;
    leemem[3] = (uint8_t)progStr[i];

    for(uint8_t i=0;i<4;i++)
    {
      ds.write(leemem[i]);
    }
    
    uint8_t crc = ds.read();

    uint8_t crc_calc = OneWire::crc8(leemem, 4);  // We calculate the CRC of the commands we sent using the library function and store it

    if (crc_calc != crc)        // Then we compare it to the value the ds250x calculated, if it fails, we print debug messages and abort
    {
      Serial.println("Invalid command CRC!");
      Serial.print("Calculated CRC:");
      Serial.println(crc_calc, HEX);   // HEX makes it easier to observe and compare
      Serial.print("DS250x readback CRC:");
      Serial.println(crc, HEX);
    }

    progPulse();
    
    uint8_t data = ds.read();
    
    Serial.print("Programmed data read back : ");
    Serial.print("adress=0x");
    Serial.print(leemem[1],HEX);
     Serial.print(" char=");
    Serial.write(data);
    Serial.print(" value=0x");
    Serial.println(data, HEX);

  }
}

byte isDSPresent()
{

  byte data[32];

  if (ds.reset())
  {

    Serial.println("device present");

    ds.write(READ_ROM, 1);
    Serial.println("ROM  Data is: ");
    for (int i = 0; i < 8; i++)
    {
      data[i] = ds.read();
      Serial.print(data[i], HEX);
      Serial.print(" ");
    }

    Serial.println(".");
    byte crc_calc = OneWire::crc8(data, 7);

    if (crc_calc == data[7] )
    {
      Serial.println("ROM CRC matches ");
      if (data[0] != DS2502DevID)
      {
        Serial.println("Device ID does not match DS2502");
      }
      return 1;
    }
    else
    {
      Serial.println("ROM CRC failed");
    }

  }
  return 0;
}

void progPulse()
{
  
  digitalWrite(PROG_PIN, HIGH);
  delayMicroseconds(ProgPulseUs);
  digitalWrite(PROG_PIN, LOW);
  delayMicroseconds(100);
  
}
