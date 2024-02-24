#include "_Plugin_Helper.h"
#ifdef USES_P253

// #######################################################################################################
// ########################   Plugin 253 IKEA Vindstyrka I2C  Sensor (SEN54)  ############################
// #######################################################################################################
// 19-06-2023 AndiBaciu creation based upon https://github.com/RobTillaart/SHT2x

# include "src/PluginStructs/P253_data_struct.h"

#define PLUGIN_253
#define PLUGIN_ID_253     253               // plugin id
#define PLUGIN_NAME_253   "Air Quality - Sensirion" // What will be dislpayed in the selection list
#define PLUGIN_VALUENAME1_253 "Temperature" // variable output of the plugin. The label is in quotation marks
#define PLUGIN_VALUENAME2_253 "Humidity"    // multiple outputs are supporte
#define PLUGIN_VALUENAME3_253 "PM 2.5"    // multiple outputs are supported
#define PLUGIN_VALUENAME4_253 "tVOC"    // multiple outputs are supported
#define PLUGIN_VALUENAME5_253 "PM 1.0"    // multiple outputs are supported
#define PLUGIN_VALUENAME6_253 "PM 4.0"    // multiple outputs are supported
#define PLUGIN_VALUENAME7_253 "PM 10.0"    // multiple outputs are supported
#define PLUGIN_VALUENAME8_253 "DewPoint"    // multiple outputs are supported

//   PIN/port configuration is stored in the following:
//   CONFIG_PIN1 - The first GPIO pin selected within the task
//   CONFIG_PIN2 - The second GPIO pin selected within the task
//   CONFIG_PIN3 - The third GPIO pin selected within the task
//   CONFIG_PORT - The port in case the device has multiple in/out pins
//
//   Custom configuration is stored in the following:
//   PCONFIG(x)
//   x can be between 1 - 8 and can store values between -32767 - 32768 (16 bit)
//
//   N.B. these are aliases for a longer less readable amount of code. See _Plugin_Helper.h
//
//   PCONFIG_LABEL(x) is a function to generate a unique label used as HTML id to be able to match 
//                    returned values when saving a configuration.

// Make accessing specific parameters more readable in the code
// #define Pxxx_OUTPUT_TYPE_INDEX  2
# define P253_I2C_ADDRESS           PCONFIG(0)
# define P253_I2C_ADDRESS_LABEL     PCONFIG_LABEL(0)
# define P253_MODEL                 PCONFIG(1)
# define P253_MODEL_LABEL           PCONFIG_LABEL(1)
# define P253_MON_SCL_PIN           PCONFIG(2)
# define P253_MON_SCL_PIN_LABEL     PCONFIG_LABEL(2)
# define P253_QUERY1   					    PCONFIG(3)
# define P253_QUERY2   					    PCONFIG(4)
# define P253_QUERY3   					    PCONFIG(5)
# define P253_QUERY4   					    PCONFIG(6)
# define P253_SEN_FIRST             PCONFIG(7)
# define P253_SEN_ATTEMPT           PCONFIG_LONG(1)


# define P253_I2C_ADDRESS_DFLT      0x69
# define P253_I2C_ADDRESS_DFLT2     0x69
# define P253_I2C_ADDRESS_DFLT3     0x69
# define P253_MON_SCL_PIN_DFLT      13
# define P253_MODEL_DFLT            0  // Vindstyrka or SEN54
# define P253_QUERY1_DFLT           0  // Temperature (C)
# define P253_QUERY2_DFLT           1  // Humidity (%)
# define P253_QUERY3_DFLT           7  // DewPoint (C)
# define P253_QUERY4_DFLT           3  // tVOC (index)


# define P253_NR_OUTPUT_VALUES      4
# define P253_NR_OUTPUT_OPTIONS     9
# define P253_QUERY1_CONFIG_POS     3
# define P253_MAX_ATTEMPT           3  // Number of tentative before declaring NAN value

//# define LIMIT_BUILD_SIZE           1

