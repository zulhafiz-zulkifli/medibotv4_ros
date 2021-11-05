//wheel radius, r = 160mm = 0.16m
//wheel separation, l = 498mm = 0.498m
//lidar height from ground = 229.4mm
//diff drive parameters in metre
#define USE_USBCON
#include <ros.h>
#include <std_msgs/Int16.h>
#include <std_msgs/Float32.h>
ros::NodeHandle  nh;
#define WHEEL_SEPARATION 0.498
#define WHEEL_RADIUS 0.16
#define ENCODER_CPM 97
#define LOOPTIME 30
//motor pins
#define LH_D1 3          // Left hand PWM
#define LH_D2 28         // Left hand STOP
#define LH_D3 27         // Left hand DIRECTION
#define LH_A1 A4         // Left hand ANALOG
#define RH_D1 4          // Right hand PWM
#define RH_D2 26         // Right hand STOP
#define RH_D3 25         // Right hand DIRECTION
#define RH_A1 A5         // Right hand ANALOG
#define BR 29         // Brake
// wheel encoder pins
#define LH_ENA 46 // left encoder A
#define LH_ENB 45 // left encoder B         
#define RH_ENA 44 // right encoder A
#define RH_ENB 43 // right encoder B 
float demandx=0; // in m/s
float demandz=0; // in rad/s
float LH_PWM=0;
float RH_PWM=0;
volatile long encoderLH_Pos = 0; // encoder left pos
volatile long encoderRH_Pos = 0; // encoder right pos
unsigned long currentMillis;
unsigned long previousMillis;  

std_msgs::Int16 lwheel_msg;
std_msgs::Int16 rwheel_msg;

void left_cmd_cb( const std_msgs::Float32& lmotor_msg){
  LH_PWM=lmotor_msg.data;
}

void right_cmd_cb( const std_msgs::Float32& rmotor_msg){
  RH_PWM=rmotor_msg.data;
}

ros::Publisher lwheel_pub("lwheel", &lwheel_msg);
ros::Publisher rwheel_pub("rwheel", &rwheel_msg); 
ros::Subscriber<std_msgs::Float32> lmotor_sub("lmotor_cmd", left_cmd_cb );
ros::Subscriber<std_msgs::Float32> rmotor_sub("rmotor_cmd", right_cmd_cb );



void setup() {
  nh.getHardware()->setBaud(115200);
  nh.initNode();
  nh.subscribe(lmotor_sub);
  nh.subscribe(rmotor_sub);
  nh.advertise(lwheel_pub);
  nh.advertise(rwheel_pub);
  pinMode(RH_D1,OUTPUT); // motor PWM pins
  pinMode(RH_D2,OUTPUT);
  pinMode(RH_D3,OUTPUT);
  pinMode(LH_D1,OUTPUT);
  pinMode(LH_D2,OUTPUT);
  pinMode(LH_D3,OUTPUT);
  pinMode(BR,OUTPUT);
  pinMode(LH_ENA, INPUT_PULLUP); // encoder pins
  pinMode(LH_ENB, INPUT_PULLUP);
  pinMode(RH_ENA, INPUT_PULLUP);
  pinMode(RH_ENB, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(LH_ENA), doEncoderLHA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(LH_ENB), doEncoderLHB, CHANGE);
  attachInterrupt(digitalPinToInterrupt(RH_ENA), doEncoderRHA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(RH_ENB), doEncoderRHB, CHANGE);
}

