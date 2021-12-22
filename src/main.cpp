/***************************************************
 * Simple two-dice application for M5StickC-plus
 * Including a webserver for a remote view at the result
 * 
 * Hague Nusseck @ electricidea
 * v2.0 | 22.December.2021
 * https://github.com/electricidea/M5StickC-plus_IOT-dice-game
 * 
 * 
 * Check the complete project at Hackster.io:
 * https://www.hackster.io/hague/digital-dice-with-iot-cheating-function-354ccc
 * 
 * 
 * 
 * Distributed as-is; no warranty is given.
****************************************************/

#include <Arduino.h>
#include "M5StickCPlus.h"
// install library:
// pio lib install "m5stack/M5StickCPlus"

#include "Dice_background.h"
#include "Shake_image.h"

// custom tools for WiFi accesspoint and web server
#include "web_server.h"

// size of the black dots on the dice
#define DOT_SIZE 12

// 1000ms = 1 second
#define One_Second 1000
// 60000ms = 1 minute
#define One_Minute 60000

// Array to define the position of the dots on the dice
int dot_positions[7][6][2] {
  {}, // 0
  {{55,55}}, // 1
  {{25,25},{85,85}}, // 2
  {{25,25},{55,55},{85,85}}, // 3
  {{25,25},{85,25},{25,85},{85,85}}, // 4
  {{25,25},{85,25},{55,55},{25,85},{85,85}},// 5
  {{25,25},{25,55},{25,85},{85,25},{85,55},{85,85}}, // 6
};


// Display brightness level
// possible values: 7 - 15
uint8_t screen_brightness = 10; 
// screen orientation
// 0 = up / 1 = left / 2 = down / 3 = right
uint8_t screen_orientation = 1;

// state machine
//  0 = start with printing text
#define START_STATE 0
// 10 = waiting for start shaking
#define WAIT_STATE 10
// 20 = generate random numbers and wait for stop shaking
#define SHAKE_STATE 20
// 30 = display dice #1 value
#define DISPLAY1_STATE 30
// 40 = display dice #2 value
#define DISPLAY2_STATE 40
// 50 = wait for button press to start new game
#define BUTTON_STATE 50
int process_State = 0;

// time until the next event
unsigned long next_event_time = 0;

// values for both dice
int dice1_value;
int dice2_value;

// avarage differential value from the acceleration sensor
double mean_accX_d = 0;
// number of average values for acceleration calculation
// Moving average filter value:
// The higher the value for the number of values to be 
// averaged, the more high-frequency signals are filtered 
// out, but the latency increases.
// 10 = good shaking detection with no knocking detection
// 5 = faster detection. Still stable. More sensitiv against knocking
// 1 = fast response but false positive detection by knocking
int mean_accX_n = 10;

// forward declaration
float get_horizontal_shaking();
void draw_dice(int16_t x, int16_t y, int dice_value);

void setup() {
  M5.begin();
  // screen brightness (7-15)
  M5.Axp.ScreenBreath(10); 
  M5.Imu.Init();
  // init display form text display
  M5.Lcd.setRotation(screen_orientation);
  M5.Axp.ScreenBreath(screen_brightness);
  M5.Lcd.setTextColor(TFT_WHITE);  
  M5.Lcd.setTextDatum(TC_DATUM);
  M5.Lcd.drawString("Digital DICE", (int)(M5.Lcd.width()/2), 18, 4);
  // Byte Order for pushImage()
  // need to be set "true" to get the right color coding
  M5.Lcd.setSwapBytes(true);
  // set the start value for the dice-web-page
  dice1_html_value = '-';
  dice2_html_value = '-';
  // init and start the web-server for the dice-web-page
  web_server_init();
  delay(3000);
}


