// Copyright 2026 Todd McCollam
/// @file
/// @brief Support for FurrionChillCube A/C protocol
/// @see https://github.com/crankyoldgit/IRremoteESP8266/issues/1787

// Supports:
//   Brand: Furrion,  Model: Chill Cube, nonducted, no heatpump
//   Brand: Furrion,  Model: RG57J5(B2)/BGCEFV1 remote



#ifndef IR_FurrionChillCube_H_
#define IR_FurrionChillCube_H_

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <algorithm>
#include <cstring>
#include "IRremoteESP8266.h"
#include "IRsend.h"
#include "IRrecv.h"
#include "IRtext.h"
#include "IRutils.h"
#ifndef UNIT_TEST
#include <Arduino.h>
#endif
#ifdef UNIT_TEST
#include "IRsend_test.h"
#endif

// Constants
const uint16_t kFurrionChillCubeHdrMark = 4440;
const uint16_t kFurrionChillCubeBitMark = 563;
const uint16_t kFurrionChillCubeHdrSpace = 4379;
const uint16_t kFurrionChillCubeOneSpace = 1592;
const uint16_t kFurrionChillCubeZeroSpace = 518;
const uint16_t kFurrionChillCubeFooterSpace = 5235;
const uint16_t kFurrionChillCubeSpaceGap = 5197;
const uint16_t kFurrionChillCubeFreq = 38000;  // Hz. (Guessing the most common frequency.)
const uint16_t kFurrionChillCubeNrOfSections = 3;
const uint16_t kFurrionChillCubeBytesPerSection = 6;
const uint16_t kFurrionChillCubeFollowNrOfSections = 2;
const uint16_t kFurrionChillCubeFollowBytesPerSection = 6;

using irutils::addBoolToString;
using irutils::addModeToString;
using irutils::addFanToString;
using irutils::addTempToString;
using std::min;
using std::max;
using std::memcpy;
using std::memcmp;

// Operating Modes                
// Dry and Fan use the same code, differentiated by the temperature code
const uint8_t kFurrionChillCubeCool = 0b00;
const uint8_t kFurrionChillCubeDry =  0b01;
const uint8_t kFurrionChillCubeAuto = 0b10;
const uint8_t kFurrionChillCubeFan =  0b01;

const uint8_t kFurrionChillCubeEcoModeOff =  0;
const uint8_t kFurrionChillCubeEcoModeOn = 1;
const uint8_t kFurrionChillCubeEcoMode50 = 2;
const uint8_t kFurrionChillCubeEcoMode75 =  3;

const uint8_t kFurrionChillCubeDefaultTempF = 75;
const uint8_t kFurrionChillCubeDefaultTempC = 24;

// Fan is always in forced auto in auto and dry modes.
// Fan Control - Goes in section 1 and 2 
const uint16_t kFurrionChillCubeFanForcedAuto = 0b00001100101;
const uint16_t kFurrionChillCubeFanAuto = 0b10101100110;
const uint16_t kFurrionChillCubeFanLevelOne = 0b10000101000;
const uint16_t kFurrionChillCubeFanLevelTwo = 0b01000111100;
const uint16_t kFurrionChillCubeFanLevelThree = 0b00101100100;


// Temperature
const uint8_t kFurrionChillCubeCelsiusMin = 16;
const uint8_t kFurrionChillCubeCelsiusMax = 30;
const uint8_t kFurrionChillCubeCelsiusMap[] = {
    // There's probably an equation to figure these out but I'm not seeing it, so mappinng it is.
    0b000001,  // 16C
    0b000000,  // 17C
    0b000100,  // 18C
    0b001100,  // 19C
    0b001000,  // 20C
    0b011000,  // 21C
    0b011100,  // 22C
    0b010100,  // 23C
    0b010000,  // 24C
    0b110000,  // 25C
    0b110100,  // 26C
    0b100100,  // 27C
    0b100000,  // 28C
    0b101000,  // 29C
    0b101100   // 30C
};

