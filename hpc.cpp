// 
// 
// 
#include <Adafruit_MCP23017.h>

#include "hpc.h"

void HPC_class::HPC()
{
  //Startup Button Chip (MCP23017)
  mcp.begin();
  //Tie both Interrupts together and make active interrupt low
  mcp.setupInterrupts(true, false, LOW);
  fanStatus = ON;
  pumpStatus = ON;
  compressorStatus = ON;
  alarmStatus = ON;
  reverserStatus = ON;
  setRelay(pinFAN, OFF);
  setRelay(pinPUMP, OFF);
  setRelay(pinCOMPRESSOR, OFF);
  setRelay(pinALARM, OFF);
  setRelay(pinREVERSER, OFF);

  mcp.pinMode(pinFAN, OUTPUT);
  mcp.pinMode(pinALARM, OUTPUT);
  mcp.pinMode(pinCOMPRESSOR, OUTPUT);
  mcp.pinMode(pinREVERSER, OUTPUT);
  mcp.pinMode(pinPUMP, OUTPUT);
  
  for (int i = 0; i < 8; i++)
  {
    mcp.pullUp(i, true);
    mcp.pinMode(i, INPUT);
  }
/*  This is accomplished by the above loop
  mcp.pinMode(mcpFlowNorm, INPUT);
  mcp.pinMode(mcpFlow, INPUT);
  mcp.pinMode(mcpCond, INPUT);
  mcp.pinMode(mcpCondNorm, INPUT);
  mcp.pinMode(mcpUp, INPUT);
  mcp.pinMode(mcpDn, INPUT);
  mcp.pinMode(mcpMode, INPUT);
  mcp.pinMode(mcpFan, INPUT);
  */

  //Start the sensors.
  sensors.begin();
  sensors.setWaitForConversion(false);
  sensors.setCheckForConversion(true);

  ValidateSensors();

}

void HPC_class::ChangeMode(uint8_t newMode)
{

}

void HPC_class::ValidateSensors()
{
  Serial.print("Number of devices = "); Serial.println(sensors.getDeviceCount());
  while (sensors.getDeviceCount!=4)
  {
    //We should see exactly 4 sensors
    delay(250);
    Serial.print("Number of devices = "); Serial.println(sensors.getDeviceCount());
    delay(250);
  }
  Serial.print("Room Resolution is "); Serial.println(sensors.getResolution(room));
  Serial.print("Output Resolution is "); Serial.println(sensors.getResolution(output));
  Serial.print("Water IN Resolution = "); Serial.println(sensors.getResolution(waterin));
  Serial.print("Water OutResolution is "); Serial.println(sensors.getResolution(waterout));
  delay(10000);
}

void HPC_class::RequestTemperatures()
{
  sensors.requestTemperatures();
}

bool HPC_class::ConversionReady()
{
  return sensors.isConversionComplete();
}
void HPC_class::setRelay(uint8_t relay, bool state)
{
  Serial.println("Entering setRelay");
  switch (relay) {
  case pinCOMPRESSOR:
  {
    if (compressorStatus != state) {
      Serial.print("Compressor State Change ");
      mcp.digitalWrite(pinCOMPRESSOR, state);
      compressorStatus = state;
      if (state==OFF)
      {
        //Assuming here the compressor is stopping
        Serial.print("Turning off Compressor Relay ");
        last_compressor_run = millis();
      }
      else
      {
        last_start = millis();
        Serial.print("Turning on Compressor Relay ");
      }
    }

    break;
  }
  case pinFAN:
  {
    if (fanStatus != state) {
      mcp.digitalWrite(pinFAN, state);
      fanStatus = state;
      Serial.print("Changing FAN Relay State ");
    }

    break;
  }
  case pinREVERSER:
  {
    if (reverserStatus != state) {
      mcp.digitalWrite(pinREVERSER, state);
      reverserStatus = state;
      Serial.print("Changing Mode Relay State ");
    }




    break;
  }
  case pinPUMP:
  {
    if (pumpStatus != state) {
      mcp.digitalWrite(pinPUMP, state);
      pumpStatus = state;
      Serial.print("Changing Valve Relay State ");
    }

    break;
  }
  case pinALARM:
  {
    if (alarmStatus != state) {
      mcp.digitalWrite(pinALARM, state);
      alarmStatus = state;
      Serial.print("Changing Alarm Relay State");
    }
  }
  }
}

void HPC_class::isrRoutine()
{
  //This is where we deal with the buttons on the mcp.
  uint8_t interrupt_pin;
  bool pin_state;

  interrupt_pin = mcp.getLastInterruptPin();
  pin_state = mcp.getLastInterruptPinValue();
  
}
void HPC_class::idle_unit()
{
  setRelay(pinCOMPRESSOR, OFF);
  setRelay(pinPUMP, OFF);
  setRelay(pinREVERSER, OFF);
  setRelay(pinFAN, OFF);
  return;
}

