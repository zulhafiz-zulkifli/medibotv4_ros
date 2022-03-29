//----------------------------------------------------------------------------------//
#define _SAM3XA_
#define USE_USBCON
#include <ros.h>
#include <std_msgs/Bool.h>
#include <std_msgs/Int16.h>
#include <geometry_msgs/Twist.h>
#include <medibotv4/SensorState.h>
#include <PID_v1.h>
#include "Config.h"
#include "Motor.h"
#include "LED.h"
Motor LH_motor(LH_D1, LH_D2, LH_D3, LH_ENA, LH_ENB);
Motor RH_motor(RH_D1, RH_D2, RH_D3, RH_ENA, RH_ENB);
Motor PAN_motor(PAN_D1, PAN_D2, PAN_EN);
Motor TILT_motor(TILT_D1, TILT_D2, TILT_EN);
LED LH_led(LED_R_LH, LED_G_LH, LED_B_LH);
LED RH_led(LED_R_RH, LED_G_RH, LED_B_RH);
void Move(int lpwm, int rpwm);
void LH_ISRA();
void LH_ISRB();
void RH_ISRA();
void RH_ISRB();
void EMG_STOP();
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);
unsigned long currentMillis, previousMillis;
double MAX_SPEED = (2*PI*WHEEL_RADIUS*MOTOR_RPM)/(60*GEAR_REDUCTION);//0.728485253 m/s
double demandx = 0, demandz = 0, lastCmdVelReceived = 0;
void cmd_vel_callback(const geometry_msgs::Twist& twist);
std_msgs::Int16 lwheel_msg;
std_msgs::Int16 rwheel_msg;
medibotv4::SensorState sensor_state_msg;
ros::NodeHandle nh;
ros::Publisher lwheel_pub("lwheel_ticks", &lwheel_msg);
ros::Publisher rwheel_pub("rwheel_ticks", &rwheel_msg);
ros::Publisher sensor_state_pub("sensor_state", &sensor_state_msg);
ros::Subscriber<geometry_msgs::Twist> cmd_vel_sub("cmd_vel", cmd_vel_callback);
//----------------------------------------------------------------------------------//

void setup(){
  pinMode(SW_MODE, INPUT_PULLUP);
  pinMode(ESTOP, INPUT_PULLUP);
  pinMode(LSR1,INPUT_PULLUP);
  pinMode(LSR2,INPUT_PULLUP);
  pinMode(LSR3,INPUT_PULLUP);
  pinMode(CS_FWD, INPUT_PULLUP);
  pinMode(CS_RVR, INPUT_PULLUP);
  pinMode(CS_LFT, INPUT_PULLUP);
  pinMode(CS_RGT, INPUT_PULLUP);
  pinMode(CS_STT, INPUT_PULLUP);
  pinMode(CS_STP, INPUT_PULLUP);
  pinMode(LH_ENA, INPUT_PULLUP);
  pinMode(LH_ENB, INPUT_PULLUP);
  pinMode(RH_ENA, INPUT_PULLUP);
  pinMode(RH_ENB, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(LH_ENA), LH_ISRA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(LH_ENB), LH_ISRB, CHANGE);
  attachInterrupt(digitalPinToInterrupt(RH_ENA), RH_ISRA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(RH_ENB), RH_ISRB, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ESTOP), EMG_STOP, RISING);
  nh.getHardware()->setBaud(115200);
  nh.initNode();
  nh.subscribe(cmd_vel_sub);
  nh.advertise(lwheel_pub);
  nh.advertise(rwheel_pub);
  nh.advertise(sensor_state_pub);
}

