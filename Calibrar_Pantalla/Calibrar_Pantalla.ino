//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> 01_Screen_Display_Test_and_Touchscreen_Calibration
//---------------------------------------- Including Libraries.
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
//---------------------------------------- 

// Defines the T_CS Touchscreen PIN.
#define T_CS_PIN    13 //--> T_CS

// Defines the width and height of the screen, also the font size.
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define FONT_SIZE 2

int centerX, centerY;

byte ts_Calibration_Sequence = 1;
byte cnt = 0;

uint16_t ts_Min_X, ts_Max_X, ts_Min_Y, ts_Max_Y;
bool FT = true;

// Declaring the "TFT_eSPI" object as "tft".
TFT_eSPI tft = TFT_eSPI();

// Declaring the "XPT2046_Touchscreen" object as "touchscreen" and its settings.
XPT2046_Touchscreen touchscreen(T_CS_PIN);



//________________________________________________________________________________ Screen_Display_Test()
void Screen_Display_Test() {
  Serial.println();
  Serial.println("Screen Display Test");
  
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
  tft.drawCentreString("Screen Display Test", centerX, centerY, FONT_SIZE);
  delay(2000);
  
  for (byte i = 0; i < 4; i++) {
    tft.setRotation(i);  //--> Range from 0 to 3.

    // Clear the screen to defined colour.
    // To see the list of colors, open the file "TFT_eSPI.h" (Arduino IDE library folder -> TFT_eSPI -> TFT_eSPI.h).
    // Look at "Section 6" (on line 302).
    tft.fillScreen(TFT_WHITE);
  
    tft.setCursor(10, 10);
    tft.setTextColor(TFT_BROWN); tft.setTextSize(1);
    tft.print("LCD TFT Touchscreen");
    tft.setCursor(9, 25);
    tft.print("ILI9341 240*320 Pixel");
    tft.setTextColor(TFT_GREEN); tft.setTextSize(2);
    tft.setCursor(10, 40);
    tft.print("with ESP32");
    tft.setCursor(10, 60);
    tft.print("& TFT_eSPI Library");
    tft.setTextColor(TFT_CYAN); tft.setTextSize(2);
    tft.setCursor(10, 85);
    tft.print("UTEH STR");
  
    // Draw a filled rectangle.
    tft.fillRect(10, 110, 20, 20, TFT_PURPLE);  //--> fillRect(x, y, w, h, color);
  
    // Draw a filled circle.
    tft.fillCircle(45, 120, 10, TFT_OLIVE);  //--> fillCircle(x, y, radius, color);
  
    // Draw a filled triangle.
    tft.fillTriangle(70, 110, 60, 130, 80, 130, TFT_BLUE);  //--> fillTriangle(x0, y0, x1, y1, x2, y2, color);

    delay(1000);
  }

  tft.setTextSize(1);
}
//________________________________________________________________________________ 



//________________________________________________________________________________ get_Touchscreen_Min_X()
void get_Touchscreen_Min_X() {
  print_Title("touchscreen_Min_X");
  
  while(true) {
    // Draws a filled circle to touch so that it gets the base value touchscreen_Min_X
    tft.fillCircle(0, centerY, 5, TFT_RED);  //--> fillCircle(x, y, radius, color);

    // Checks if Touchscreen was touched, and prints X, Y and Pressure (Z) info on the TFT display and Serial Monitor.
    if (touchscreen.touched()) {
      cnt++;
      
      // Get Touchscreen points
      TS_Point p = touchscreen.getPoint();
  
      print_Val(p.x, p.y, p.z);

      if(FT == true && cnt > 1) {
        ts_Min_X = p.x;
        FT = false;
      }

      if(p.x < ts_Min_X) ts_Min_X = p.x;

      if(cnt > 30) {
        cnt = 0;
        FT = true;
        ts_Calibration_Sequence++;
        break;
      }
  
      delay(50);
    }
  }

  print_Rslt("touchscreen_Min_X", ts_Min_X);

  delay(5000);
}
//________________________________________________________________________________ 



