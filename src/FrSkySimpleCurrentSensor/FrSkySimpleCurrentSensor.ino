

/*

Changelog:

V3 - 23.8.2013
  * fixed rounding from float to integer
  * better and simpler averaging (median)
  * data sent to frsky receiver every ~200ms
  * using low power sleep (average consumption with sensor 20mA, without power saving 35mA)
  * Verified voltage calibration - perfect
  * green LED turned off 

V2 
  * bug: fixed negative current
  * set IO pins for lowest power consumption  


V1 - 20.8.2013
  * initial version

*/


#include <SoftwareSerial.h>
#include <LowPower.h>  //from https://github.com/rocketscream/Low-Power

//#define DEBUG

// Pin 13 has an LED connected on most Arduino boards.
// give it a name:
//#define CURRENT_PIN  6    //vhod A6
//#define CURRENT_PIN_NAME   A6
#define PIN_SerialTX        4  // 4  the pin to transmit the serial data to the frsky telemetry enabled receiver


//------------------------------------------------------------------------------------------------
// FRSKY constants
//------------------------------------------------------------------------------------------------
#define FRSKY_USERDATA_GPS_ALT_B    0x01
#define FRSKY_USERDATA_TEMP1        0x02
#define FRSKY_USERDATA_RPM          0x03
#define FRSKY_USERDATA_FUEL         0x04
#define FRSKY_USERDATA_TEMP2        0x05
#define FRSKY_USERDATA_CELL_VOLT    0x06

#define FRSKY_USERDATA_GPS_ALT_A    0x09
#define FRSKY_USERDATA_BARO_ALT_B   0x10
#define FRSKY_USERDATA_GPS_SPEED_B  0x11
#define FRSKY_USERDATA_GPS_LONG_B   0x12
#define FRSKY_USERDATA_GPS_LAT_B    0x13
#define FRSKY_USERDATA_GPS_CURSE_B  0x14
#define FRSKY_USERDATA_GPS_DM       0x15
#define FRSKY_USERDATA_GPS_YEAR     0x16
#define FRSKY_USERDATA_GPS_HM       0x17
#define FRSKY_USERDATA_GPS_SEC      0x18
#define FRSKY_USERDATA_GPS_SPEED_A  0x19
#define FRSKY_USERDATA_GPS_LONG_A   0x1A
#define FRSKY_USERDATA_GPS_LAT_A    0x1B
#define FRSKY_USERDATA_GPS_CURSE_A  0x1C

#define FRSKY_USERDATA_BARO_ALT_A   0x21
#define FRSKY_USERDATA_GPS_LONG_EW  0x22
#define FRSKY_USERDATA_GPS_LAT_EW   0x23
#define FRSKY_USERDATA_ACC_X        0x24
#define FRSKY_USERDATA_ACC_Y        0x25
#define FRSKY_USERDATA_ACC_Z        0x26

#define FRSKY_USERDATA_CURRENT      0x28  //current in 100mA steps, uint16_t

#define FRSKY_USERDATA_VERT_SPEED   0x30 // open9x Vario Mode Only
#define FRSKY_USERDATA_VFAS_NEW     0x39 // open9x Vario Mode Only, voltage in 100mV steps, uint16_t

#define FRSKY_USERDATA_VOLTAGE_B    0x3A
#define FRSKY_USERDATA_VOLTAGE_A    0x3B



//------------------------------------------------------------------------------------------------
// Global variables
//------------------------------------------------------------------------------------------------

SoftwareSerial _mySerial(0, PIN_SerialTX,true); // RX, TX
unsigned int n = 0;
unsigned int SumCount = 0;
unsigned int Current_raw_sum = 0;
unsigned int Voltage_raw_sum = 0;



void setup()
{
  // initialize the digital pin as an output.
  digitalWrite(2, HIGH); //enable pullup
  digitalWrite(3, HIGH); //enable pullup
  //digitalWrite(4, HIGH); //Sofware serial output
  digitalWrite(5, HIGH); //enable pullup
  digitalWrite(6, HIGH); //enable pullup
  digitalWrite(7, HIGH); //enable pullup
  digitalWrite(8, HIGH); //enable pullup
  digitalWrite(9, HIGH); //enable pullup
  digitalWrite(10, HIGH); //enable pullup
  digitalWrite(11, HIGH); //enable pullup
  digitalWrite(12, HIGH); //enable pullup
  pinMode(13, OUTPUT);       //LED output
  digitalWrite(13, LOW); //turn off LED

  digitalWrite(A0, HIGH); //enable pullup
  digitalWrite(A2, HIGH); //enable pullup
  //pinMode(A3, INPUT);          //A3 is voltage sensor
  digitalWrite(A3, LOW);       //disable pullup for actual analog input
  digitalWrite(A4, HIGH); //enable pullup
  digitalWrite(A5, HIGH); //enable pullup
  //pinMode(A6, INPUT);          //A6 is current sensor
  digitalWrite(A6, LOW);       //disable pullup for actual analog input
  digitalWrite(A7, HIGH); //enable pullup

#ifdef DEBUG
  Serial.begin(115200);
  Serial.println("Current sensor start");
#endif

  // set the data rate for the SoftwareSerial port
  _mySerial.begin(9600);
  
  delay(10);
}