void HPC_class::setFan()
{
  //Back half of the fan scheduler.
  if (fan_scheduler.cmd_en)
  {
    setRelay(pinFAN, fan_scheduler.cmd);
    fan_scheduler.cmd_en = false; //Disable the scheduler since the fan command has been run.
    if (fan_scheduler.cmd == OFF)
      op_state = OP_IDLE;
    else
      op_state = OP_RUN;
  }
}

void HPC_class::checkAlarms()
{
  if ((op_state > 0) && (op_state < 254))
  {
    //Alarms that need to be checked at any time the unit is running
    if (mcp.digitalRead(mcpFlow)!=flowNorm)
    {
      //Assuming no water flow!
      Serial.println("Flow Error");
      op_state = OP_ALARM;
      idle_unit();
      set_alarm(ERR_NO_FLOW, true);
    }
    if (mcp.digitalRead(mcpCond) != condNorm)
    {
      Serial.println("Condensate Error");
      op_state = OP_ALARM;
      idle_unit();
      set_alarm(ERR_COND_OVERFLOW, true);
    }
  }
  if (((millis() - last_start) > 10000) && ((op_state > 0) && (op_state < 254)))
  {
    //Alarms that need to be checked after the unit has been running awhile
    if (WaterOUTTemp > water_out_max)
    {
      //Output Water Temp Limit Exceeded
      Serial.println("Temp Limit Exceeded");
      op_state = OP_ALARM;
      idle_unit();
      set_alarm(ERR_WATER_OUT_HIGH, true);
    }
    else if (WaterOUTTemp < water_out_min)
    {
      //Output water temp below threshold
      Serial.println("Freeze Warning");
      op_state = OP_ALARM;
      idle_unit();
      set_alarm(ERR_WATER_OUT_LOW, true);

    }
    if (!waterIN_DISABLED)
    {
      WaterDelta = abs(WaterOUTTemp - WaterINTemp);
      if (WaterDelta > water_delta_limit)
      {
        //Delta Exceeded!
        Serial.println("Delta Exceeded");
        op_state = OP_ALARM;
        idle_unit();
        set_alarm(ERR_DELTA_HIGH, true);
      }
    }
    else
      WaterDelta = 255; //Sensor is bad so we need to feed a bad reading
  }

}

void HPC_class::set_alarm(uint8_t mask, bool state)
{
  switch (mask)
  {
  case ERR_COND_OVERFLOW:
    alarm_mask ^= ERR_COND_OVERFLOW;
    break;
  case ERR_NO_FLOW:
    break;
  case ERR_WATER_OUT_HIGH:
    break;
  case ERR_WATER_OUT_LOW:
    break;
  case ERR_DELTA_HIGH:
    break;
  case ERR_WATER_OUT_BAD:
    break;
  case ERR_ROOM_SENSOR:
    break;
  case ERR_WATER_IN_SENSOR:
    break;


  }
}