//________________________________________________________________________________ get_Touchscreen_Max_X()
void get_Touchscreen_Max_X() {
  print_Title("touchscreen_Max_X");
  
  while(true) {
    // Draws a filled circle to touch so that it gets the base value touchscreen_Max_X
    tft.fillCircle(239, centerY, 5, TFT_RED);  //--> fillCircle(x, y, radius, color);    

    // Checks if Touchscreen was touched, and prints X, Y and Pressure (Z) info on the TFT display and Serial Monitor.
    if (touchscreen.touched()) {
      cnt++;
      
      // Get Touchscreen points
      TS_Point p = touchscreen.getPoint();
  
      print_Val(p.x, p.y, p.z);

      if(FT == true && cnt > 3) {
        ts_Max_X = p.x;
        FT = false;
      }

      if(p.x > ts_Max_X) ts_Max_X = p.x;

      if(cnt > 30) {
        cnt = 0;
        FT = true;
        ts_Calibration_Sequence++;
        break;
      }
  
      delay(50);
    }
  }

  print_Rslt("touchscreen_Max_X", ts_Max_X);

  delay(5000);
}
//________________________________________________________________________________ 



//________________________________________________________________________________ get_Touchscreen_Min_Y()
void get_Touchscreen_Min_Y() {
  print_Title("touchscreen_Min_Y");
  
  while(true) {
    // Draws a filled circle to touch so that it gets the base value touchscreen_Min_Y
    tft.fillCircle(centerX, 0, 5, TFT_RED);  //--> fillCircle(x, y, radius, color);

    // Checks if Touchscreen was touched, and prints X, Y and Pressure (Z) info on the TFT display and Serial Monitor.
    if (touchscreen.touched()) {
      cnt++;
      
      // Get Touchscreen points
      TS_Point p = touchscreen.getPoint();
  
      print_Val(p.x, p.y, p.z);

      if(FT == true && cnt > 3) {
        ts_Min_Y = p.y;
        FT = false;
      }

      if(p.y < ts_Min_Y) ts_Min_Y = p.y;

      if(cnt > 30) {
        cnt = 0;
        FT = true;
        ts_Calibration_Sequence++;
        break;
      }
  
      delay(50);
    }
  }

  print_Rslt("touchscreen_Min_Y", ts_Min_Y);

  delay(5000);
}
//________________________________________________________________________________ 



//________________________________________________________________________________ get_Touchscreen_Max_Y()
void get_Touchscreen_Max_Y() {
  print_Title("touchscreen_Max_Y");
  
  while(true) {
    // Draws a filled circle to touch so that it gets the base value touchscreen_Max_Y
    tft.fillCircle(centerX, 319, 5, TFT_RED);  //--> fillCircle(x, y, radius, color);

    // Checks if Touchscreen was touched, and prints X, Y and Pressure (Z) info on the TFT display and Serial Monitor.
    if (touchscreen.touched()) {
      cnt++;
      
      // Get Touchscreen points
      TS_Point p = touchscreen.getPoint();
  
      print_Val(p.x, p.y, p.z);

      if(FT == true && cnt > 3) {
        ts_Max_Y = p.y;
        FT = false;
      }

      if(p.y > ts_Max_Y) ts_Max_Y = p.y;

      if(cnt > 30) {
        cnt = 0;
        FT = true;
        ts_Calibration_Sequence++;
        break;
      }
  
      delay(50);
    }
  }

  print_Rslt("touchscreen_Max_Y", ts_Max_Y);

  delay(5000);
}
//________________________________________________________________________________ 



//________________________________________________________________________________ print_Title()
void print_Title(String ttl) {
  Serial.println();
  Serial.print("-----------------Get ");
  Serial.print(ttl);
  Serial.println(" value");
  Serial.println("Please touch inside the red area.");
  Serial.println();
  
  tft.fillScreen(TFT_WHITE); 
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("Get " + ttl, centerX, 20, FONT_SIZE);
  tft.drawCentreString("value", centerX, 40, FONT_SIZE);
  tft.drawCentreString("Please touch inside", centerX, (centerY - 8) - 10, FONT_SIZE);
  tft.drawCentreString("the red area.", centerX, (centerY - 8) + 10, FONT_SIZE);
}
//________________________________________________________________________________ 