const uint8_t kFurrionChillCubeFahrenheitMin = 60;
const uint8_t kFurrionChillCubeFahrenheitMax = 86;
const uint8_t kFurrionChillCubeFahrenheitMap[] = {
    // The remotes internal F to C conversion does not match round((F - 32.0) * 5.0 / 9.0), nor roundup or rounddown
    0b000001,  // 60
    0b000011,  // 61
    0b000000,  // 62
    0b000010,  // 63
    0b000100,  // 64
    0b000110,  // 65
    0b001100,  // 66
    0b001110,  // 67
    0b001000,  // 68
    0b001010,  // 69
    0b011000,  // 70
    0b011010,  // 71
    0b011100,  // 72
    0b011110,  // 73
    0b010110,  // 74
    0b010000,  // 75
    0b010010,  // 76
    0b110000,  // 77
    0b110010,  // 78
    0b110100,  // 79
    0b110110,  // 80
    0b100100,  // 81
    0b100110,  // 82
    0b100010,  // 83
    0b101000,  // 84
    0b101010,  // 85
    0b101100,  // 86
};

const uint8_t kFurrionChillCubeTempNA = 0b11100100; // 0xE4 Used in fan mode where there is no temperature, also differentiates between dry and fan

// Fixed State Messages
const uint8_t kFurrionChillCubeOff[] = { 0xB2, 0x4D, 0x7B, 0x84, 0xE0, 0x1F, 0xB2, 0x4D, 0x7B, 0x84, 0xE0, 0x1F };
const uint8_t kFurrionChillCubeLEDToggle[] = { 0xB9, 0x46, 0xF5, 0x0A, 0x09, 0xF6, 0xB9, 0x46, 0xF5, 0x0A, 0x09, 0xF6 };
const uint8_t kFurrionChillCubeSwingOn[] = { 0xB9, 0x46, 0xF5, 0x0A, 0x04, 0xFB, 0xB9, 0x46, 0xF5, 0x0A, 0x04, 0xFB };
const uint8_t kFurrionChillCubeSwingOff[] = { 0xB9, 0x46, 0xF5, 0x0A, 0x05, 0xFA, 0xB9, 0x46, 0xF5, 0x0A, 0x05, 0xFA };
const uint8_t kFurrionChillCubeTurboOn[] = { 0xB9, 0x46, 0xF5, 0x0A, 0x01, 0xFE, 0xB9, 0x46, 0xF5, 0x0A, 0x01, 0xFE };
const uint8_t kFurrionChillCubeTurboOff[] = { 0xB9, 0x46, 0xF5, 0x0A, 0x02, 0xFD, 0xB9, 0x46, 0xF5, 0x0A, 0x02, 0xFD };

const uint8_t kFurrionChillCubeEcoOff[] = { 0xB5, 0x4A, 0xF5, 0x0A, 0xBB, 0x44, 0xB5, 0x4A, 0xF5, 0x0A, 0xBB, 0x44 };
const uint8_t kFurrionChillCubeEcoOn[] = { 0xB9, 0x46, 0xF5, 0x0A, 0x24, 0xDB, 0xB9, 0x46, 0xF5, 0x0A, 0x24, 0xDB };
const uint8_t kFurrionChillCube50power[] = { 0xB5, 0x4A, 0xF5, 0x0A, 0xBE, 0x41, 0xB5, 0x4A, 0xF5, 0x0A, 0xBE, 0x41 };
const uint8_t kFurrionChillCube75power[] = { 0xB5, 0x4A, 0xF5, 0x0A, 0xBD, 0x42, 0xB5, 0x4A, 0xF5, 0x0A, 0xBD, 0x42 };

// On, 77F, Mode: Auto
const uint8_t kFurrionChillCubeDefaultState[kFurrionChillCubeStateLength] = {
  0xB2, 0x4D, 0x1F, 0xE0, 0xC8, 0x37,
  0xB2, 0x4D, 0x1F, 0xE0, 0xC8, 0x37,
  0xD5, 0x65, 0x00, 0x01, 0x00, 0x3B};

