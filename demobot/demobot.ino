#include <Servo.h>
#include <SPI.h>
Servo servoch1;
Servo servoch2;
bool readerror=false;


float const gyroSensitivity    = 80.0; // From ADXRS450 datasheet.
float const gyroRawNullOffset  = 74.0; // Hard code for now to avoid calibration time at startup.
float       currentAngle       = 0;    // Keep track of our current angle
float       currentAnglularVel = 0;    // Keep track of our current angle
uint32_t    lastTimeUs         = 0;    // Time since last measurement in uS
bool        toggle             = false;// if the light has toggled this loop already is true
bool        momentarylast      = true; // last mode was in momentary light mode if true

#define PIN_CTRL_DRIVE_X      ( 5 )
#define PIN_CTRL_DRIVE_Y      ( 4 )
#define PIN_CTRL_HORN         ( 3 )
#define PIN_CTRL_LIGHT        ( 2 )
#define PIN_CTRL_ROBOT_ENABLE ( 6 )
#define PIN_UNUSED            ( 7 )
#define PIN_SERVO1            ( 8 )
#define PIN_SERVO_RESERVED_0  ( 9 )
#define PIN_SERVO_RESERVED_1  ( 10 )
#define PIN_SPI_MOSI          ( 11 )
#define PIN_SPI_MISO          ( 12 )
#define PIN_SPI_SCK           ( 13 )

#define PIN_LIGHTS            ( A0 )
#define PIN_HORN              ( A1 )
#define PIN_SERVO2            ( A2 )
#define PIN_SPI_CS            ( A3 )

#define STICK_TRIGGER 50

void setup() {
pinMode(PIN_CTRL_DRIVE_X, INPUT);
pinMode(PIN_CTRL_DRIVE_Y, INPUT);
pinMode(PIN_CTRL_HORN, INPUT);
pinMode(PIN_CTRL_LIGHT, INPUT);
pinMode(PIN_CTRL_ROBOT_ENABLE, INPUT);
pinMode(PIN_UNUSED, INPUT);//not used
servoch1.attach(PIN_SERVO1);
//pins 9 and 10 used for timing in servo library
servoch2.attach(PIN_SERVO2);
pinMode(PIN_HORN, OUTPUT);
pinMode(PIN_LIGHTS, OUTPUT);
digitalWrite(PIN_HORN,LOW);
digitalWrite(PIN_LIGHTS,LOW);

Serial.begin(9600);
SPI.begin();
pinMode(PIN_SPI_CS, OUTPUT);
digitalWrite(PIN_SPI_CS,HIGH);
SPI.setBitOrder(MSBFIRST);
SPI.setClockDivider(SPI_CLOCK_DIV16); 
SPI.setDataMode(SPI_MODE0);
delay(100);
}


int16_t ReadGyroData( void )
{
  digitalWrite(PIN_SPI_CS, LOW);
  int16_t result0 = SPI.transfer(0x20);
  int16_t result1 = SPI.transfer(0x00);
  int16_t result2 = SPI.transfer(0x00);
  int16_t result3 = SPI.transfer(0x00);
  
  int16_t result = ( ( 0x03 & result0 ) << 14 ) | ( result1 << 6 ) | ( result2 >> 2 );

  digitalWrite(PIN_SPI_CS, HIGH);
  return result;
}


void Drive( int forwardSpeed, int turnSpeed )
{
  float turnSpeedAdj = 0.0;

  // Experimentally found values.
  uint16_t const minTurnSpeed        = 5;
  float    const minSpeedToCorrect   = 30.0;
  float    const speedCorrectionGain = 0.2;

  if ( abs(turnSpeed) < minTurnSpeed && 
       fabs(currentAnglularVel) > minSpeedToCorrect ) 
  {
    turnSpeedAdj = -currentAnglularVel * speedCorrectionGain;
  }
  else
  {
    turnSpeedAdj = turnSpeed;
  }

  int rightmotor = (float)forwardSpeed - turnSpeedAdj;
  int leftmotor  = (float)forwardSpeed + turnSpeedAdj;

  servoch1.writeMicroseconds( victormap( leftmotor,  false ) );
  servoch2.writeMicroseconds( victormap( rightmotor, false ) );
}


void updateAngle( void )
{
    uint32_t currTimeUs         = micros();
    float    elapsedTimeS       = ( currTimeUs - lastTimeUs ) * 1.0e-6;
    float    gyroRateDegreePerS = ( ReadGyroData() - gyroRawNullOffset ) / gyroSensitivity;
    float    angleChange        = gyroRateDegreePerS * elapsedTimeS;
  
    currentAngle       += angleChange;
    currentAnglularVel = gyroRateDegreePerS;
    lastTimeUs         = currTimeUs;
}


