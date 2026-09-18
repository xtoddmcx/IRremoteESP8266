// Copyright 2026 Todd McCollam
/// @file
/// @brief Support for the Furrion Chill Cube
/// @see https://github.com/crankyoldgit/IRremoteESP8266/issues/
/// Issue not created yet to merge into main project

#include "ir_Furrion.h"
#include <algorithm>

#if SEND_FURRION_CHILLCUBE
/// Send a Furrion 144-bit or 96 bit message
/// Status: ALPHA
/// @param[in] data The message to be sent.
/// @param[in] nbytes The number of bytes of message to be sent.
/// @param[in] repeat The number of times the command is to be repeated.
void IRsend::sendFurrionChillCube(const unsigned char data[], const uint16_t nbytes,
                          const uint16_t repeat) {
  // nbytes is required to be a multiple of kFurrionChillCubeBytesPerSection.
  if (nbytes % kFurrionChillCubeBytesPerSection != 0)return;
  // Set IR carrier frequency
  enableIROut(kFurrionChillCubeFreq);



  for (uint16_t r = 0; r <= repeat; r++) {
    for (uint16_t offset=0; offset < nbytes; offset += kFurrionChillCubeBytesPerSection)
      // Section Header + Data + Footer
      sendGeneric(kFurrionChillCubeHdrMark, kFurrionChillCubeHdrSpace,
                  kFurrionChillCubeBitMark, kFurrionChillCubeOneSpace,
                  kFurrionChillCubeBitMark, kFurrionChillCubeZeroSpace,
                  kFurrionChillCubeBitMark, kFurrionChillCubeFooterSpace,
                  data + offset, kFurrionChillCubeBytesPerSection,
                  kFurrionChillCubeFreq, true, 0, kDutyDefault);
    space(kDefaultMessageGap);  // Complete guess
  }
}

#endif  // SEND_FURRION_CHILLCUBE

/// Class constructor.
/// @param[in] pin GPIO to be used when sending.
/// @param[in] inverted Is the output signal to be inverted?
/// @param[in] use_modulation Is frequency modulation to be used?
IRFurrionChillCubeAC::IRFurrionChillCubeAC(const uint16_t pin, const bool inverted,
                           const bool use_modulation)
    : _irsend(pin, inverted, use_modulation) { stateReset(); }

/// Reset the internal state to a fixed known good state.
void IRFurrionChillCubeAC::stateReset(void) {
  setRaw(kFurrionChillCubeDefaultState, kFurrionChillCubeStateLength);
  setRawFollow(kFurrionChillCubeFollowDefaultState, kFurrionChillCubeFollowStateLength);
  setPower(true);
}

/// Set up hardware to be able to send a message.
void IRFurrionChillCubeAC::begin(void) { _irsend.begin(); }

#if SEND_FURRION_CHILLCUBE
void IRFurrionChillCubeAC::sendLED(const uint16_t repeat) {
  _irsend.sendFurrionChillCube(kFurrionChillCubeLEDToggle, sizeof(kFurrionChillCubeLEDToggle), repeat);
}

void IRFurrionChillCubeAC::sendTurbo(const uint16_t repeat) {
  if (turboFlag)
      _irsend.sendFurrionChillCube(kFurrionChillCubeTurboOn, sizeof(kFurrionChillCubeTurboOn), repeat);
  else
    _irsend.sendFurrionChillCube(kFurrionChillCubeTurboOff, sizeof(kFurrionChillCubeTurboOff), repeat);
}

void IRFurrionChillCubeAC::sendSwing(const uint16_t repeat) {
  if (swingFlag)
    _irsend.sendFurrionChillCube(kFurrionChillCubeSwingOn, sizeof(kFurrionChillCubeSwingOn), repeat);
  else
    _irsend.sendFurrionChillCube(kFurrionChillCubeSwingOff, sizeof(kFurrionChillCubeSwingOff), repeat);
}

void IRFurrionChillCubeAC::sendEco(const uint16_t repeat) {
  switch (ecoFlag) {
    case 1:
      _irsend.sendFurrionChillCube(kFurrionChillCubeEcoOn, sizeof(kFurrionChillCubeEcoOn), repeat);
      return;
    case 2:
      _irsend.sendFurrionChillCube(kFurrionChillCube50power, sizeof(kFurrionChillCube50power), repeat);
      return;
    case 3:
      _irsend.sendFurrionChillCube(kFurrionChillCube75power, sizeof(kFurrionChillCube75power), repeat);
      return;
    default:
      _irsend.sendFurrionChillCube(kFurrionChillCubeEcoOff, sizeof(kFurrionChillCubeEcoOff), repeat);
  }

}