void HPC_class::evaluate()
{
  //This is where we check states and make changes

  switch (mode)
  {
  case MODE_OFF:
    if ((op_state > 0) && (op_state<254))
    {
      idle_unit();
    }
    break;
  case MODE_HEAT:
    switch (op_state)
    {
    case OP_ALARM:
      if ((op_state > 0) || (op_state < 254))
        idle_unit(); //If we are in alarm and the unit (we should never be here)
      break;
    case OP_WAIT:  //If we are in wait status the we haven't started yet so we are actually idle!
    case OP_IDLE:
      //We're not running at this time.  If temp dictates we need to start the unit.
      if (RoomTemp < (setpoint - hysteresis))
      {
        //Need to start the unit!
        if ((millis() - (last_compressor_run)) > COMPRESSOR_COOLDOWN)
        {
          //We can start the compressor!
          setRelay(pinPUMP, ON);
          setRelay(pinCOMPRESSOR, ON);
          op_state = OP_WARMUP; //Let the display know we're letting the coils come to temperature before turning on the fan.
          if (auto_fan)
          {
            fan_scheduler.cmd = ON;
            fan_scheduler.cmd_en = true;
            fan_scheduler.cmd_time = millis();
            fan_scheduler.delay = 30000; //Set Delay to 30 seconds
          }
          else
            setRelay(pinFAN, ON);
          //We don't need to activate the reverser, but check to make sure its off
          if (reverserStatus == ON)
            setRelay(pinREVERSER, OFF);

        }
        else
        {
          //Need to wait on the compressor to be ready
          op_state = OP_WAIT; //This lets the other functions know we have demanded service, but are not yet running.
        }

      }
    case OP_RUN:
      if (RoomTemp > (setpoint + hysteresis))
      {
        //Need to stop the unit!
        setRelay(pinCOMPRESSOR, OFF);
        last_compressor_run = millis();
        setRelay(pinPUMP, OFF);
        if (auto_fan)
        {
          fan_scheduler.cmd_en = true;
          fan_scheduler.cmd = OFF;
          fan_scheduler.cmd_time = millis();
          fan_scheduler.delay = 30000;
        }

        else
          setRelay(pinFAN, ON);

      }


    }
    break;
  case MODE_COOL:
    Serial.print("Entering evaluate.mode_cool SP="); Serial.print(setpoint); Serial.print(" Room Temp="); Serial.print(RoomTemp); Serial.print(" Mode="); Serial.print(mode); Serial.print(" OP-State="); Serial.println(op_state);
    switch (op_state)
    {
    case OP_ALARM:
      Serial.println("Entering OP_ALARM");
      if ((op_state > 0) || (op_state < 254))
        idle_unit(); //If we are in alarm and the unit (we should never be here)
      break;
    case OP_WAIT:  //If we are in wait status the we haven't started yet so we are actually idle!
      Serial.println("Entering OP_WAIT");
    case OP_IDLE:
      Serial.println("Entering OP_IDLE");
      //We're not running at this time.  If temp dictates we need to start the unit.
      if (RoomTemp > (setpoint + hysteresis))
      {
        //Need to start the unit!
        Serial.print("Time since last run = "); Serial.println((millis() - last_compressor_run));
        if ((millis() - (last_compressor_run)) > COMPRESSOR_COOLDOWN)
        {
          //We can start the compressor!
          Serial.println("We can start the compressor");
          setRelay(pinPUMP, ON);
          setRelay(pinREVERSER, ON);
          delay(10);
          setRelay(pinCOMPRESSOR, ON);
          op_state = OP_RUN; //Let the display know we're letting the coils come to temperature before turning on the fan.
          if (auto_fan)
          {
            fan_scheduler.cmd = ON;
            fan_scheduler.cmd_en = true;
            fan_scheduler.cmd_time = millis();
            fan_scheduler.delay = 500; //this should be 30 seconds
          }
          else
            setRelay(pinFAN, ON);


        }
        else
        {
          //Need to wait on the compressor to be ready
          op_state = OP_WAIT; //This lets the other functions know we have demanded service, but are not yet running.
        }

      }
      else if (op_state == OP_WAIT)
        op_state = OP_IDLE; //If we made it here the call for cooling ended before the compressor was ready so reset to idle.
      break;
    case OP_RUN:
      Serial.println("Entering OP_RUN");
      if (RoomTemp < (setpoint - hysteresis))
      {
        Serial.println("Entering OP_RUN (STOPPING)");
        //Need to stop the unit!
        setRelay(pinCOMPRESSOR, OFF);
        setRelay(pinREVERSER, OFF);
        last_compressor_run = millis();
        setRelay(pinPUMP, OFF);
        if (auto_fan)
        {
          setRelay(pinFAN, OFF);
        }

        else
          setRelay(pinFAN, ON);
        op_state = OP_IDLE;
      }
      break;

    }
    break;
  case MODE_FP:
    mode = MODE_HEAT;
    setpoint = 45;
    break;
  case MODE_FAN:
    break;
  default:
    break;

  }


  
}

void HPC_class::ReadTemperatures(bool init)
{
  if (!init)
  {
    float interim;

    interim = sensors.getTempF(room);
    if ((interim > 1) && (interim < 150))
    {
      //Sensor value in reasonable limits
      //(weight*temp_temp) + ((1 - weight)*data[0]);
      RoomTemp = (WEIGHT*interim) + ((1 - WEIGHT)*RoomTemp);
    }

    interim = sensors.getTempF(output);
    if ((interim > 1) && (interim < 150))
    {
      //Sensor value in reasonable limits
      //(weight*temp_temp) + ((1 - weight)*data[0]);
      OutputTemp = (WEIGHT*interim) + ((1 - WEIGHT)*OutputTemp);
    }

    interim = sensors.getTempF(waterin);
    if ((interim > 1) && (interim < 150))
    {
      //Sensor value in reasonable limits
      //(weight*temp_temp) + ((1 - weight)*data[0]);
      WaterINTemp = (WEIGHT*interim) + ((1 - WEIGHT)*WaterINTemp);
    }

    interim = sensors.getTempF(waterout);
    if ((interim > 1) && (interim < 150))
    {
      //Sensor value in reasonable limits
      //(weight*temp_temp) + ((1 - weight)*data[0]);
      WaterOUTTemp = (WEIGHT*interim) + ((1 - WEIGHT)*WaterOUTTemp);
    }
  }
  else
  {
    RoomTemp = sensors.getTempF(room);
    OutputTemp = sensors.getTempF(output);
    WaterINTemp = sensors.getTempF(waterin);
    WaterOUTTemp = sensors.getTempF(waterout);
  }
}


HPC_class Hpc;

