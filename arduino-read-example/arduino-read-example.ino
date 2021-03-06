#include <SPI.h>

#define NSS_PIN 22 
#define READ_ENA_PIN 2

static const int spi_encoder_offset = 4;
enum SpiEncoders
{
  _X1 = 0b0001,
  _X2 = 0b0010,
  _Y  = 0b0100,
  _Z  = 0b1000,
};

void setup()
{
  pinMode(READ_ENA_PIN, INPUT_PULLUP);
  pinMode(NSS_PIN, OUTPUT);
  digitalWrite(NSS_PIN, HIGH);

  Serial.begin(9600);
  SPI.begin();
}

void loop()
{
  const int readPeriodMs = 200;
  
  if (LOW == digitalRead(READ_ENA_PIN))
  {
    long EncoderData = ReadEncoder(_X1);
    Serial.print("EncoderData: ");
    Serial.println(EncoderData);
  }
  
  delay(readPeriodMs);
}


void ResetEncoder(SpiEncoders encoders)
{
  const byte reset_cmd = 0x00;

  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  digitalWrite(NSS_PIN,LOW);
  delayMicroseconds(2);
  SPI.transfer(reset_cmd | (encoders << spi_encoder_offset) );
  delayMicroseconds(5);
  digitalWrite(NSS_PIN,HIGH);
  SPI.endTransaction();
  
}

long ReadEncoder(SpiEncoders encoder)
{
  const byte read_cmd  = 0x0F;
  int readSize = 4;
  long encoderVal = 0;

  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  digitalWrite(NSS_PIN,LOW);
  delayMicroseconds(2);
  SPI.transfer(read_cmd | (encoder << spi_encoder_offset) );
  delayMicroseconds(5);

  for (int i = 0; i < readSize; ++i)
  {
    encoderVal <<= 8;
    encoderVal |= SPI.transfer(0x01);
  }
  
  digitalWrite(NSS_PIN,HIGH);
  SPI.endTransaction();
  return encoderVal;
}