// These pointers may be used among multiple instances of the same plugin,
// as long as the same settings are used.
P253_data_struct *Plugin_253_SEN              = nullptr;
boolean Plugin_253_init                       = false;

void IRAM_ATTR Plugin_253_interrupt();

// Forward declaration helper functions
const __FlashStringHelper* p253_getQueryString(uint8_t query);
const __FlashStringHelper* p253_getQueryValueString(uint8_t query);
unsigned int               p253_getRegister(uint8_t query, uint8_t model);
float                      p253_readVal(uint8_t      query, uint8_t      node, unsigned int model);


// A plugin has to implement the following function

boolean Plugin_253(uint8_t function, struct EventStruct *event, String& string)
{
  // function: reason the plugin was called
  // event: ??add description here??
  // string: ??add description here??

  boolean success = false;

  switch (function)
  {


    case PLUGIN_DEVICE_ADD:
    {
      // This case defines the device characteristics
      Device[++deviceCount].Number            = PLUGIN_ID_253;
      Device[deviceCount].Type                = DEVICE_TYPE_I2C;
      Device[deviceCount].VType               = Sensor_VType::SENSOR_TYPE_QUAD;
      Device[deviceCount].Ports               = 0;
      Device[deviceCount].PullUpOption        = true;
      Device[deviceCount].InverseLogicOption  = false;
      Device[deviceCount].FormulaOption       = true;
      Device[deviceCount].ValueCount          = P253_NR_OUTPUT_VALUES;
      Device[deviceCount].SendDataOption      = true;
      Device[deviceCount].TimerOption         = true;
      Device[deviceCount].I2CNoDeviceCheck    = true;
      Device[deviceCount].GlobalSyncOption    = true;
      Device[deviceCount].PluginStats         = true;
      Device[deviceCount].OutputDataType      = Output_Data_type_t::Simple;
      break;
    }


    case PLUGIN_GET_DEVICENAME:
    {
      // return the device name
      string = F(PLUGIN_NAME_253);
      break;
    }


    case PLUGIN_GET_DEVICEVALUENAMES:
    {
      // called when the user opens the module configuration page
      // it allows to add a new row for each output variable of the plugin
      // For plugins able to choose output types, see P026_Sysinfo.ino.
      for (uint8_t i = 0; i < VARS_PER_TASK; ++i) 
      {
        if ( i < P253_NR_OUTPUT_VALUES) 
        {
          uint8_t choice = PCONFIG(i + P253_QUERY1_CONFIG_POS);
          safe_strncpy(ExtraTaskSettings.TaskDeviceValueNames[i], p253_getQueryValueString(choice), sizeof(ExtraTaskSettings.TaskDeviceValueNames[i]));
        }
        else
        {
          ZERO_FILL(ExtraTaskSettings.TaskDeviceValueNames[i]);
        }
      }
      break;
    }


    case PLUGIN_SET_DEFAULTS:
    {
      // Set a default config here, which will be called when a plugin is assigned to a task.
      P253_I2C_ADDRESS = P253_I2C_ADDRESS_DFLT;
      P253_MODEL = P253_MODEL_DFLT;
      P253_QUERY1 = P253_QUERY1_DFLT;
      P253_QUERY2 = P253_QUERY2_DFLT;
      P253_QUERY3 = P253_QUERY3_DFLT;
      P253_QUERY4 = P253_QUERY4_DFLT;
      P253_MON_SCL_PIN = P253_MON_SCL_PIN_DFLT;
      P253_SEN_FIRST = 99;
      success = true;
      break;
    }


    # if FEATURE_I2C_GET_ADDRESS
    case PLUGIN_I2C_GET_ADDRESS:
    {
      event->Par1 = P253_I2C_ADDRESS_DFLT;
      success     = true;
      break;
    }
    # endif // if FEATURE_I2C_GET_ADDRESS


    case PLUGIN_I2C_HAS_ADDRESS:
    case PLUGIN_WEBFORM_SHOW_I2C_PARAMS:
    {
      //const uint8_t i2cAddressValues[] = { P253_I2C_ADDRESS_DFLT, P253_I2C_ADDRESS_DFLT2, P253_I2C_ADDRESS_DFLT3 };
      const uint8_t i2cAddressValues[] = { P253_I2C_ADDRESS_DFLT };
      if (function == PLUGIN_WEBFORM_SHOW_I2C_PARAMS) 
      {
        if (P253_SEN_FIRST == event->TaskIndex)                               // If first SEN, serial config available
        {
          //addFormSelectorI2C(P253_I2C_ADDRESS_LABEL, 3, i2cAddressValues, P253_I2C_ADDRESS);
          addFormSelectorI2C(F("i2c_addr"), 1, i2cAddressValues, P253_I2C_ADDRESS);
          addFormNote(F("Vindstyrka, SEN54, SEN55 default i2c address: 0x69"));
        }
      } 
      else 
      {
        success = intArrayContains(1, i2cAddressValues, event->Par1);
      }
      break;
    }


    case PLUGIN_WEBFORM_SHOW_GPIO_DESCR:
    {
      if (P253_SEN_FIRST == event->TaskIndex)                               // If first SEN, serial config available
      {
        string  = F("MonPin SCL: ");
        string += formatGpioLabel(P253_MON_SCL_PIN, false);
      }
      success = true;
      break;
    }


    case PLUGIN_WEBFORM_LOAD_OUTPUT_SELECTOR:
    {
      const __FlashStringHelper *options[P253_NR_OUTPUT_OPTIONS];
      for (int i = 0; i < P253_NR_OUTPUT_OPTIONS; ++i) 
      {
        options[i] = p253_getQueryString(i);
      }
      for (uint8_t i = 0; i < P253_NR_OUTPUT_VALUES; ++i) 
      {
        const uint8_t pconfigIndex = i + P253_QUERY1_CONFIG_POS;
        sensorTypeHelper_loadOutputSelector(event, pconfigIndex, i, P253_NR_OUTPUT_OPTIONS, options);
      }
      break;
    }


    case PLUGIN_WEBFORM_LOAD:
    {
      // this case defines what should be displayed on the web form, when this plugin is selected
      // The user's selection will be stored in
      // PCONFIG(x) (custom configuration)

      if (Plugin_253_SEN == nullptr) 
      {
        P253_SEN_FIRST = event->TaskIndex; // To detect if first SEN or not
      }

      if (P253_SEN_FIRST == event->TaskIndex)                               // If first SEN, serial config available
      {
        addHtml(F("<br><B>This SEN5x is the first. Its configuration of Pins will affect next SEN5x.</B>"));
        addHtml(F("<span style=\"color:red\"> <br><B>If several SEN5x's foreseen, don't use other pins.</B></span>"));
        
        const __FlashStringHelper *options_model[3] = { F("IKEA Vindstyrka"), F("Sensirion SEN54"), F("Sensirion SEN55")};
        
        addFormSelector(F("Model Type"), P253_MODEL_LABEL, 3, options_model, nullptr, P253_MODEL);

        addFormPinSelect(PinSelectPurpose::Generic_input, F("MonPin SCL"), F("taskdevicepin3"), P253_MON_SCL_PIN);
        addFormNote(F("Pin for monitoring i2c communication between Vindstyrka controller and SEN5x. (Only when Model - IKEA Vindstyrka is selected.)"));


        if (Plugin_253_SEN != nullptr) 
        {
          addRowLabel(F("Check (pass/fail/errCode)"));
          String chksumStats;
          chksumStats  = Plugin_253_SEN->getSuccCount();
          chksumStats += '/';
          chksumStats += Plugin_253_SEN->getErrCount();
          chksumStats += '/';
          chksumStats += Plugin_253_SEN->getErrCode();
          addHtml(chksumStats);
        }
        #ifndef LIMIT_BUILD_SIZE
        if (Plugin_253_SEN != nullptr) 
        {
          String prodname;
          String sernum;
          uint8_t firmware;
          Plugin_253_SEN->getEID(prodname, sernum, firmware);
          String txt = F(".::CHIP ID::. ProdName: ");
          txt += prodname;
          txt += F(", Serial Number: ");
          txt += sernum;
          txt += F(" ,Firmware: ");
          txt += String (firmware);
          addFormNote(txt);
        }
        #endif
      }
      else
      {
        addHtml(F("<br><B>This SEN5x is the NOT the first. Model and Pins config are DISABLED. Configuration is available in the first SEN5x plugin.</B>"));
        addHtml(F("<span style=\"color:red\"> <br><B>Only output value can be configured.</B></span>"));

        //looking for FIRST task Named "IKEA_Vindstyrka"
        uint8_t allready_defined=88;
        allready_defined=findTaskIndexByName("IKEA_Vindstyrka");
        P253_SEN_FIRST = allready_defined;
      }

      success = true;
      break;
    }


    case PLUGIN_WEBFORM_SAVE:
    {
      // this case defines the code to be executed when the form is submitted
      // the plugin settings should be saved to PCONFIG(x)
      // ping configuration should be read from CONFIG_PIN1 and stored

      // Save output selector parameters.
      for (uint8_t i = 0; i < P253_NR_OUTPUT_VALUES; ++i) 
      {
        const uint8_t pconfigIndex = i + P253_QUERY1_CONFIG_POS;
        const uint8_t choice = PCONFIG(pconfigIndex);
        sensorTypeHelper_saveOutputSelector(event, pconfigIndex, i, p253_getQueryValueString(choice));
      }
      P253_MODEL           = getFormItemInt(P253_MODEL_LABEL);
      P253_I2C_ADDRESS     = P253_I2C_ADDRESS_DFLT;
      P253_MON_SCL_PIN     = getFormItemInt(F("taskdevicepin3"));
      P253_SEN_FIRST       = P253_SEN_FIRST;
      
      Plugin_253_init = false; // Force device setup next time
      success = true;
      break;
    }


    case PLUGIN_INIT:
    {
      // this case defines code to be executed when the plugin is initialised
      // This will fail if the set to be first taskindex is no longer enabled

      if (P253_SEN_FIRST == event->TaskIndex) // If first SEN5x, config available
      {
        if (Plugin_253_SEN != nullptr) 
        {
          delete Plugin_253_SEN;
          Plugin_253_SEN = nullptr;
        }

        Plugin_253_SEN = new (std::nothrow) P253_data_struct();

        if (Plugin_253_SEN != nullptr) 
        {
          Plugin_253_SEN->setupModel(P253_MODEL);
          Plugin_253_SEN->setupDevice(P253_I2C_ADDRESS);
          Plugin_253_SEN->setupMonPin(P253_MON_SCL_PIN);
          Plugin_253_SEN->reset();

          pinMode(P253_MON_SCL_PIN, INPUT_PULLUP);
          attachInterrupt(P253_MON_SCL_PIN, Plugin_253_interrupt, RISING);
        }
      }
      else
      {
          if (Plugin_253_SEN != nullptr) 
          {
            //
          }
      }
      
      UserVar[event->BaseVarIndex]     = NAN;
      UserVar[event->BaseVarIndex + 1] = NAN;
      UserVar[event->BaseVarIndex + 2] = NAN;
      UserVar[event->BaseVarIndex + 3] = NAN;

      success = true;
      Plugin_253_init = true;

      break;
    }


    case PLUGIN_EXIT:
    {
      if (P253_SEN_FIRST == event->TaskIndex) // If first SEN5x, config available
      {
        if (Plugin_253_SEN != nullptr)
        {
          Plugin_253_SEN->disableInterrupt_monpin();
          delete Plugin_253_SEN;
          Plugin_253_SEN = nullptr;
        }
      }

      success = true;
      break;
    }


    case PLUGIN_READ:
    {
      // code to be executed to read data
      // It is executed according to the delay configured on the device configuration page, only once

      if(event->TaskIndex!=P253_SEN_FIRST)
      {
        //All the DATA are in the first task of IKEA_Vindstyrka
        //so all you have to do is to load this data in the current taskindex data
        initPluginTaskData(event->TaskIndex, getPluginTaskData(P253_SEN_FIRST) );
      }

      if (nullptr != Plugin_253_SEN) 
      {
        if (Plugin_253_SEN->inError()) 
        {
          UserVar[event->BaseVarIndex]     = NAN;
          UserVar[event->BaseVarIndex + 1] = NAN;
          UserVar[event->BaseVarIndex + 2] = NAN;
          UserVar[event->BaseVarIndex + 3] = NAN;
          addLog(LOG_LEVEL_ERROR, F("Vindstyrka / SEN5X: in Error!"));
        }
        else
        {
          if(event->TaskIndex==P253_SEN_FIRST)
          {
            Plugin_253_SEN->startMeasurements(); // getting ready for another read cycle
          }

          UserVar[event->BaseVarIndex]         = Plugin_253_SEN->getRequestedValue(P253_QUERY1);
          UserVar[event->BaseVarIndex + 1]     = Plugin_253_SEN->getRequestedValue(P253_QUERY2);
          UserVar[event->BaseVarIndex + 2]     = Plugin_253_SEN->getRequestedValue(P253_QUERY3);
          UserVar[event->BaseVarIndex + 3]     = Plugin_253_SEN->getRequestedValue(P253_QUERY4);
        }
      }

      success = true;
      break;
    }


    case PLUGIN_ONCE_A_SECOND:
    {
      // code to be executed once a second. Tasks which do not require fast response can be added here
      success = true;
    }


    case PLUGIN_TEN_PER_SECOND:
    {
      // code to be executed 10 times per second. Tasks which require fast response can be added here
      // be careful on what is added here. Heavy processing will result in slowing the module down!
      success = true;
    }


    case PLUGIN_FIFTY_PER_SECOND:
    {
      // code to be executed 10 times per second. Tasks which require fast response can be added here
      // be careful on what is added here. Heavy processing will result in slowing the module down!
      if(event->TaskIndex==P253_SEN_FIRST)
      {
        if (nullptr != Plugin_253_SEN) 
        {
          Plugin_253_SEN->monitorSCL();    // Vind / SEN5X FSM evaluation
          Plugin_253_SEN->update();
        }
      }
      success = true;
    }
  } // switch

  return success;
}   // function