void loop(){
  nh.spinOnce();
  currentMillis = millis();
  //if emergency stop button is released
  if(!ESTOP){
    if(currentMillis - previousMillis >= LOOPTIME){
      previousMillis = currentMillis;

      //RESCUE MODE
      if(!digitalRead(SW_MODE)){
        if(!digitalRead(CS_FWD) && digitalRead(CS_RVR) && digitalRead(CS_LFT) && digitalRead(CS_RGT) && digitalRead(CS_STT) && digitalRead(CS_STP)){
          Move(MOTOR_SPEED, MOTOR_SPEED);//forward
        }
        else if(digitalRead(CS_FWD) && !digitalRead(CS_RVR) && digitalRead(CS_LFT) && digitalRead(CS_RGT) && digitalRead(CS_STT) && digitalRead(CS_STP)){
          Move(-MOTOR_SPEED, -MOTOR_SPEED);//reverse
        }
        else if(digitalRead(CS_FWD) && digitalRead(CS_RVR) && !digitalRead(CS_LFT) && digitalRead(CS_RGT) && digitalRead(CS_STT) && digitalRead(CS_STP)){
          Move(-DIFF_MOTOR_SPEED, DIFF_MOTOR_SPEED);//left
        }
        else if(digitalRead(CS_FWD) && digitalRead(CS_RVR) && digitalRead(CS_LFT) && !digitalRead(CS_RGT) && digitalRead(CS_STT) && digitalRead(CS_STP)){
          Move(DIFF_MOTOR_SPEED, -DIFF_MOTOR_SPEED);//right
        }
        else if(digitalRead(CS_FWD) && digitalRead(CS_RVR) && !digitalRead(CS_LFT) && digitalRead(CS_RGT) && !digitalRead(CS_STT) && digitalRead(CS_STP)){
          PAN_motor.Rotate(1, PAN_LEFT_LIM, PAN_RIGHT_LIM);//pan left
        }
        else if(digitalRead(CS_FWD) && digitalRead(CS_RVR) && digitalRead(CS_LFT) && !digitalRead(CS_RGT) && !digitalRead(CS_STT) && digitalRead(CS_STP)){
          PAN_motor.Rotate(-1, PAN_LEFT_LIM, PAN_RIGHT_LIM);//pan right
        }
        else if(!digitalRead(CS_FWD) && digitalRead(CS_RVR) && digitalRead(CS_LFT) && digitalRead(CS_RGT) && digitalRead(CS_STT) && !digitalRead(CS_STP)){
          TILT_motor.Rotate(1, TILT_UP_LIM, TILT_DOWN_LIM);//tilt up
        }
        else if(digitalRead(CS_FWD) && !digitalRead(CS_RVR) && digitalRead(CS_LFT) && digitalRead(CS_RGT) && digitalRead(CS_STT) && !digitalRead(CS_STP)){
          TILT_motor.Rotate(-1, TILT_UP_LIM, TILT_DOWN_LIM);//tilt down
        }
        else{
          Move(0, 0);//stop
          PAN_motor.Rotate(0);//stop pan
          TILT_motor.Rotate(0);//stop tilt
        }
      }

      //REMOTE MODE
      if(digitalRead(SW_MODE)){
        // To make sure the robot move differentially, else stop
        //if((demandx==0 && demandz>0)||(demandx==0 && demandz<0)||(demandx>0 && demandz==0)||(demandx<0 && demandz==0))
        if((demandx==0 && demandz!=0) || (demandz==0 && demandx!=0)){
          // Convert Linear X and Angular Z Velocity to PWM
          // Calculate left and right wheel velocity
          double  left_vel = demandx - demandz*(WHEEL_SEPARATION/2);
          double right_vel = demandx + demandz*(WHEEL_SEPARATION/2);
          // Calculate left and right sign to indicate direction
          int lsign = (left_vel>0)?1:((left_vel<0)?-1:0);
          int rsign = (right_vel>0)?1:((right_vel<0)?-1:0);
          // Map wheel velocity to PWM
          double  left_pwm = mapFloat(fabs(left_vel), 0, MAX_SPEED, MIN_PWM, MAX_PWM);
          double right_pwm = mapFloat(fabs(right_vel), 0, MAX_SPEED, MIN_PWM, MAX_PWM);
          // Actuate the motors
          Move(lsign*left_pwm,rsign*right_pwm);
        }
        else{
          Move(0,0);
        }
        //Stop the robot if there are no cmd_vel messages
        if(millis() - lastCmdVelReceived > CMD_VEL_TIMEOUT){
          demandx = 0;
          demandz = 0;
          Move(0, 0);
          PAN_motor.Rotate(0);//stop pan
          TILT_motor.Rotate(0);//stop tilt
        }
      }
    }
  }
  //if emergency stop button is being pressed
  else{
    PAN_motor.Rotate(0);
    TILT_motor.Rotate(0);
    Move(0, 0);
    LH_led.Emit('r');RH_led.Emit('r');
  }

  //Publishing data to ROS
  sensor_state_msg.ir1 = analogRead(IR1);
  sensor_state_msg.ir2 = analogRead(IR2);
  sensor_state_msg.ir3 = analogRead(IR3);
  sensor_state_msg.sonar = analogRead(SON1);
  sensor_state_msg.cliff = analogRead(CSENS);
  sensor_state_msg.laser1 = digitalRead(LSR1);
  sensor_state_msg.laser2 = digitalRead(LSR2);
  sensor_state_msg.laser3 = digitalRead(LSR3);
  sensor_state_msg.switch_mode = digitalRead(SW_MODE);
  sensor_state_msg.estop = digitalRead(ESTOP);
  sensor_state_msg.cs_stt = digitalRead(CS_STT);
  sensor_state_msg.cs_stp = digitalRead(CS_STP);
  sensor_state_msg.cs_fwd = digitalRead(CS_FWD);
  sensor_state_msg.cs_rvr = digitalRead(CS_RVR);
  sensor_state_msg.cs_lft = digitalRead(CS_LFT);
  sensor_state_msg.cs_rgt = digitalRead(CS_RGT);
  lwheel_pub.publish(&lwheel_msg);
  rwheel_pub.publish(&rwheel_msg);
  sensor_state_pub.publish(&sensor_state_msg);
}

////////////FUNCTION DEFINITIONS////////////////

void Move(int lpwm, int rpwm){
  char mv_clr = 'b', stp_clr ='w';
  stp_clr = nh.connected()? 'w':'y';

  if(lpwm==0 && rpwm==0){
    LH_led.Emit(stp_clr);RH_led.Emit(stp_clr);
  }
  else{
    if(lpwm<rpwm){
      LH_led.Emit(mv_clr);RH_led.Emit(stp_clr);
    }
    else if(lpwm>rpwm){
      LH_led.Emit(stp_clr);RH_led.Emit(mv_clr);
    }
    else{
      LH_led.Emit(mv_clr);RH_led.Emit(mv_clr);
    }
  }
  //add "-" sign to invert direction if needed
  LH_motor.Rotate(-lpwm);
  RH_motor.Rotate(rpwm);
}

void LH_ISRA(){
  lwheel_msg.data = LH_motor.doEncoderA();
}

void LH_ISRB(){
  lwheel_msg.data = LH_motor.doEncoderB();
}

void RH_ISRA(){
  rwheel_msg.data = RH_motor.doEncoderA();
}

void RH_ISRB(){
  rwheel_msg.data = RH_motor.doEncoderB();
}

void EMG_STOP(){
  PAN_motor.Rotate(0);
  TILT_motor.Rotate(0);
  Move(0, 0);
  LH_led.Emit('r');RH_led.Emit('r');
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max){
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void cmd_vel_callback( const geometry_msgs::Twist& twist){
  demandx = twist.linear.x;
  demandz = twist.angular.z;
  lastCmdVelReceived = millis();
}