/// Send the current internal state as an IR message.
/// @param[in] repeat Nr. of times the message will be repeated.
void IRFurrionChillCubeAC::send(const uint16_t repeat) {
  if (!powerFlag) {  // "Off" is a 96bit message
    _irsend.sendFurrionChillCube(kFurrionChillCubeOff, sizeof(kFurrionChillCubeOff), repeat);
  } else {
    _irsend.sendFurrionChillCube(getRaw(), kFurrionChillCubeFollowStateLength, repeat);
  }
}
void IRFurrionChillCubeAC::sendFollow(const uint16_t repeat) {
  // TODO: Add LED, Swing, and Turbo messages
  if (!powerFlag) return; // AC is off
  _irsend.sendFurrionChillCube(getRawFollow(), kFurrionChillCubeFollowStateLength, repeat);
}

#endif  // SEND_FURRION_CHILLCUBE

/// Get a copy of the internal state as a valid code for this protocol.
/// @return A valid code for this protocol based on the current internal state.
unsigned char* IRFurrionChillCubeAC::getRaw(void) {
  setInvertBytes();
  setCheckSumS3();
  return _.raw;
}

unsigned char* IRFurrionChillCubeAC::getRawFollow(void) {
  setInvertBytesFollow();
  return __.raw;
}

/// Set the internal state from a valid code for this protocol.
/// @param[in] new_code A valid code for this protocol.
/// @param[in] length Size of the array being passed in in bytes.
void IRFurrionChillCubeAC::setRaw(const uint8_t new_code[], const uint16_t length) {
  const uint16_t len = min(length, kFurrionChillCubeStateLength);
  memcpy(_.raw, new_code, len);
}

void IRFurrionChillCubeAC::setRawFollow(const uint8_t new_code[], const uint16_t length) {
  const uint16_t len = min(length, kFurrionChillCubeFollowStateLength);
  memcpy(__.raw, new_code, len);
}


void IRFurrionChillCubeAC::setPower(const bool on) {
  powerFlag = on;
}

bool IRFurrionChillCubeAC::getPower(void) const {
  return powerFlag;
}

void IRFurrionChillCubeAC::setTurbo(const bool on) {
  turboFlag = on;
}

bool IRFurrionChillCubeAC::getTurbo(void) const {
  return turboFlag;
}

void IRFurrionChillCubeAC::setSwing(const bool on) {
  swingFlag = on;
}

bool IRFurrionChillCubeAC::getSwing(void) const {
  return swingFlag;
}

stdAc::swingv_t IRFurrionChillCubeAC::toCommonSwing(void) const {
  if (swingFlag) return stdAc::swingv_t::kAuto;
  return stdAc::swingv_t::kOff;
}

void IRFurrionChillCubeAC::setEco(const uint8_t on) {
  ecoFlag = on;
}

uint8_t IRFurrionChillCubeAC::getEco(void) const {
  return ecoFlag;
}

void IRFurrionChillCubeAC::setTempRaw(const uint8_t code) {
  _.TempS1 = _.TempS2 = code >> 2;  // save bits 3-6 in S1 and S2
  __.FTempS1 = __.FTempS2 = code >> 2;  // save bits 3-6 in S1 and S2
  _.TempS3 = code >> 1;             // save bit 2 in Section3
  _.TempS3P2 = code;                  // save bit 1 in Section3
}

void IRFurrionChillCubeAC::setSensorTemp(const uint8_t temp, const bool fahrenheit) {
  if (temp == 0) {
    setSensorTempRaw(0);
	setRaw(kFurrionChillCubeDefaultState, kFurrionChillCubeStateLength);
    return;
  }
  uint8_t tempC = temp;
  if (fahrenheit) tempC = (temp - 32.0) * 5.0 / 9.0;
  tempC = std::round(tempC);
  setRaw(kFurrionChillCubeFollowDefaultState, kFurrionChillCubeFollowStateLength);

  setSensorTempRaw(tempC);
}

uint8_t IRFurrionChillCubeAC::getSensorTempRaw(void) const {
	return __.FSensTempS1;
}

void IRFurrionChillCubeAC::setSensorTempRaw(const uint8_t code) {
  __.FSensTempS1 = __.FSensTempS2 = code;
}