//________________________________________________________________________________ print_Val()
void print_Val(uint16_t x, uint16_t y, uint16_t z) {
  //---------------------------------------- Print Touchscreen info about X, Y and Pressure (Z) on the Serial Monitor.
  Serial.print("p.x = ");
  Serial.print(x);
  Serial.print(" | p.y = ");
  Serial.print(y);
  Serial.print(" | Pressure = ");
  Serial.println(z);
  //----------------------------------------
  
  //---------------------------------------- Print Touchscreen info about X, Y and Pressure (Z) on the TFT Display.
  // Clear TFT screen
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);

  int textY = 80;
 
  String rslt_Text = "p.x = " + String(x);
  tft.drawCentreString(rslt_Text, centerX, textY, FONT_SIZE);

  textY += 20;
  rslt_Text = "p.y = " + String(y);
  tft.drawCentreString(rslt_Text, centerX, textY, FONT_SIZE);

  textY += 20;
  rslt_Text = "Pressure = " + String(z);
  tft.drawCentreString(rslt_Text, centerX, textY, FONT_SIZE);
  //----------------------------------------
}
//________________________________________________________________________________ 



//________________________________________________________________________________ print_Rslt()
void print_Rslt(String ttl, uint16_t ts_min_rslt) {
  Serial.println();
  Serial.print("Getting ");
  Serial.print(ttl);
  Serial.print(" value is complete.");
  Serial.println("Please release your touch from the screen.");

  Serial.println();
  Serial.print(ttl);
  Serial.print(" = ");
  Serial.println(ts_min_rslt);

  Serial.println();
  Serial.println("Please wait for the next process...");
  Serial.println("-----------------");

  String rslt_Text = ttl + " = " + String(ts_min_rslt);
  tft.setTextColor(TFT_BLUE, TFT_WHITE);
  tft.drawCentreString(rslt_Text, centerX, 160, FONT_SIZE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawCentreString("Please release your touch", centerX, 200, FONT_SIZE);
  tft.drawCentreString("from the screen.", centerX, 220, FONT_SIZE);
  tft.drawCentreString("Please wait for", centerX, 260, FONT_SIZE);
  tft.drawCentreString("the next process...", centerX, 280, FONT_SIZE);
}
//________________________________________________________________________________ 



//________________________________________________________________________________ VOID SETUP()
void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);
  Serial.println();
  delay(3000);

  // Set X and Y coordinates for center of display.
  centerX = SCREEN_WIDTH / 2;
  centerY = SCREEN_HEIGHT / 2;

  // Start the SPI for the touchscreen and init the touchscreen.
  touchscreen.begin();

  // Set the Touchscreen rotation in landscape mode.
  // Note: in some displays, the touchscreen might be upside down, so you might need to set the rotation to 0. "touchscreen.setRotation(0);"
  touchscreen.setRotation(2);

  tft.init();
  tft.setRotation(0);

  // Clear the screen before writing to it.
  tft.fillScreen(TFT_WHITE);

  Screen_Display_Test();

  tft.setRotation(0);

  Serial.println();
  Serial.println("Touchscreen Calibration");
  
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_LIGHTGREY);
  tft.drawCentreString("Touchscreen Calibration", centerX, centerY, FONT_SIZE);
  delay(2000);
}
//________________________________________________________________________________ 



//________________________________________________________________________________ VOID LOOP()
void loop() {
  // put your main code here, to run repeatedly:

  if (ts_Calibration_Sequence == 1) {
    get_Touchscreen_Min_X();
  } else if (ts_Calibration_Sequence == 2) {
    get_Touchscreen_Max_X();
  } else if (ts_Calibration_Sequence == 3) {
    get_Touchscreen_Min_Y();
  } else if (ts_Calibration_Sequence == 4) {
    get_Touchscreen_Max_Y();
  } else if (ts_Calibration_Sequence == 5) {
    Serial.println();
    Serial.println("-----------------Calibration results");
    Serial.println("Copy and save the calibration results below.");
    Serial.println();
    Serial.print("#define touchscreen_Min_X ");
    Serial.println(ts_Min_X);
    Serial.print("#define touchscreen_Max_X ");
    Serial.println(ts_Max_X);
    Serial.print("#define touchscreen_Min_Y ");
    Serial.println(ts_Min_Y);
    Serial.print("#define touchscreen_Max_Y ");
    Serial.println(ts_Max_Y);
    Serial.println("-----------------");

    tft.fillScreen(TFT_WHITE);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.drawCentreString("Calibration process complete.", centerX, 10, FONT_SIZE);
    tft.drawCentreString("See the results on", centerX, 30, FONT_SIZE);
    tft.drawCentreString("the serial monitor.", centerX, 50, FONT_SIZE);
    while(true) {
      delay(1000);
    }
  } 

  delay(50);
}
//________________________________________________________________________________ 
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
