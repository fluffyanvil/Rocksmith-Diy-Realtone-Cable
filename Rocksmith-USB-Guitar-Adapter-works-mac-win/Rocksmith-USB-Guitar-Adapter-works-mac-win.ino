#define PIN_DO 50 //MISO - Digital 50
#define PIN_DI 51 //MOSI - Digital 51
#define PIN_SK 52 //SCK - Digital 52
#define PIN_CS 53 //SS - Digital 53

#define BIT_LENGTH 16

//#define _DEBUG_MODE_
#ifdef _DEBUG_MODE_
#define DEBUG_LOG(x) Serial.print(x)
#else
#define DEBUG_LOG(x)
#endif

union DATA{
  uint16_t  W; 
  uint8_t   B[2];
};
// Rocksmith USB Guitar Adapter vendor (works on mac, windows, ps4)                                                                                                                               
uint8_t DESC_DATA[] = { 0x05,0x67,0xBA,0x12,0xFF,0x00,0xFF,0xFF,\
                        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,\
                        0xFF,0xFF,0xFF,0xFF,0x3C,0x00,0x52,0x6F,\
                        0x63,0x6B,0x73,0x6D,0x69,0x74,0x68,0x20,\
                        0x55,0x53,0x42,0x20,0x47,0x75,0x69,0x74,\
                        0x61,0x72,0x20,0x41,0x64,0x61,0x70,0x74,\
                        0x65,0x72,0x00,0xFF,0x07,0x00,0x55,0x42,\
                        0x49,0x53,0x4F,0x46,0x54,0x00,0xFF,0xFF,\
                        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,\
                        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,\
                        0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
uint8_t DESC_COUNT = 88;

void mwiOut(uint16_t value,uint8_t bitOrder,uint8_t bitAmount)
{
  uint8_t dataPin = PIN_DI;
  uint8_t clockPin = PIN_SK;
  uint8_t x = (bitAmount>BIT_LENGTH)?BIT_LENGTH:bitAmount;
  uint8_t i;
  uint8_t dataBit = 0;

  DEBUG_LOG("W[");
  for (i = 0; i < x; i++)
  {
    if (bitOrder == LSBFIRST)
    {
      dataBit = (1 & ( value >> i));
    }
    else  
    {
      dataBit = (1 & ( value >> ((BIT_LENGTH-1)-i)));
    }
    digitalWrite(dataPin,dataBit);
    DEBUG_LOG(dataBit);
    digitalWrite(clockPin, HIGH);
    digitalWrite(clockPin, LOW);  
  }
  DEBUG_LOG("]");
}

uint16_t mwiIn(uint8_t bitOrder,uint8_t bitAmount)
{
  uint16_t value = 0;
  uint8_t dataPin = PIN_DO;
  uint8_t clockPin = PIN_SK;
  uint8_t i;
  uint8_t x = (bitAmount>BIT_LENGTH)?BIT_LENGTH:bitAmount;

  DEBUG_LOG("R[");
  for (i = 0; i < x; ++i) {
    digitalWrite(clockPin, HIGH);
    digitalWrite(clockPin, LOW);
    DEBUG_LOG(digitalRead(dataPin));
    if (bitOrder == LSBFIRST)
      value |= digitalRead(dataPin) << i;
    else
      value |= digitalRead(dataPin) << ((BIT_LENGTH-1) - i);
  }
  DEBUG_LOG("]");
  return value;
}

void waitReady(void)
{
  // Wait ready operation
  EEPROM_ENABLE(true);
  bool bReady = false;
  do
  {
    bReady = (digitalRead(PIN_DO)==HIGH);
    digitalWrite(PIN_SK, HIGH);
    digitalWrite(PIN_SK, LOW);
    //Serial.print(".");
  }while(!bReady);
  Serial.println("DONE!");
  EEPROM_ENABLE(false);
}

void EEPROM_ENABLE(bool value)
{
  if(value)
  {
    digitalWrite(PIN_CS, HIGH);
    digitalWrite(PIN_SK, LOW);
    digitalWrite(PIN_DI, LOW);
  }
  else
  {
    digitalWrite(PIN_CS, LOW);
    digitalWrite(PIN_SK, LOW);
    digitalWrite(PIN_DI, LOW);
  }
}

void EEPROM_EWEN()
{
  uint16_t cmd = 0b1001100000000000;

  EEPROM_ENABLE(true);
  mwiOut(cmd,MSBFIRST,9);
  EEPROM_ENABLE(false);
}

void EEPROM_EWDS()
{
  uint16_t cmd = 0b1000000000000000;

  EEPROM_ENABLE(true);
  mwiOut(cmd,MSBFIRST,9);
  EEPROM_ENABLE(false);
}

uint16_t EEPROM_READ(uint16_t address)
{
  uint16_t value = 0;
  uint16_t cmd = 0b1100000000000000;
  uint16_t add = address << 10;

  EEPROM_ENABLE(true);
  mwiOut(cmd,MSBFIRST,3);
  mwiOut(add,MSBFIRST,6);
  value = mwiIn(MSBFIRST,16);
  EEPROM_ENABLE(false);
  return value;
}

void EEPROM_WRITE(uint16_t address,uint16_t data)
{
  uint16_t cmd = 0b1010000000000000;
  uint16_t add = address << 10;

  // Write operation
  EEPROM_ENABLE(true);
  mwiOut(cmd,MSBFIRST,3);
  mwiOut(add,MSBFIRST,6);
  mwiOut(data,MSBFIRST,16);
  EEPROM_ENABLE(false);
  waitReady();
}

void setup()
{
  // start serial port at 9600 bps:
  Serial.begin(9600);
  Serial.println("ROCKSMITH EEPROM WRITER (AT93C46)");

  // setup software 3-Wire interface
  pinMode(PIN_SK, OUTPUT);
  pinMode(PIN_CS, OUTPUT);
  pinMode(PIN_DI, OUTPUT);
  pinMode(PIN_DO, INPUT);
  EEPROM_ENABLE(false);
}

void loop()
{
  Serial.print("EEPROM_ENABLE(TRUE):");
  EEPROM_ENABLE(true);
  Serial.println(" DONE!");
  Serial.print("EEPROM_EWEN:");
  EEPROM_EWEN();
  Serial.println(" DONE!");
  for(int i=0;i<=DESC_COUNT;i+=2)
  {
    Serial.print("EEPROM_WRITE(");
    Serial.print(i);
    Serial.print("/");
    Serial.print(DESC_COUNT);
    Serial.print(")");
    EEPROM_WRITE((i>>1),((DESC_DATA[i+1]<<8)|(DESC_DATA[i])));
  }
  Serial.print("EEPROM_EWDS:");
  EEPROM_EWDS();
  Serial.println(" DONE!");

  Serial.println();
  Serial.println("VERIFY EEPROM");
  DATA temp;
  bool error = false;
  for(uint16_t i=0;i<=44;i++)
  {
    temp.W = EEPROM_READ(i);
    Serial.print("EEPROM_READ(0x");
    Serial.print(i,HEX);
    Serial.print("):0x");
    Serial.print(temp.W,HEX);

    if((temp.B[0]==DESC_DATA[(i<<1)])&&(temp.B[1]==DESC_DATA[(i<<1)+1]))
    {
      Serial.println(" PASSED!");
    }
    else
    {
      Serial.println(" FAILED!");
      error = true;
    }
  }

  if(error)
    Serial.println("DATA ERROR. PROGRAM TERMINATED!");
  else
    Serial.println("ALL PROCESS DONE!");

  delay(1000);
  exit(0);
}
