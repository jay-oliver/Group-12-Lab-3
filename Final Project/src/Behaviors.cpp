#include <Romi32U4.h>
#include "Behaviors.h"
#include "Speed_controller.h"
#include "Position_estimation.h"
#include "Wall_following_controller.h"
#include "Median_filter.h"
#include "IMU.h"
#include "Encoders.h"

//sensors
IMU_sensor LSM6;
Romi32U4ButtonA buttonA;
Encoder encoder;
//median filter
MedianFilter med_x;
MedianFilter med_y;
MedianFilter med_z;

//motor-speed controller
SpeedController PIcontroller;
WallFollowingController wallfollow;

//state machine
enum ROBOT_STATE {STEP1, STEP2, STEP3, STEP4, STEP5, STEP6, STOP};
ROBOT_STATE robot_state = STEP1;




void Behaviors::Init(void)
{
    LSM6.Init();
    med_x.Init();
    med_y.Init();
    med_z.Init();
    PIcontroller.Init();
    wallfollow.Init();
}

boolean Behaviors::DetectCollision(void)
{
    auto data_acc = LSM6.ReadAcceleration();
    data[0] = med_x.Filter(data_acc.X)/**0.061*/;
    data[1] = med_y.Filter(data_acc.Y)/**0.061*/;
    data[2] = med_z.Filter(data_acc.Z)/**0.061*/;
    Serial.println(data[2]);
    if((data[2] > threshold_high) || (data[2] < threshold_low)){
         return true;
    }
    else 
        return false;
}

boolean Behaviors::DetectBeingPickedUp(void)
{
    auto data_acc = LSM6.ReadAcceleration();
    data[0] = med_x.Filter(data_acc.X)/**0.061*/;
    data[1] = med_y.Filter(data_acc.Y)/**0.061*/;
    data[2] = med_z.Filter(data_acc.Z)/**0.061*/;
    if( data_acc.Z >= threshold_pick_up){
        Serial.println("I was picked up");
        return true;
    }
    else{
        return false;
    }
}

boolean Behaviors::DetectOffRamp(void){
     auto data_acc = LSM6.ReadAcceleration();
    data[0] = med_x.Filter(data_acc.X)/**0.061*/;
    data[1] = med_y.Filter(data_acc.Y)/**0.061*/;
    data[2] = med_z.Filter(data_acc.Z)/**0.061*/;
     Serial.println(data_acc.Z);
    if( data_acc.Z <= threshold_off_ramp || data_acc.Z >= threshold_off_high){
        Serial.println(data_acc.Z);
        return true;
    }
    else{
        return false;
    }
}

boolean Behaviors::DetectOnRamp(void){
    auto data_acc = LSM6.ReadAcceleration();
    data[0] = med_x.Filter(data_acc.X)/**0.061*/;
    data[1] = med_y.Filter(data_acc.Y)/**0.061*/;
    data[2] = med_z.Filter(data_acc.Z)/**0.061*/;
    Serial.println(data_acc.Z);
    if( data_acc.Z >= threshold_on_ramp_high || data_acc.Z <= threshold_on_ramp_low){
        Serial.println(data_acc.Z);
        return true;
    }
    else{
        return false;
    }
}
void Behaviors::Stop(void)
{
    PIcontroller.Stop();
    Serial.println("stopped");
}

void Behaviors::Ramp(int var){
  PIcontroller.Ramp(var);
}
void Behaviors::Move(int speed, int time){
    if (buttonA.getSingleDebouncedRelease()){
        PIcontroller.Straight(speed, time);
    }
}

void Behaviors::FollowWall(void){
    float speed = wallfollow.Process(40);
    PIcontroller.Process(50+speed, 50-speed);
}
void Behaviors::Turn(int degree, int direction){
  if (buttonA.getSingleDebouncedRelease()){
  PIcontroller.Turn(degree, direction);
  }
}

void Behaviors::Go(int left, int right){
  PIcontroller.Run(left, right);
}
void Behaviors::Run(void){

    switch (robot_state)
    {
    case STEP1:
      ////Load Object onto the robot and press button
      if(buttonA.getSingleDebouncedRelease())
      {
        Serial.println("Step One Completed Successfully");
        now = millis();
        //Can we please have a happy beep or something that'd be adorable
        robot_state = STEP2;
      }
      else{
        robot_state = STEP1;
      }
      break;
    case STEP2:
      ////After one second, move straight until you collide with the wall, then stop
      //millis until one second
     
      if (millis()- now >= 1000){
        timerUp = true;  
        if(timerUp){
            if(DetectCollision()){
                PIcontroller.Stop();
                Serial.println("Step 2 complete");
                robot_state = STEP3;
            }
            else{
                PIcontroller.Run(200, 200);
                Serial.println("no collision yet");
                robot_state = STEP2;
            }
      }
    }
   else{
        timerUp = false;
         Serial.println("waiting for 1 sec");
    }
      break;
    case STEP3:
      ////Remove object from robot, place another object onto the robot, press button
      if(buttonA.getSingleDebouncedRelease()){
        Serial.println("Step Three Completed Successfully");
        //Can we please have a happy beep or something that'd be adorable
        robot_state = STEP4;
        PIcontroller.Turn(90, 0);
        FollowWall();
        step4Time = millis();
      }
      else{
        robot_state = STEP3;
        Serial.println("Please press button");
      }
    break;
    case STEP4:
      ////Rotate 90 degrees, follow wall around the corner
      if(millis()-step4Time <=17000){
        Serial.println("waited to wall follow");
        float speed = wallfollow.Process(40);
        PIcontroller.Process(50+speed, 50-speed);
        robot_state = STEP4;
      }
      else //(DetectOnRamp())
      {
        Serial.println("Step 4 completed");
        step5Time = millis();
        robot_state = STEP5;
      }
      /*else{
        float speed = wallfollow.Process(40);
      PIcontroller.Process(50+speed, 50-speed);
        robot_state = STEP4;
    }*/
      break;
    case STEP5:
      ////Pass the ramp, then continue to follow the wall for another 10 cm
      if(/*DetectOffRamp()*/millis()-step5Time >= 15000){
        PIcontroller.Stop();
        Serial.println("Step 5 completed");
        robot_state = STEP6;
      }
      else{
        Serial.println("ramp");
        PIcontroller.Ramp(100);
        robot_state = STEP5;
      }
      break;
    case STEP6:
    PIcontroller.Straight(25, 4);
    PIcontroller.Stop();
    Serial.println("Step 6 completed");
    robot_state = STEP1;
      ////Stop, and remove object from robot
      //Happy happy beep maybe possibly?
    break;
    case STOP:
    PIcontroller.Stop();
    break;
}

}