void loop()
{
  //sleep in low power mode to reduce current consuption
  LowPower.idle(SLEEP_15Ms, ADC_OFF, TIMER2_OFF, TIMER1_OFF, TIMER0_OFF, SPI_OFF, USART0_OFF, TWI_OFF);

  //read sensors
  readInputs();
  if ( ++n >= 14 )      //executes every cca 200ms
  {
    n = 0;
    //send data
    sendData();
    //toogle_led();
    delay(10); //delay needed before going into low powe mode to finis sending serial data
  }
    
}


void readInputs()
{
  Current_raw_sum += analogRead(6);
  Voltage_raw_sum += analogRead(3);
  SumCount += 1;
}

void sendData()
{
  //calculate
  //float Current = ( (Current_raw * 5.0 / 1023.0) - 0.588 )  * 25.0;
  //float Current = ( (Current_raw_avg_x10 / 10.0 * 5.0 / 1023.0) - 0.588 )  * 25.0;
  //float Current = ( (Current_raw_avg_x10 / 2046.0) - 0.588 )  * 25.0;
  //float Current = (Current_raw_avg_x10 / 81.84) - 14.7 ;
  float Current = (( Current_raw_sum / float(SumCount) ) / 8.184) - 14.7; //calibrated to my sensor (RC filter 22k/1uF)
  if ( Current < 0 )
  {
    //current must be positive (unsigned 16 bit integer in OpenTX)
    Current = 0;
  }
  SendValue(FRSKY_USERDATA_CURRENT, (uint16_t)(Current * 10.0 + 0.5));
  
  
  //float Voltage = ( Voltage_raw_avg_x10 * 5.0 / 1023 ) * 5.61 ; 
  //float Voltage = Voltage_raw_avg_x10 / 364.7058;
  float Voltage = ( Voltage_raw_sum / float(SumCount) ) / 36.47058; //calibrated to my sensor (divider and RC filter: 22k/4k7/1uF, divider for max 6S battery)
  
  SendValue(FRSKY_USERDATA_VFAS_NEW, (uint16_t)(Voltage * 10.0 + 0.5));
  _mySerial.write(0x5E); // End of Frame
  
 
#ifdef DEBUG  
  Serial.print("avgc:"); Serial.print(Current_raw_sum); 
  Serial.print(" avgv:"); Serial.print(Voltage_raw_sum); 
  Serial.print(" cur:"); Serial.print(Current); //Serial.println("");   
  Serial.print(" vol:"); Serial.print(Voltage);
  Serial.print(" cnt:"); Serial.print(SumCount); Serial.println("");   
#endif 

  SumCount = Voltage_raw_sum = Current_raw_sum = 0;
  
}


void toogle_led()
{
  static bool state;
  digitalWrite(13, state);
  state = ! state;
}

/**********************************************************/
/* SendValue => send a value as frsky sensor hub data     */
/**********************************************************/
void SendValue(uint8_t ID, uint16_t Value) 
{
  uint8_t tmp1 = Value & 0x00ff;
  uint8_t tmp2 = (Value & 0xff00)>>8;
  _mySerial.write(0x5E);  
  _mySerial.write(ID);
  if(tmp1 == 0x5E) {
    _mySerial.write(0x5D);    
    _mySerial.write(0x3E);  
  }
  else if(tmp1 == 0x5D) {    
    _mySerial.write(0x5D);    
    _mySerial.write(0x3D);  
  }
  else {    
    _mySerial.write(tmp1);  
  }
  if(tmp2 == 0x5E) {    
    _mySerial.write(0x5D);    
    _mySerial.write(0x3E);  
  }
  else if(tmp2 == 0x5D) {    
    _mySerial.write(0x5D);    
    _mySerial.write(0x3D);  
  }
  else {    
    _mySerial.write(tmp2);  
  }
}

