/*
 * BinClock.c
 * Jarrod Olivier
 * Modified by Keegan Crankshaw
 * Further Modified By: Mark Njoroge
 *
 * <MCKLAU003> <GRNMAT017>
 * Date 20/08/2021
*/

#include <signal.h> //for catching signals
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdio.h> //For printf functions
#include <stdlib.h> // For system functions

#include "BinClock.h"
#include "CurrentTime.h"

//Global variables
int hours, mins, secs;
long lastInterruptTime = 0; //Used for button debounce
int RTC; //Holds the RTC instance

int HH,MM,SS;


// Clean up function to avoid damaging used pins
void CleanUp(int sig){
	printf("Cleaning up\n");

	//Set LED to low then input mode
	digitalWrite(LED, LOW); //Set LED low
	pinMode(LED, INPUT); //Set LED to input mode


	for (int j=0; j < sizeof(BTNS)/sizeof(BTNS[0]); j++) {
		pinMode(BTNS[j],INPUT);
	}

	exit(0);

}

void initGPIO(void){
	/*
	 * Sets GPIO using wiringPi pins. see pinout.xyz for specific wiringPi pins
	 * You can also use "gpio readall" in the command line to get the pins
	 * Note: wiringPi does not use GPIO or board pin numbers (unless specifically set to that mode)
	 */
	printf("Setting up\n");
	wiringPiSetup(); //This is the default mode


	RTC = wiringPiI2CSetup(RTCAddr); //Set up the RTC

	//Set up the LED
	pinMode(LED, OUTPUT); //Set LED in output mode
	pullUpDnControl(LED,PUD_OFF); //Set LED resistor mode off


	printf("LED and RTC done\n");

	//Set up the Buttons
	for(int j=0; j < sizeof(BTNS)/sizeof(BTNS[0]); j++){
		pinMode(BTNS[j], INPUT);
		pullUpDnControl(BTNS[j], PUD_UP); //Set buttons pull-up resistor mode
}

	//Attach interrupts to Buttons
	wiringPiISR(BTNS[0],INT_EDGE_FALLING,&hourInc); //hour btn interrupt
	wiringPiISR(BTNS[1],INT_EDGE_FALLING,&minInc); //minute btn interrupt

	printf("BTNS done\n");
	printf("Setup done\n");
}


/*
 * The main function
 */
int main(void){
	signal(SIGINT,CleanUp);
	initGPIO();

	//Set random time (3:04PM)
	//You can comment this file out later
	wiringPiI2CWriteReg8(RTC, HOUR_REGISTER, 0x13+TIMEZONE);
	wiringPiI2CWriteReg8(RTC, MIN_REGISTER, 0x4);
	wiringPiI2CWriteReg8(RTC, SEC_REGISTER, 0x0);

	// Repeat this until we shut down
	for (;;){
		//Fetch the time from the RTC
		hours = hexCompensation(wiringPiI2CReadReg8(RTC, HOUR_REGISTER)); //fetch hours in dec
		mins = hexCompensation(wiringPiI2CReadReg8(RTC, MIN_REGISTER)); //fetch minutes in dec
		secs = hexCompensation(wiringPiI2CReadReg8(RTC, SEC_REGISTER)); //fetch seconds in dec

		//Toggle Seconds LED
        	digitalWrite(LED, HIGH); //LED on
        	delay(500);
        	digitalWrite(LED, LOW); //LED off

		// Print out the time we have stored on our RTC
		printf("The current time is: %d:%d:%d\n", hours, mins, secs);

		//using a delay to make our program "less CPU hungry"
		delay(500); //milliseconds
	}
	return 0;
}

/*
 * Changes the hour format to 12 hours
 */
int hFormat(int hours){
	/*formats to 12h*/
	if (hours >= 24){
		hours = 0;
	}
	else if (hours > 12){
		hours -= 12;
	}
	return (int)hours;
}


/*
 * hexCompensation
 * This function may not be necessary if you use bit-shifting rather than decimal checking for writing out time values
 * Convert HEX or BCD value to DEC where 0x45 == 0d45
 */
int hexCompensation(int units){

	int unitsU = units%0x10;

	if (units >= 0x50){
		units = 50 + unitsU;
	}
	else if (units >= 0x40){
		units = 40 + unitsU;
	}
	else if (units >= 0x30){
		units = 30 + unitsU;
	}
	else if (units >= 0x20){
		units = 20 + unitsU;
	}
	else if (units >= 0x10){
		units = 10 + unitsU;
	}
	return units;
}


/*
 * decCompensation
 * This function "undoes" hexCompensation in order to write the correct base 16 value through I2C
 */
int decCompensation(int units){
	int unitsU = units%10;

	if (units >= 50){
		units = 0x50 + unitsU;
	}
	else if (units >= 40){
		units = 0x40 + unitsU;
	}
	else if (units >= 30){
		units = 0x30 + unitsU;
	}
	else if (units >= 20){
		units = 0x20 + unitsU;
	}
	else if (units >= 10){
		units = 0x10 + unitsU;
	}
	return units;
}


/*
 * hourInc
 * Fetch the hour value off the RTC, increase it by 1, and write back
 * Be sure to cater for there only being 23 hours in a day
 * Software Debouncing should be used
 */
void hourInc(void){
	//Debounce
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		printf("Interrupt 1 triggered, %d\n", hours); //triggered hours in dec
		//Increase hours by 1, ensuring not to overflow
		if (hours > 22){ //reset hours
		hours = 0;
		}
		else{
			hours = hours + 1;
		}
		//Write hours back to the RTC
		wiringPiI2CWriteReg8(RTC, HOUR_REGISTER, decCompensation(hours)); //write hours in hex
	}
	lastInterruptTime = interruptTime;
}

/* 
 * minInc
 * Fetch the minute value off the RTC, increase it by 1, and write back
 * Be sure to cater for there only being 60 minutes in an hour
 * Software Debouncing should be used
 */
void minInc(void){
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		printf("Interrupt 2 triggered, %d\n", mins); //triggered mins in dec
                //Increase minutes by 1, ensuring not to overflow
		if (mins > 58){
			mins = 0; //reset mins to 0
			hours = hours + 1; //increment hours
                	if (hours > 23){ //reset hours to 0
                        	hours = 0;
                	}
                wiringPiI2CWriteReg8(RTC, HOUR_REGISTER, decCompensation(hours)); //write hours in hex
		}
		else{
			mins = mins + 1; //increment mins
		}
		//Write minutes back to the RTC
                wiringPiI2CWriteReg8(RTC, MIN_REGISTER, decCompensation(mins)); //write mins in hex
	}
	lastInterruptTime = interruptTime;
}

//This interrupt will fetch current time from another script and write it to the clock registers
//This functions will toggle a flag that is checked in main
void toggleTime(void){
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		HH = getHours();
		MM = getMins();
		SS = getSecs();

		HH = hFormat(HH);
		HH = decCompensation(HH);
		wiringPiI2CWriteReg8(RTC, HOUR_REGISTER, HH);

		MM = decCompensation(MM);
		wiringPiI2CWriteReg8(RTC, MIN_REGISTER, MM);

		SS = decCompensation(SS);
		wiringPiI2CWriteReg8(RTC, SEC_REGISTER, 0b10000000+SS);

	}
	lastInterruptTime = interruptTime;
}
