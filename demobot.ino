#include <Servo.h>
Servo servoch1;
Servo servoch2;
bool readerror=false;

void setup() {
pinMode(2, INPUT);
pinMode(3, INPUT);
pinMode(4, INPUT);
pinMode(5, INPUT);
pinMode(6, INPUT);
pinMode(7, INPUT);//not used
servoch1.attach(8);
//pins 9 and 10 used for timing in servo library
servoch2.attach(11);
pinMode(12, OUTPUT);
pinMode(13, OUTPUT);
digitalWrite(12,LOW);
digitalWrite(13,LOW);
Serial.begin(9600);
}

void loop() {
  int rightmotor=0;
  int leftmotor=0;
  int ch1 = pulseIn(2, HIGH); //read the pulse width of each channel
  int ch2 = pulseIn(3, HIGH);
  int ch3 = pulseIn(4, HIGH);
  int ch4 = pulseIn(5, HIGH);
  int ch5 = pulseIn(6, HIGH);
  if(ch1==0||ch2==0||ch3==0||ch4==0||ch5==0) readerror=true;
  //int ch6 = pulseIn(7, HIGH);
  ch1=map(ch1, 1100,1875,-100,100);//tested max limits of each channel
  ch2=map(ch2, 1198,1870,-100,100);
  ch3=map(ch3, 1139,1820,-100,100);
  ch4=map(ch4, 1113,1905,-100,100);
  ch5=map(ch5, 997,2007,-100,100);
  //ch6=map(ch6, 997,2007,-100,100);
  if(ch5>0&&readerror==false){
    ch1=deadband(ch1);
    ch2=deadband(ch2);
    ch3=deadband(ch3);
    ch4=deadband(ch4);
    ch5=deadband(ch5);
   // ch6=deadband(ch6);
    rightmotor=ch2-ch1;
    leftmotor=ch2+ch1;
    servoch1.writeMicroseconds(victormap(leftmotor,false));
    servoch2.writeMicroseconds(victormap(rightmotor,false));
    if (ch3>50) digitalWrite(12,HIGH);//blow horn
    else digitalWrite(12,LOW);
    if(ch4>50||ch4<-50) digitalWrite(13,HIGH);//turn on lights
    else digitalWrite(13,LOW);
  }
  else{
    servoch1.writeMicroseconds(1500);
    servoch2.writeMicroseconds(1500);
    digitalWrite(12,LOW);
    digitalWrite(13,LOW);
    if(readerror==true) Serial.println("Channel Read Error");
    int ch1 = pulseIn(2, HIGH); //read the pulse width of each channel
    int ch2 = pulseIn(3, HIGH);
    int ch3 = pulseIn(4, HIGH);
    int ch4 = pulseIn(5, HIGH);
    int ch5 = pulseIn(6, HIGH);
    readerror=false;
    if(ch1==0||ch2==0||ch3==0||ch4==0||ch5==0) readerror=true;
  }
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
