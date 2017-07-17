// hpc.h

#ifndef _HPC_h
#define _HPC_h

#if defined(ARDUINO) && ARDUINO >= 100
  #include "arduino.h"
#else
  #include "WProgram.h"
#endif
#include <DallasTemperature.h>
#include <OneWire.h>
#define ON 0
#define OFF 1

#define MAX_SETPOINT 120 //Don't allow the temp setpoint to exceed this value
#define MIN_SETPOINT 40 //Don't allow the temp setpoint below this value
#define MAX_HYSTERESIS 10 //How much temp swing is allowed
#define MIN_HYSTERESIS 1 //How little temp swing is allowed

#define MAX_DELTA 15 //Max change in water temp allowed.

#define COMPRESSOR_COOLDOWN 60000 //How long between compressor runs

#define ERR_COND_OVERFLOW 0x01
#define ERR_NO_FLOW 0x02
#define ERR_WATER_OUT_HIGH 0x04
#define ERR_WATER_OUT_LOW 0x08
#define ERR_DELTA_HIGH 0x10
#define ERR_WATER_OUT_BAD 0x20
#define ERR_ROOM_SENSOR 0x40
#define ERR_WATER_IN_SENSOR 0x80 //Not a critical alarm, but does invalidate DELTA.


#define mcpUp 7   //Temp up
#define mcpDn 6   //Temp Down
#define mcpMode 4 //Mode
#define mcpFlow 3 //Flow Sensor
#define mcpCond 2 //Condensate Sensor
#define mcpFlowNorm 1 //Normal Flow Switch Position
#define mcpCondNorm 0 //Normal Condensate Overflow position
#define mcpFan 5    //Fan Mode Switch

#define pinFAN  8   //Group B #0 For FAN
#define pinREVERSER 9 //Group B #1 for Reversing Valve
#define pinCOMPRESSOR 10  //Group B #2 for Compressor Relay
#define pinPUMP 11      //Group B #3 for Pump Relay
#define pinALARM 12     //Group B #4 for Alarm

#define WEIGHT .88

class HPC_class
{
 protected:
   unsigned long mode_change_time = 0L;
   unsigned long last_start = 0L;
   unsigned long last_compressor_run = 0L;
   bool current_op_mode = 0;
   uint8_t max_setpoint = MAX_SETPOINT;
   uint8_t min_setpoint = MIN_SETPOINT;
   volatile bool waterIN_DISABLED = false;
   volatile bool flowNorm, condNorm;
   Adafruit_MCP23017 mcp;
   OneWire one_wire = OneWire(8);
   DallasTemperature sensors = DallasTemperature(&one_wire);

   const uint8_t room[8] = { 0x28, 0xEE, 0x60, 0x11, 0x21, 0x16, 0x01, 0xA5 };
   const uint8_t waterin[8] = { 0x28, 0xEE, 0x11, 0xF0, 0x20, 0x16, 0x01, 0x68 };
   const uint8_t waterout[8] = { 0x28, 0xEE, 0xB5, 0xFE, 0x1D, 0x16, 0x02, 0x7D };
   const uint8_t output[8] = { 0x28, 0xEE, 0xE8, 0x44, 0x1E, 0x16, 0x02, 0x6A };

   void setRelay(uint8_t pin, bool state);



   void idle_unit();
   void set_alarm(uint8_t mask, bool state);

 public:
  void HPC();
  void ChangeMode(uint8_t newMode);
  void ChangeSetPoint(uint8_t newSetPoint);

  //DS18B20 Methods
  void ReadTemperatures(bool init);
  void RequestTemperatures();
  bool ConversionReady();
  void ValidateSensors();

  void evaluate();
  void setFan();
  void checkAlarms();

  void isrRoutine();

  //MCP23017 Methods