/// Set the temp. in degrees
/// @param[in] temp Desired temperature in Degrees.
/// @param[in] fahrenheit Use units of Fahrenheit and set that as units used.
///   false is Celsius (Default), true is Fahrenheit.
void IRFurrionChillCubeAC::setTemp(const uint8_t temp, const bool fahrenheit, const stdAc::opmode_t mode) {
  if ( mode == stdAc::opmode_t::kFan) {
    setTempRaw(kFurrionChillCubeTempNA);
    return;
  } 
  if (fahrenheit) {
    uint8_t constrainedTemp = max(kFurrionChillCubeFahrenheitMin, temp);
    constrainedTemp = min(kFurrionChillCubeFahrenheitMax, constrainedTemp);
    setTempRaw(
      kFurrionChillCubeFahrenheitMap[constrainedTemp - kFurrionChillCubeFahrenheitMin]);
    setUseFahrenheit(true);
  } else {
    uint8_t constrainedTemp = max(kFurrionChillCubeCelsiusMin, temp);
    constrainedTemp = min(kFurrionChillCubeCelsiusMax, constrainedTemp);
    setTempRaw(kFurrionChillCubeCelsiusMap[constrainedTemp - kFurrionChillCubeCelsiusMin]);
    setUseFahrenheit(false);
  }
}

uint8_t IRFurrionChillCubeAC::getTemp(void) const {
  uint8_t temp = (_.TempS1 << 2) + (_.TempS3 << 1) + _.TempS3P2;
  if (getUseFahrenheit()) {
    for (uint8_t i = 0; i < sizeof(kFurrionChillCubeFahrenheitMap); i++) {
      if (temp == kFurrionChillCubeFahrenheitMap[i]) {
        return kFurrionChillCubeFahrenheitMin + i;
      }
    }
    return 77;
  } else {
    for (uint8_t i = 0; i < sizeof(kFurrionChillCubeCelsiusMap); i++) {
      if (temp == kFurrionChillCubeCelsiusMap[i]) {
        return kFurrionChillCubeCelsiusMin + i;
      }
    }
    return 25;
  }
}

void IRFurrionChillCubeAC::setUseFahrenheit(const bool on) {
  _.UseFahrenheit = on;
}

bool IRFurrionChillCubeAC::getUseFahrenheit(void) const {
  return _.UseFahrenheit;
}

/// Set the speed of the fan.
/// @param[in] speed The desired setting.
void IRFurrionChillCubeAC::setFan(const uint16_t speed) {
  _.FanS1 = _.FanS2 = speed >> 8;  // save 3 bits in S1 and S2
  _.FanS3 = speed & 0b11111111;      // save 8 bits in Section3
}

uint16_t IRFurrionChillCubeAC::getFan(void) const {
  return (_.FanS1 << 8) + _.FanS3;
}

/// Set the desired operation mode.
/// @param[in] mode The desired operation mode.
void IRFurrionChillCubeAC::setMode(const uint8_t mode) {
  _.ModeS1 = _.ModeS2 = mode;  // save 4 bits in S1 and S2
  __.FModeS1 = __.FModeS2 = mode >> 3;
  if ( mode == kFurrionChillCubeFan ) setTempRaw(kFurrionChillCubeTempNA);
}

uint8_t IRFurrionChillCubeAC::getMode(void) const {
  return _.ModeS1;
}

/// Set the Sleep mode of the A/C.
// Haven't decoded how sleep mode works so this is a placeholder
void IRFurrionChillCubeAC::setSleep(const bool sleep) {
  if (sleep) _.SleepS3 = 1;
    else _.SleepS3 = 0;            
}

/// Get the Sleep mode of the A/C.
bool IRFurrionChillCubeAC::getSleep(void) const { 
  if (_.SleepS3 == 1) return true;
  return false; 
}

/// Convert a stdAc::opmode_t enum into its native mode.
/// @param[in] mode The enum to be converted.
/// @return The native equivalent of the enum.
uint8_t IRFurrionChillCubeAC::convertMode(const stdAc::opmode_t mode) {
  switch (mode) {
    case stdAc::opmode_t::kCool:
      return kFurrionChillCubeCool;
    case stdAc::opmode_t::kDry:
      return kFurrionChillCubeDry;
    case stdAc::opmode_t::kFan:
      return kFurrionChillCubeFan;
    default:
      return kFurrionChillCubeAuto;
  }
}