// On, 25C, 25C, Auto, Auto
const uint8_t kFurrionChillCubeFollowDefaultState[kFurrionChillCubeFollowStateLength] = {
  0xBA, 0x45, 0x5A, 0xA5, 0xCA, 0x35,
  0xBA, 0x45, 0x5A, 0xA5, 0xCA, 0x35};


union FurrionChillCubeProtocol {
  uint8_t raw[kFurrionChillCubeStateLength];  ///< The state in IR code form.
  struct {
    uint8_t               :8;   // Fixed value 0b10110010 / 0xB2    ############
    uint8_t InnvertS1_1   :8;   // Invert byte 1          / 0x4D    #
    uint8_t               :5;   // Timer off                        # Section 1
    uint8_t FanS1         :3;   // Fan Speed                        #
    uint8_t InnvertS1_3   :8;   // Invert byte 3                    # =
	uint8_t               :2;   // Timer                            #
	uint8_t ModeS1        :2;   // Operating Mode                   #
    uint8_t TempS1        :4;   // Temperature Setpoint             # Section 2
    uint8_t InnvertS1_5   :8;   // Invert byte 5 or Timer On        # ##########

    uint8_t               :8;   // Fixed value 0b10110010 / 0xB2    ############
    uint8_t InnvertS2_1   :8;   // Invert byte 1          / 0x4D    #
    uint8_t               :5;   // Timer off                        # Section 1
    uint8_t FanS2         :3;   // Fan Speed                        #
    uint8_t InnvertS2_3   :8;   // Invert byte 3                    # =
	uint8_t               :2;   // Timer                            #
	uint8_t ModeS2        :2;   // Operating Mode                   #
    uint8_t TempS2        :4;   // Temperature Setpoint             # Section 2
    uint8_t InnvertS2_5   :8;   // Invert byte 5 or Timer On        # ##########

    uint8_t               :8;   // Fixed value 0b11010101 / 0xD5    ###########
    uint8_t FanS3         :8;   // Fan speed                        #
    uint8_t               :5;   // Undeciphered, 0b00000            #
    uint8_t TempS3        :1;   // 1F temp modifier, unused for C   #
    uint8_t               :1;   // Undeciphered, 0b0                #
    uint8_t SleepS3       :1;   // Sleep Mode                       #
	uint8_t UseFahrenheit :1;   // b0001 for F, b0000 for C         #
	uint8_t               :3;   // Undeciphered, 0b000              #
    uint8_t TempS3P2      :1;   // b1 for 61,62F,16C else b0        #
    uint8_t               :3;   // Undeciphered, 0b000              #
    uint8_t               :8;   // Undeciphered                     #
    uint8_t ChecksumS3    :8;   // Checksum from byte 13-17         ###########
  };
};

union FurrionChillCubeFollowProtocol {
  uint8_t raw[kFurrionChillCubeFollowStateLength];  ///< The state in IR code form.
  struct {
    uint8_t               :8;   // Constant 0xBA                    ############
    uint8_t FInnvertS1_1  :8;   // Invert byte 1                    #
    uint8_t FSensTempS1   :6;   // Sensor temp                      # Section 1
    uint8_t FPowerS1      :1;   // Follow mode on/off               #
	uint8_t               :1;   // Undeciphered                     #
    uint8_t FInnvertS1_3  :8;   // Invert byte 3                    # =
	uint8_t               :2;   // Undeciphered                     #
	uint8_t FModeS1       :2;   // Mode, Auto or Cool               #
    uint8_t FTempS1       :4;   // Temperature setpoint             # Section 2
    uint8_t FInnvertS1_5  :8;   // Invert byte 5                    # ##########

    uint8_t               :8;   // Constant 0xBA                    ############
    uint8_t FInnvertS2_1  :8;   // Invert byte 1                    #
    uint8_t FSensTempS2   :6;   // Sensor temp                      # Section 1
    uint8_t FPowerS2      :1;   // Follow mode on/off               #
	uint8_t               :1;   // Undeciphered                     #
    uint8_t FInnvertS2_3  :8;   // Invert byte 3                    # =
	uint8_t               :2;   // Undeciphered                     #
	uint8_t FModeS2       :2;   // Mode, Auto or Cool               #
    uint8_t FTempS2       :4;   // Temperature setpoint             # Section 2
    uint8_t FInnvertS2_5  :8;   // Invert byte 5                    # ##########
  };
};