/// @brief 
/// @param query 
/// @return 
const __FlashStringHelper*  p253_getQueryString(uint8_t query) 
{
  switch(query)
  {
    case 0: return F("Temperature (C)");
    case 1: return F("Humidity (% RH)");
    case 2: return F("PM 2.5 (um)");
    case 3: return F("tVOC (%)");
    case 4: return F("PM 1.0 (um)");
    case 5: return F("PM 4.0 (um)");
    case 6: return F("PM 10.0 (um)");
    case 7: return F("DewPoint (C)");
  }
  return F("");
}

/// @brief 
/// @param query 
/// @return 
const __FlashStringHelper* p253_getQueryValueString(uint8_t query) 
{
  switch(query)
  {
    case 0: return F("Temperature");
    case 1: return F("Humidity");
    case 2: return F("PM2p5");
    case 3: return F("tVOC");
    case 4: return F("PM1p0");
    case 5: return F("PM4p0");
    case 6: return F("PM10p0");
    case 7: return F("DewPoint");
  }
  return F("");
}


// When using interrupts we have to call the library entry point
// whenever an interrupt is triggered
void IRAM_ATTR Plugin_253_interrupt() 
{
  //addLog(LOG_LEVEL_ERROR, F("********* SEN5X: interrupt apear!"));
  if (Plugin_253_SEN) 
  {
    Plugin_253_SEN->checkPin_interrupt();
  }
}



#endif  //USES_P253