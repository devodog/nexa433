/*
 * motor.h
 *
 * Created: 24.04.2021 14:03:24
 *  Author: Dag
 */ 

#ifndef MOTOR_H_
#define MOTOR_H_

#define MICRO_STEP 200
#define SEQUENCE_STEP 50

#define IDLE_INDICATION 60
#define SPEED_DIVIDER 20
#define MAX_SPEED 80

//#define DYNAMIC_SPEED_CONTROL 1
#ifndef DYNAMIC_SPEED_CONTROL
#define MINIMUM_SIGNAL_DURATION 2350   // microseconds - without dynamic speed
// adjustment...
#else
#define MINIMUM_SIGNAL_DURATION 100
// Sampling the adc takes more than one millisecond so the dynamic speed
// adjustment will not give max speed to the motor.
#endif

#define MAX_SIGNAL_DURATION ((1024/SPEED_DIVIDER)+1)

void promt();
uint8_t executeCmd(char *termInput, int cmdLength);
void noOperation();

#endif /* MOTOR_H_ */