void loop() {
  nh.spinOnce();
  currentMillis = millis();
  if(currentMillis - previousMillis >= LOOPTIME){
    previousMillis = currentMillis;
    //////////DRIVE LEFT WHEEL//////////
    if(LH_PWM>0){//left wheel forward
      analogWrite(LH_D1,LH_PWM);
      digitalWrite(LH_D2,HIGH);
      digitalWrite(LH_D3,LOW);
    }
    else if(LH_PWM<0){//left wheel backward
      analogWrite(LH_D1,abs(LH_PWM));
      digitalWrite(LH_D2,HIGH);
      digitalWrite(LH_D3,HIGH);
    }
    else{//left wheel stop
      analogWrite(LH_D1,0);
      digitalWrite(LH_D2,LOW);
//      digitalWrite(BR,HIGH);
    }
    //////////DRIVE RIGHT WHEEL//////////
    if(RH_PWM>0){//right wheel forward
      analogWrite(RH_D1,RH_PWM);
      digitalWrite(RH_D2,HIGH);
      digitalWrite(RH_D3,LOW);
    }
    else if(RH_PWM<0){//right wheel backward
      analogWrite(RH_D1,abs(RH_PWM));
      digitalWrite(RH_D2,HIGH);
      digitalWrite(RH_D3,HIGH);
    }
    else{//right wheel stop
      analogWrite(RH_D1,0);//right wheel stop
      digitalWrite(RH_D2,LOW);
//      digitalWrite(BR,HIGH);
    }
    //////////////////////////////////////
    lwheel_pub.publish(&lwheel_msg);
    rwheel_pub.publish(&rwheel_msg);
  }
}


//*************************encoders interrupt functions******************
//****************** encoder left ********************
void doEncoderLHA(){
  //look for low-to-high on channel A
  if(digitalRead(LH_ENA)==HIGH){
    //check channel B to see which way encoder is turning
    if(digitalRead(LH_ENB)==LOW){
      encoderLH_Pos++; // CW
    }
    else{
      encoderLH_Pos--; // CCW
    }
  }
  else{ //must be a high-to-low edge on channel A
    //check channel B to see which way encoder is turning
    if(digitalRead(LH_ENB)==HIGH){
      encoderLH_Pos++; // CW
    }
    else{
      encoderLH_Pos--; // CCW
    }
  }
  lwheel_msg.data = encoderLH_Pos;
}

void doEncoderLHB(){
  //look for low-to-high on channel B
  if(digitalRead(LH_ENB)==HIGH){
    //check channel A to see which way encoder is turning
    if(digitalRead(LH_ENA)==HIGH){
      encoderLH_Pos++; // CW
    }
    else{
      encoderLH_Pos--; // CCW
    }
  }
  else{ //must be a high-to-low edge on channel B
    //check channel A to see which way encoder is turning
    if(digitalRead(LH_ENA)==LOW){
      encoderLH_Pos++; // CW
    }
    else{
      encoderLH_Pos--; // CCW
    }
  }
  lwheel_msg.data = encoderLH_Pos;
}

//****************** encoder right ********************
void doEncoderRHA(){
  //look for low-to-high on channel A
  if(digitalRead(RH_ENA)==HIGH){
    //check channel B to see which way encoder is turning
    if(digitalRead(RH_ENB)==LOW){
      encoderRH_Pos++; // CW
    }
    else{
      encoderRH_Pos--; // CCW
    }
  }
  else{ //must be a high-to-low edge on channel A
    //check channel B to see which way encoder is turning
    if(digitalRead(RH_ENB)==HIGH){
      encoderRH_Pos++; // CW
    }
    else{
      encoderRH_Pos--; // CCW
    }
  }
  rwheel_msg.data = encoderRH_Pos;
}

void doEncoderRHB(){
  //look for low-to-high on channel B
  if(digitalRead(RH_ENB)==HIGH){
    //check channel A to see which way encoder is turning
    if(digitalRead(RH_ENA)==HIGH){
      encoderRH_Pos++; // CW
    }
    else{
      encoderRH_Pos--; // CCW
    }
  }
  else{ //must be a high-to-low edge on channel B
    //check channel A to see which way encoder is turning
    if(digitalRead(RH_ENA)==LOW){
      encoderRH_Pos++; // CW
    }
    else{
      encoderRH_Pos--; // CCW
    }
  }
  rwheel_msg.data = encoderRH_Pos;
}