void loop() {
  M5.update();
	// current time in milliseconds
	unsigned long currentMillis = millis();

  // button B to change the orientation
  if(M5.BtnB.wasPressed()){
    // 1 = right handed
    // 3 = left handed
    if(screen_orientation == 1)
      screen_orientation = 3;
    else
      screen_orientation = 1;
    M5.Lcd.setRotation(screen_orientation); 
    process_State = START_STATE;
    next_event_time = currentMillis;
  }

  // state machine cases
  switch(process_State) {
    // ******** START_STATE = start with printing text
    case START_STATE  :
      // if time for event is reached
    	if (currentMillis >= next_event_time) {
        M5.Lcd.pushImage(0, 0, Shake_image_WIDTH, Shake_image_HEIGHT, (uint16_t *)Shake_image);
        // next state
        process_State = WAIT_STATE;
        next_event_time = currentMillis + 0.5*One_Second;
      }
    break;

    // ******** WAIT_STATE = waiting for acceleration (start shaking)
    case WAIT_STATE  :
      if (currentMillis > next_event_time){
        // looking for start shaking
          if (get_horizontal_shaking() > 3.0) {
            M5.Lcd.fillScreen(TFT_BLACK);
            process_State = SHAKE_STATE;
            next_event_time = currentMillis + 0.2*One_Second;
          }
      }
    break;

    // ******** SHAKE_STATE = generate random numbers and waiting for stop shaking
    case SHAKE_STATE  :
      if (currentMillis > next_event_time){
        // rolling the dice
        dice1_value = random(1, 7);
        dice1_html_value = '0'+dice1_value;
        dice2_value = random(1, 7);
        dice2_html_value = '0'+dice2_value;
        // looking for stop shaking
        if (get_horizontal_shaking() < 1.0) {
          process_State = DISPLAY1_STATE;
          next_event_time = currentMillis + 0.2*One_Second;
        }
      }
    break;

    // ******** DISPLAY1_STATE = display dice #1
    case DISPLAY1_STATE  :
      if (currentMillis > next_event_time){
        M5.Lcd.fillRect(0, 0, M5.Lcd.width(), M5.Lcd.height(), WHITE);
        draw_dice(12,9,dice1_value);
        process_State = DISPLAY2_STATE;
        next_event_time = currentMillis + 0.5*One_Second;
      }
    break;

    // ******** DISPLAY2_STATE = display dice #2
    case DISPLAY2_STATE  :
      if (currentMillis > next_event_time){
        draw_dice(125,9,dice2_value);
        process_State = BUTTON_STATE;
        next_event_time = currentMillis + 1*One_Second;
      }
    break;

    // ******** BUTTON_STATE = wait for button press to start new roll
    case BUTTON_STATE  :
      if (currentMillis > next_event_time){
        if(M5.BtnA.isPressed()){
          dice1_html_value = 'X';
          dice2_html_value = 'X';
          process_State = START_STATE;
          next_event_time = currentMillis + 0.1*One_Second;
        }
      }
    break;
  }
  // check for web browser requests
  web_server_update();
  delay(20);
}


/***************************************************************************************
* Function name:          draw_dice
* Description:            function to draw dots of a die
* Parameter:              x, y = top / left origin of the die
*                         dice_value = number on the die
***************************************************************************************/
void draw_dice(int16_t x, int16_t y, int dice_value) {
  // M5StickC Display size is 80x160
  // M5StickC-plus Display size is 135x240
  // Dice size is 110x110
  M5.Lcd.pushImage(x, y, 110, 110, (uint16_t *)Dice_background);
  if(dice_value > 0 && dice_value < 7){
    for(int dot_index = 0; dot_index < dice_value; dot_index++) {
      M5.Lcd.fillCircle(x+dot_positions[dice_value][dot_index][0], 
                        y+dot_positions[dice_value][dot_index][1], 
                        DOT_SIZE, TFT_BLACK);
    }  
  }

}


/***************************************************************************************
* Function name:          get_horizontal_shaking
* Description:            function to get a horizontal movement value
***************************************************************************************/
float get_horizontal_shaking(){
  // two values of X-acceleration
  float accX_1, accX_2;
  // Y and Z values are needed for function call
  float accY, accZ;
  // differential value of X-acceleration
  float accX_d = 0;
  // get the first and the second sensor data
  M5.Imu.getAccelData(&accX_1,&accY,&accZ);
  delay(100);
  M5.Imu.getAccelData(&accX_2,&accY,&accZ);
  // calculate the absolute differential value
  accX_d = fabs(accX_2 - accX_1);
  // building the mean value (Moving average calculation)
  mean_accX_d = ((mean_accX_d * (mean_accX_n-1)) + accX_d)/mean_accX_n;
  return mean_accX_d;
}