/// Convert a stdAc::fanspeed_t enum into it's native speed.
/// @param[in] speed The enum to be converted.
/// @return The native equivalent of the enum.
uint16_t IRFurrionChillCubeAC::convertFan(const stdAc::fanspeed_t speed) {
	/* Figure out how to get the mode so we can set forcedauto
  switch (mode) {
    case stdAc::opmode_t::kAuto:
      return kFurrionChillCubeFanForcedAuto;
    case stdAc::opmode_t::kDry:
      return kFurrionChillCubeFanForcedAuto;
  }
  */
  switch (speed) {
    case stdAc::fanspeed_t::kMin:
      return kFurrionChillCubeFanLevelOne;
    case stdAc::fanspeed_t::kLow:
      return kFurrionChillCubeFanLevelOne;
    case stdAc::fanspeed_t::kMedium:
      return kFurrionChillCubeFanLevelTwo;
    case stdAc::fanspeed_t::kHigh:
      return kFurrionChillCubeFanLevelThree;
    case stdAc::fanspeed_t::kMax:
      return kFurrionChillCubeFanLevelThree;
    default:
      return kFurrionChillCubeFanAuto;
  }
}

/// Convert a native mode into its stdAc equivalent.
/// @param[in] mode The native setting to be converted.
/// @return The stdAc equivalent of the native setting.
stdAc::opmode_t IRFurrionChillCubeAC::toCommonMode(const uint8_t mode, const uint8_t temp) {
  switch (mode) {
    case kFurrionChillCubeCool: return stdAc::opmode_t::kCool;
    case kFurrionChillCubeDry: 
      if ( temp == kFurrionChillCubeTempNA )
	    return stdAc::opmode_t::kFan;
      else
        return stdAc::opmode_t::kDry;
    default: return stdAc::opmode_t::kAuto;
  }
}

/// Convert a native fan speed into its stdAc equivalent.
/// @param[in] speed The native setting to be converted.
/// @return The stdAc equivalent of the native setting.
stdAc::fanspeed_t IRFurrionChillCubeAC::toCommonFanSpeed(const uint16_t speed) {
  switch (speed) {
    case kFurrionChillCubeFanLevelThree: return stdAc::fanspeed_t::kHigh;
    case kFurrionChillCubeFanLevelTwo: return stdAc::fanspeed_t::kMedium;
    case kFurrionChillCubeFanLevelOne: return stdAc::fanspeed_t::kLow;
    default: return stdAc::fanspeed_t::kAuto;
  }
}

/// Convert the current internal state into its stdAc::state_t equivalent.
/// @return The stdAc equivalent of the native settings.
stdAc::state_t IRFurrionChillCubeAC::toCommon(void) const {
  stdAc::state_t result{};
  result.protocol = decode_type_t::FURRION_CHILLCUBE;
  result.power = getPower();
  result.mode = toCommonMode(getMode(), getTemp());
  result.celsius = !getUseFahrenheit();
  result.degrees = getTemp();
  result.fanspeed = toCommonFanSpeed(getFan());
  // Not supported.
  result.model = -1;
  result.turbo = getTurbo();
  result.swingv = toCommonSwing();
  result.swingh = stdAc::swingh_t::kOff;
  result.light = false;
  result.filter = false;
  result.econo = getEco();
  result.clean = false;
  result.beep = false;
  result.clock = -1;
  result.sleep = getSleep();
  return result;
}

/// Convert the current internal state into a human readable string.
/// @return A human readable string.
String IRFurrionChillCubeAC::toString(void) const {
  uint8_t mode = getMode();
  uint8_t fan = static_cast<int>(toCommonFanSpeed(getFan()));
  String result = "";
  result.reserve(70);  // Reserve some heap for the string to reduce fragging.
  result += addBoolToString(getPower(), kPowerStr, false);
  result += addModeToString(mode, kFurrionChillCubeAuto, kFurrionChillCubeCool, 0,
                            kFurrionChillCubeDry, kFurrionChillCubeFan);
  result += addFanToString(fan, static_cast<int>(stdAc::fanspeed_t::kMax),
                           static_cast<int>(stdAc::fanspeed_t::kMin),
                           static_cast<int>(stdAc::fanspeed_t::kAuto),
                           static_cast<int>(stdAc::fanspeed_t::kAuto),
                           static_cast<int>(stdAc::fanspeed_t::kMedium));
  result += addTempToString(getTemp(), !getUseFahrenheit());
  return result;
}

