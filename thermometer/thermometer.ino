#include <OneWire.h>

#define pb0  8
#define pb1  9
#define pb2  10
#define pb3  11
#define pb4  12
#define pb5  13

#define pd0  0
#define pd1  1
#define pd2  2
#define pd3  3
#define pd4  4
#define pd5  5
#define pd6  6
#define pd7  7

#define segA  0xFE
#define segB  0xFD
#define segC  0xFB
#define segD  0xF7
#define segE  0xEF
#define segF  0xDF
#define segG  0xBF
#define segDP 0x7F

// DS18S20 Temperature chip i/o
OneWire ds(A3);

const byte charsMap[13] =
{
    segA & segB & segC & segD & segE & segF,        // 0
    segB & segC,                                    // 1
    segA & segB & segD & segE & segG,               // 2
    segA & segB & segC & segD & segG,               // 3
    segB & segC & segF & segG,                      // 4
    segA & segC & segD & segG & segF,               // 5
    segA & segC & segD & segE & segF & segG,        // 6
    segA & segB & segC,                             // 7
    segA & segB & segC & segD & segE & segF & segG, // 8
    segA & segB & segC & segD & segF & segG,        // 9
    segG,                                           // -
    0xFF                                            // off
};

const byte outputPins[14] = {pd0, pd1, pd2, pd3, pd4, pd5, pd6, pd7, pb0, pb1, pb2, pb3, pb4, pb5};
const byte ledsAnodes[4] = {0xFE, 0xFD, 0xFB, 0xF7};

byte displayBuffer[4] = {10, 10, 10, 10};
byte currentDigit = 0;
byte isTempMeasured = false;
byte sensorAddr[8];

bool isConvPending = false;

unsigned long lastUpdate;
unsigned long currentMillis;

void setup(void)
{
    for (int i = 0; i < 14; i++)
    {
        pinMode(outputPins[i], OUTPUT);
    }

    PORTD = 0xFF;
    PORTB = PORTB | 0xFF;

    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
}

void showDigit()
{
    PORTB = PORTB | 0x0F;
    PORTD = charsMap[displayBuffer[currentDigit]];

    if (currentDigit == 2 && isTempMeasured)
    {
        PORTD = PORTD & segDP;
    }

    PORTB = PORTB & ledsAnodes[currentDigit];

    currentDigit = currentDigit < 3 ? currentDigit + 1 : 0;
}


SIGNAL(TIMER0_COMPA_vect)
{
    showDigit();

    if (!isConvPending)
    {
        return;
    }

    currentMillis = millis();

    if ((currentMillis - lastUpdate) > 1000)
    {
        lastUpdate = millis();
        readSensorValue();
        isConvPending = false;
    }
}

void readSensor()
{
    if (isConvPending)
    {
        return;
    }

    ds.reset_search();

    if (!ds.search(sensorAddr))
    {
        ds.reset_search();
        return;
    }

    if (OneWire::crc8(sensorAddr, 7) != sensorAddr[7])
    {
        return;
    }

    ds.reset();
    ds.select(sensorAddr);
    ds.write(0x44, 1);  // start conversion, with parasite power on at the end

    isConvPending = true;
}

void readSensorValue()
{
    int HighByte, LowByte, TReading, SignBit, Tc_100, Whole, Fract;
    byte data[12];

    ds.reset();
    ds.select(sensorAddr);
    ds.write(0xBE); // Read Scratchpad

    for (int i = 0; i < 9; i++) // we need 9 bytes
    {
        data[i] = ds.read();
    }

    LowByte = data[0];
    HighByte = data[1];
    TReading = (HighByte << 8) + LowByte;
    SignBit = TReading & 0x8000;    // test most sig bit

    if (SignBit)    // negative
    {
        TReading = (TReading ^ 0xffff) + 1; // 2's comp
    }

    Tc_100 = (6 * TReading) + TReading / 4; // multiply by (100 * 0.0625) or 6.25

    Whole = Tc_100 / 100;   // separate off the whole and fractional portions
    Fract = Tc_100 % 100;

    byte tmpBuffer[4] = {11, 11, 11, 11};

    if (Whole > 9) {
      tmpBuffer[1] = Whole / 10;
    }
    
    tmpBuffer[2] = Whole % 10;
    tmpBuffer[3] = Fract / 10;

    if (SignBit)
    {
        if (Whole < 10)
        {
            tmpBuffer[1] = 10;
        }
        else
        {
            tmpBuffer[0] = 10;
        }
    }

    isTempMeasured = true;

    for (int i = 0; i < 4; i++)
    {
        displayBuffer[i] = tmpBuffer[i];
    }
}

void loop()
{
    readSensor();
}