// Classes

/// Class for handling detailed FurrionChillCube A/C messages.
class IRFurrionChillCubeAC {
 public:
  explicit IRFurrionChillCubeAC(const uint16_t pin, const bool inverted = false,
                      const bool use_modulation = true);
  void stateReset(void);
#if SEND_FURRION_CHILLCUBE
  void send(const uint16_t repeat = 0);
  void sendFollow(const uint16_t repeat = 0);
  /// Run the calibration to calculate uSec timing offsets for this platform.
  /// @return The uSec timing offset needed per modulation of the IR Led.
  /// @note This will produce a 65ms IR signal pulse at 38kHz.
  ///   Only ever needs to be run once per object instantiation, if at all.
  int8_t calibrate(void) { return _irsend.calibrate(); }
#endif  // SEND_FURRION_CHILLCUBE
  void begin();
  void setPower(const bool state);
  bool getPower(void) const;
  void setTemp(const uint8_t temp);
  void setSensorTemp(const uint8_t temp);
  void setSensorTempRaw(const uint8_t code);
  uint8_t getSensorTempRaw(void) const;
  uint8_t getTemp(void) const;
  void setUseFahrenheit(const bool on);
  bool getUseFahrenheit(void) const;
  void setFan(const uint16_t speed);
  uint16_t getFan(void) const;
  void setMode(const uint8_t mode);
  uint8_t getMode(void) const;
  void setTurbo(const bool on);
  bool getTurbo(void) const;
  void setSwing(const bool on);
  bool getSwing(void) const;
  stdAc::swingv_t toCommonSwing(void) const;
  void setEco(const uint8_t on);
  uint8_t getEco(void) const;
  uint8_t* getRaw(void);
  uint8_t* getRawFollow(void);
  void setRaw(const uint8_t new_code[],
              const uint16_t length = kFurrionChillCubeStateLength);
  void setRawFollow(const uint8_t new_code[],
              const uint16_t length = kFurrionChillCubeFollowStateLength);
  static uint8_t convertMode(const stdAc::opmode_t mode);
  static uint16_t convertFan(const stdAc::fanspeed_t speed);
  static stdAc::opmode_t toCommonMode(const uint8_t mode, const bool dryMode);
  static stdAc::fanspeed_t toCommonFanSpeed(const uint16_t speed);
  stdAc::state_t toCommon(void) const;
  String toString(void) const;
  void sendLED(const uint16_t repeat);
  void sendTurbo(const uint16_t repeat);
  void sendSwing(const uint16_t repeat);
  void sendEco(const uint16_t repeat);
  void setSleep(const bool sleep);
  bool getSleep(void) const;
  void setFollow(const bool follow);
  bool getFollow(void) const;
  void setDryMode(const bool dry);
  bool getDryMode(void) const;
  void fixState(void);
#ifndef UNIT_TEST

 private:
  IRsend _irsend;  ///< Instance of the IR send class
#else
  /// @cond IGNORE
  IRsendTest _irsend;  ///< Instance of the testing IR send class
  /// @endcond
#endif
  FurrionChillCubeProtocol _;  ///< The state of the IR remote in IR code form.
  FurrionChillCubeFollowProtocol __;  ///< The state of the IR remote in IR code form for follow me codes
  // Internal State settings
  bool powerFlag = false;
  bool turboFlag = false;
  bool swingFlag = false;
  bool dryFlag = false;
  uint8_t ecoFlag = 0;

  void setInvertBytes();
  void setInvertBytesFollow();
  void setCheckSumS3();
  void setTempRaw(const uint8_t code);
  uint8_t getTempRaw(void) const;
};

#endif  // IR_FurrionChillCube_H_