void IRFurrionChillCubeAC::setInvertBytes() {
  for (uint8_t i = 0; i <= 10; i += 2) {
    _.raw[i + 1] = ~_.raw[i];
  }
}

void IRFurrionChillCubeAC::setInvertBytesFollow() {
  for (uint8_t i = 0; i <= 10; i += 2) {
    __.raw[i + 1] = ~__.raw[i];
  }
}

void IRFurrionChillCubeAC::setCheckSumS3() {
  _.ChecksumS3 =  sumBytes(&(_.raw[12]), 5); // Don't need the bitwise and, sumbytes is a uint8_t so it only fits 8 bits
}

#if DECODE_FURRION_CHILLCUBE
/// Decode the supplied Furrion 144-bit / 18-byte A/C message, or 48 bit / 6 byte
/// Status: ALPHA
/// @param[in,out] results Ptr to the data to decode & where to store the decode
///   result.
/// @param[in] offset The starting index to use when attempting to decode the
///   raw data. Typically/Defaults to kStartOffset.
/// @param[in] nbits The number of data bits to expect.
/// @param[in] strict Flag indicating if we should perform strict matching.
/// @return A boolean. True if it can decode it, false if it can't.
/// nbits isn't helpful since we're checking for 2 different message lengths, might delete it
bool IRrecv::decodeFurrionChillCube(decode_results *results, uint16_t offset,
                            const uint16_t nbits, const bool strict) {
  bool followerMessage = false;
  if (results->rawlen < 2 * kFurrionChillCubeBits +
                        kFurrionChillCubeNrOfSections * (kHeader + kFooter) -
                        1 + offset) {
    if (results->rawlen < 2 * kFurrionChillCubeFollowBits +
                          kFurrionChillCubeFollowNrOfSections * (kHeader + kFooter) -
                          1 + offset) 
      return false;
    else 
      followerMessage = true;
  }
  // Since this function is defined in ir_Furrion.h as nbits = kFurrionChillCubeBits
  // I don't see how this if statement does anything
  if (!followerMessage) {
    if (strict && nbits != kFurrionChillCubeBits )
      return false;      // Not strictly a FurrionChillCube message.
  } 
  if (nbits % 8 != 0)  // nbits has to be a multiple of nr. of bits in a byte.
    return false;
  if (!followerMessage)
    if (nbits % kFurrionChillCubeNrOfSections != 0 )
      return false;  // nbits has to be a multiple of kFurrionChillCubeNrOfSections.
  else
    if (nbits % kFurrionChillCubeFollowNrOfSections != 0)
      return false;  // nbits has to be a multiple of kFurrionChillCubeFollowNrOfSections.
  uint16_t kSectionBits = 0;
  uint16_t kSectionBytes = 0;
  uint16_t kNBytes = 0;
  if (!followerMessage) {
    kSectionBits = kFurrionChillCubeBits / kFurrionChillCubeNrOfSections;
    kSectionBytes = kSectionBits / 8;
    kNBytes = kSectionBytes * kFurrionChillCubeNrOfSections;
  } else {
    kSectionBits = kFurrionChillCubeFollowBits / kFurrionChillCubeFollowNrOfSections;
    kSectionBytes = kSectionBits / 8;
    kNBytes = kSectionBytes * kFurrionChillCubeFollowNrOfSections;
  }
  // Capture each section individually
  for (uint16_t pos = 0, section = 0;
       pos < kNBytes;
       pos += kSectionBytes, section++) {
    uint16_t used = 0;
    // Section Header + Section Data + Section Footer
    used = matchGeneric(results->rawbuf + offset, results->state + pos,
                        results->rawlen - offset, kSectionBits,
                        kFurrionChillCubeHdrMark, kFurrionChillCubeHdrSpace,
                        kFurrionChillCubeBitMark, kFurrionChillCubeOneSpace,
                        kFurrionChillCubeBitMark, kFurrionChillCubeZeroSpace,
                        kFurrionChillCubeBitMark, kFurrionChillCubeFooterSpace,
                        section >= kFurrionChillCubeNrOfSections - 1,
                        _tolerance, kMarkExcess, true);
    if (!used) return false;  // Didn't match.
    offset += used;
  }


  // Compliance

  // Success
  results->decode_type = decode_type_t::FURRION_CHILLCUBE;
  if (!followerMessage)
    results->bits = kFurrionChillCubeBits;
  else
    results->bits = kFurrionChillCubeFollowBits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}
#endif  // DECODE_FURRION_CHILLCUBE