void loop() {
  // Update angle between puleIn calls to be more accurate.
  // pulseIn calls take a large amount of time.
  updateAngle();
  int ch1 = pulseIn(PIN_CTRL_DRIVE_X, HIGH); //read the pulse width of each channel
  updateAngle();
  int ch2 = pulseIn(PIN_CTRL_DRIVE_Y, HIGH);
  updateAngle();
  int ch3 = pulseIn(PIN_CTRL_HORN, HIGH);
  updateAngle();
  int ch4 = pulseIn(PIN_CTRL_LIGHT, HIGH);
  updateAngle();
  int ch5 = pulseIn(PIN_CTRL_ROBOT_ENABLE, HIGH);
  updateAngle();
  if(ch1==0||ch2==0||ch3==0||ch4==0||ch5==0) readerror=true;
  //int ch6 = pulseIn(PIN_UNUSED, HIGH);
  ch1=map(ch1, 1139,1820,-100,100);//tested max limits of each channel
  ch2=map(ch2, 1113,1905,-100,100);
  ch3=map(ch3, 1198,1870,-100,100);
  ch4=map(ch4, 1100,1875,-100,100);
  ch5=map(ch5, 997,2007,-100,100);
  //ch6=map(ch6, 997,2007,-100,100);
  if(ch5>0&&readerror==false){
    ch1=deadband(ch1);
    ch2=deadband(ch2);
    ch3=deadband(ch3);
    ch4=deadband(ch4);
    ch5=deadband(ch5);
   // ch6=deadband(ch6);

    Drive( ch2, ch1 );
    
    if (ch3>STICK_TRIGGER) digitalWrite(PIN_HORN,HIGH);//blow horn
    else digitalWrite(PIN_HORN,LOW);
    if(ch4<-STICK_TRIGGER){
      if (toggle==false) digitalWrite(PIN_LIGHTS,!digitalRead(PIN_LIGHTS));//toggle lights
      toggle=true;
      momentarylast=false;
    } 
    if(ch4>=-STICK_TRIGGER && ch4<=STICK_TRIGGER){
      toggle=false;//reset toggle to allow again
      if (momentarylast==true) digitalWrite(PIN_LIGHTS,LOW); //turnoff lights if in momentary mode
    }
    if(ch4>STICK_TRIGGER){
      toggle=false;
      momentarylast=true;
      digitalWrite(PIN_LIGHTS,HIGH);//turn on lights
    }
  }
  else{
    servoch1.writeMicroseconds(1500);
    servoch2.writeMicroseconds(1500);
    digitalWrite(PIN_HORN,LOW);
    digitalWrite(PIN_LIGHTS,LOW);
    if(readerror==true) Serial.println("Channel Read Error");
    ch1 = pulseIn(PIN_CTRL_DRIVE_X, HIGH); //read the pulse width of each channel
    ch2 = pulseIn(PIN_CTRL_DRIVE_Y, HIGH);
    ch3 = pulseIn(PIN_CTRL_HORN, HIGH);
    ch4 = pulseIn(PIN_CTRL_LIGHT, HIGH);
    ch5 = pulseIn(PIN_CTRL_ROBOT_ENABLE, HIGH);
    readerror=false;
    if(ch1==0||ch2==0||ch3==0||ch4==0||ch5==0) readerror=true;
  }


  Serial.print(currentAngle);
  Serial.print("   ");
  Serial.print(ch1);
  Serial.print("   ");
  Serial.print(ch2);
  Serial.print("   ");
  Serial.print(ch3);
  Serial.print("   ");
  Serial.print(ch4);
  Serial.print("   ");
  Serial.print(ch5);
  Serial.println("");
}

int victormap(int ch, bool rev){
  if(rev==true) ch=(-ch);
  if(ch>100) ch=100;
  if(ch<-100) ch=-100;
  if (ch<0){
    ch=abs(ch);
    ch=map(ch,100,0,1000,1500);
    return ch;
  }
  if (ch>0){
    ch=map(ch,0,100,1500,2000);
    return ch;
  }
  return 1500; //center is 1500us
}

int deadband(int ch){
  static int band=10;
  if(ch>0&&ch<band) ch=0;
  if(ch<0&&ch>(-band)) ch=0;
  //remap at end of deaband
  if(ch>0) ch=map(ch,band,100,0,100);
  if(ch<0) ch=map(ch,-band,-100,0,-100);  
  return ch;
}