  uint8_t alarm_counts[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  uint8_t mode=2;   //Mode
#define MODE_OFF 0
#define MODE_HEAT 1
#define MODE_COOL 2
#define MODE_FP 3
#define MODE_FAN 4
  uint8_t op_state;
#define OP_IDLE 0
#define OP_RUN 1
#define OP_WARMUP 2
#define OP_COOLDOWN 4
#define OP_WAIT 128
#define OP_ALARM 255
  //auto_fan true means the fan runs when required, false means fan is always on.
  bool auto_fan = true;
  bool compressorStatus, fanStatus, reverserStatus, pumpStatus, alarmStatus;
  volatile uint8_t setpoint = 75;
  volatile uint8_t water_out_max = 100;
  volatile uint8_t water_out_min = 40;
  volatile uint8_t water_delta_limit = 30;



  uint8_t hysteresis = 1;
  float RoomTemp;
  float OutputTemp;
  float WaterINTemp;
  float WaterOUTTemp;
  float WaterDelta;
  float AirDelta;

  struct alarms {
    uint8_t type = 0;
    /* Type 0
    * Type 0x01 - OverFlow
    * Type 0x02 - No Flow
    * Type 0x04 - Water Out Temp High
    * Type 0x08 - Water Out Temp Low
    * Tyep 0x10 - High Delta Temp
    * Type 0x20 - Bad Water Out Temp Sensor
    * Type 0x40 - Bad Room Temp Sensor
    * Type 0x80 - Bad Water IN Temp Sensor
    */
    int eventNumber = 0; //number of ocurrences
    char alarm[21] = "No Events Yet       ";
    unsigned long event_time;
  } alarm_one, alarm_two;
  byte alarm_mask = 0;

  struct relay_scheduler {
    unsigned long cmd_time=0L; //Time (seconds) when to set relay to cmd state.
    unsigned long delay = 0L;
    bool cmd = OFF; //What to set the relay to
    bool cmd_en = false; //Do we have this scheduled
    uint8_t mode; //See if mode changed after scheduling.
  } fan_scheduler;



};

extern HPC_class Hpc;

#endif
//#define fan_raw_icon0_bits { 0x80, 0x03, 0x00, 0xe0, 0x0f, 0x00, 0xf0, 0x0f, 0x06, 0xf8, 0x8f, 0x1f, 0xf8, 0xcf, 0x3f, 0xfc, 0xe7, 0x7f, 0xfc, 0xe7, 0x7f, 0xf8, 0xe7, 0xff, 0xf8, 0xe7, 0xff, 0xf0, 0xff, 0xff, 0xe0, 0xff, 0x7f, 0x00, 0x67, 0x78, 0x1e, 0x66, 0x00, 0xfe, 0xff, 0x03, 0xff, 0xff, 0x0f, 0xff, 0xef, 0x1f, 0xff, 0xe7, 0x1f, 0xfe, 0xe7, 0x3f, 0xfe, 0xe7, 0x3f, 0xfc, 0xf3, 0x1f, 0xf8, 0xf1, 0x1f, 0x60, 0xf0, 0x0f, 0x00, 0xf0, 0x07, 0x00, 0xc0, 0x01 }
//#define fan_raw_icon1_bits { 0x00, 0xe0, 0x03, 0x00, 0xf8, 0x07, 0x00, 0xf8, 0x0f, 0x78, 0xfc, 0x1f, 0xfc, 0xfc, 0x1f, 0xfe, 0xfc, 0x1f, 0xff, 0xfc, 0x1f, 0xff, 0xfc, 0x0f, 0xff, 0xf9, 0x00, 0xff, 0x7f, 0x00, 0xff, 0x7f, 0x1f, 0xfe, 0xe7, 0x7f, 0xfe, 0xe7, 0x7f, 0xf8, 0xfe, 0xff, 0x00, 0xfe, 0xff, 0x00, 0x9f, 0xff, 0xf0, 0x3f, 0xff, 0xf8, 0x3f, 0xff, 0xf8, 0x3f, 0x7f, 0xf8, 0x3f, 0x3f, 0xf8, 0x3f, 0x1e, 0xf0, 0x1f, 0x00, 0xe0, 0x1f, 0x00, 0xc0, 0x07, 0x00 }
//#define fan_raw_icon2_bits { 0x00, 0xff, 0x00, 0x80, 0xff, 0x00, 0xc0, 0xff, 0x01, 0xc0, 0xff, 0x01, 0xc0, 0xff, 0x00, 0xc0, 0x7f, 0x00, 0x80, 0x3f, 0x3c, 0x8c, 0x1f, 0x7f, 0x1f, 0x9f, 0xff, 0x3f, 0xfe, 0xff, 0x7f, 0xfe, 0xff, 0xff, 0xe7, 0xff, 0xff, 0xe7, 0xff, 0xff, 0x7f, 0xfe, 0xff, 0x7f, 0xfc, 0xff, 0xf9, 0xf8, 0xfe, 0xf8, 0x31, 0x3c, 0xfc, 0x01, 0x00, 0xfe, 0x03, 0x00, 0xff, 0x03, 0x80, 0xff, 0x03, 0x80, 0xff, 0x01, 0x00, 0xff, 0x01, 0x00, 0xff, 0x00 }


static const uint8_t fan_raw_icon0_bits[72] = {
  0x80, 0x03, 0x00, 0xe0, 0x0f, 0x00, 0xf0, 0x0f, 0x06, 0xf8, 0x8f, 0x1f,
  0xf8, 0xcf, 0x3f, 0xfc, 0xe7, 0x7f, 0xfc, 0xe7, 0x7f, 0xf8, 0xe7, 0xff,
  0xf8, 0xe7, 0xff, 0xf0, 0xff, 0xff, 0xe0, 0xff, 0x7f, 0x00, 0x67, 0x78,
  0x1e, 0x66, 0x00, 0xfe, 0xff, 0x03, 0xff, 0xff, 0x0f, 0xff, 0xef, 0x1f,
  0xff, 0xe7, 0x1f, 0xfe, 0xe7, 0x3f, 0xfe, 0xe7, 0x3f, 0xfc, 0xf3, 0x1f,
  0xf8, 0xf1, 0x1f, 0x60, 0xf0, 0x0f, 0x00, 0xf0, 0x07, 0x00, 0xc0, 0x01 };


static const uint8_t fan_raw_icon1_bits[72] = {
  0x00, 0xe0, 0x03, 0x00, 0xf8, 0x07, 0x00, 0xf8, 0x0f, 0x78, 0xfc, 0x1f,
  0xfc, 0xfc, 0x1f, 0xfe, 0xfc, 0x1f, 0xff, 0xfc, 0x1f, 0xff, 0xfc, 0x0f,
  0xff, 0xf9, 0x00, 0xff, 0x7f, 0x00, 0xff, 0x7f, 0x1f, 0xfe, 0xe7, 0x7f,
  0xfe, 0xe7, 0x7f, 0xf8, 0xfe, 0xff, 0x00, 0xfe, 0xff, 0x00, 0x9f, 0xff,
  0xf0, 0x3f, 0xff, 0xf8, 0x3f, 0xff, 0xf8, 0x3f, 0x7f, 0xf8, 0x3f, 0x3f,
  0xf8, 0x3f, 0x1e, 0xf0, 0x1f, 0x00, 0xe0, 0x1f, 0x00, 0xc0, 0x07, 0x00 };
static const uint8_t fan_raw_icon2_bits[72] = {
  0x00, 0xff, 0x00, 0x80, 0xff, 0x00, 0xc0, 0xff, 0x01, 0xc0, 0xff, 0x01,
  0xc0, 0xff, 0x00, 0xc0, 0x7f, 0x00, 0x80, 0x3f, 0x3c, 0x8c, 0x1f, 0x7f,
  0x1f, 0x9f, 0xff, 0x3f, 0xfe, 0xff, 0x7f, 0xfe, 0xff, 0xff, 0xe7, 0xff,
  0xff, 0xe7, 0xff, 0xff, 0x7f, 0xfe, 0xff, 0x7f, 0xfc, 0xff, 0xf9, 0xf8,
  0xfe, 0xf8, 0x31, 0x3c, 0xfc, 0x01, 0x00, 0xfe, 0x03, 0x00, 0xff, 0x03,
  0x80, 0xff, 0x03, 0x80, 0xff, 0x01, 0x00, 0xff, 0x01, 0x00, 0xff, 0x00 };

  