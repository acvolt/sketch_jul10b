//#include <Adafruit_NeoPixel.h>
#include <U8x8lib.h>
#include <U8g2lib.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_FRAM_SPI.h>

//#include <Stream.h>

#include "hpc.h"
unsigned long lastconversion = millis();
HPC_class hpc;


U8G2_SH1106_128X64_NONAME_1_4W_HW_SPI u8x8(U8G2_R0, 10, 6, 7);
//Adafruit_NeoPixel strip = Adafruit_NeoPixel(17, 9, NEO_GRB + NEO_KHZ800);
char lm34[10];
uint8_t three = 0;

void setup()
{
  Serial.begin(115200);

  hpc.HPC();

//  strip.begin();

  u8x8.begin();
  u8x8.setFlipMode(1);
  u8x8.setFont(u8g2_font_profont12_mr);

  while (!hpc.ConversionReady())
  {
    delay(10);
  }
  hpc.ReadTemperatures(true);
  Serial.println(hpc.RoomTemp);
  lastconversion = millis();
  delay(400);

  /* add setup code here */
  attachInterrupt(3, isrRoutine, FALLING); //This watches state changes from the MCP23017
  

}

void isrRoutine(void)
{
  hpc.isrRoutine();
}

void loop()
{

  if (hpc.fan_scheduler.cmd_en)
  {
    if ((millis() - hpc.fan_scheduler.cmd_time) >= hpc.fan_scheduler.delay)
      hpc.setFan();
  }
  if (hpc.ConversionReady())
  {
    
    hpc.ReadTemperatures(false);
    hpc.RequestTemperatures();
    hpc.evaluate();
    hpc.checkAlarms();
//    Serial.print("Mode="); Serial.print(hpc.mode); Serial.print(" ");
  //  Serial.print("millis:"); Serial.print(millis() - lastconversion); Serial.print(":");
//    Serial.println(hpc.RoomTemp);
//    lastconversion = millis();
  }
  display_driver();
  delay(100);
  
  /* add main program code here */

}


void display_driver()
{
  u8x8.firstPage();
  do {
    //First thing will be to draw the status line
    u8x8.setFont(u8g2_font_profont10_mr);
    if (hpc.fanStatus == ON) {
      u8x8.setFontMode(0);
      u8x8.setDrawColor(0);
      u8x8.drawStr(0, 8, "FAN");
      u8x8.setFontMode(1);
      u8x8.setDrawColor(1);
    }
    if (hpc.reverserStatus == OFF)
      u8x8.drawStr(20, 8, "HEAT");
    else
      u8x8.drawStr(20, 8, "COOL");
    if (hpc.compressorStatus == OFF)
      u8x8.drawStr(46, 8, "COMP");
    if (hpc.pumpStatus == ON)
      u8x8.drawStr(71, 8, "PUMP");
    if (hpc.alarmStatus == ON)
      u8x8.drawStr(95, 8, "ALARM");



    u8x8.setFont(u8g2_font_profont29_mr);
    //    u8x8.drawFrame(2, 12, u8x8.getStrWidth(lm34) + 2, 15);
    //    Serial.println("Draw Frame");

    dtostrf((hpc.RoomTemp), 4, 1, lm34);
    u8x8.drawStr(0, 32, lm34);
    u8x8.drawHLine(0, 34, u8x8.getStrWidth(lm34));
    u8x8.setCursor(90, 32);
    u8x8.print(hpc.setpoint);
    u8x8.drawHLine(90, 34, 38);
    //  u8x8.drawStr(90, 23, (int)setpoint);
    u8x8.setFont(u8g2_font_profont15_mr);
    u8x8.drawStr(11, 47, "Room");
    u8x8.setFont(u8g2_font_profont15_mr);
    u8x8.drawStr(97, 47, "Set");

    u8x8.setFont(u8g2_font_profont12_mr);
    u8x8.drawStr(0, 64, "H20 Out:");
    u8x8.setFont(u8g2_font_profont15_mr);
    dtostrf((hpc.WaterOUTTemp), 4, 1, lm34);
    u8x8.drawStr(u8x8.getStrWidth("H20 Out:") - 5, 64, lm34);
    u8x8.drawTriangle(90, 58, 95, 64, 85, 64);
    dtostrf((hpc.WaterDelta), 4, 1, lm34);
    u8x8.drawStr(100, 64, lm34);
    //    Serial.println("Draw String");
    if (!(hpc.pumpStatus) || !(hpc.compressorStatus) || !(hpc.fanStatus)) {
      if (three == 0)
        u8x8.drawXBM(64, 13, 24, 24, fan_raw_icon0_bits);
      else if (three == 1)
        u8x8.drawXBM(64, 13, 24, 24, fan_raw_icon1_bits);
      else
        u8x8.drawXBM(64, 13, 24, 24, fan_raw_icon2_bits);
    }
    switch (hpc.mode)
    {
    case  MODE_OFF:
      u8x8.setFont(u8g2_font_profont15_mr);
      u8x8.drawStr(60, 45, "OFF");
      break;
    case MODE_FAN:
      u8x8.setFont(u8g2_font_profont15_mr);
      u8x8.drawStr(60, 45, "FAN");
      break;
    case MODE_HEAT:
      u8x8.setFont(u8g2_font_profont15_mr);
      u8x8.drawStr(60, 45, "HEAT");
      break;
    case MODE_COOL:
      u8x8.setFont(u8g2_font_profont15_mr);
      u8x8.drawStr(60, 45, "COOL");
      break;
    case MODE_FP:
      u8x8.setFont(u8g2_font_profont15_mr);
      u8x8.drawStr(60, 45, "PROTECT");
      break;
    }
    //    u8x8.drawDisc(i, 40, 10);
    //    Serial.print("I="); Serial.print(i);
    //    Serial.println("  Draw Disc");


  } while (u8x8.nextPage());
  three = three + 1;
  if (three >2)
    three = 0;
}
