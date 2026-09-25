// Copyright 2022 David Conran
// Copyright 2022 Nico Thien
/// @file
/// @brief Support for the Furrion A/C / heatpump protocol
/// @see https://github.com/crankyoldgit/IRremoteESP8266/issues/1787

#include "ir_Furrion.h"
#include <algorithm>

#if SEND_FURRION_CHILLCUBE
/// Send a Furrion 144-bit / 18-byte message (24-bit message are also possible)
/// Status: ALPHA
/// @param[in] data The message to be sent.
/// @param[in] nbytes The number of bytes of message to be sent.
/// @param[in] repeat The number of times the command is to be repeated.
void IRsend::sendFurrionChillCube(const unsigned char data[], const uint16_t nbytes,
                          const uint16_t repeat) {
  // nbytes is required to be a multiple of kFurrionChillCubeBytesPerSection.
  if (nbytes % kFurrionChillCubeBytesPerSection != 0)return;
  // Set IR carrier frequency
  enableIROut(kFurrionFreq);

  for (uint16_t r = 0; r <= repeat; r++) {
    for (uint16_t offset=0; offset < nbytes; offset += kFurrionChillCubeBytesPerSection)
      // Section Header + Data + Footer
      sendGeneric(kFurrionHdrMark, kFurrionHdrSpace,
                  kFurrionBitMark, kFurrionOneSpace,
                  kFurrionBitMark, kFurrionZeroSpace,
                  kFurrionBitMark, kFurrionFooterSpace,
                  data + offset, kFurrionChillCubeBytesPerSection,
                  kFurrionFreq, true, 0, kDutyDefault);
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
  setPower(true);
}

/// Set up hardware to be able to send a message.
void IRFurrionChillCubeAC::begin(void) { _irsend.begin(); }

#if SEND_FURRION_CHILLCUBE
/// Send the current internal state as an IR message.
/// @param[in] repeat Nr. of times the message will be repeated.
void IRFurrionChillCubeAC::send(const uint16_t repeat) {
  if (!powerFlag) {  // "Off" is a 96bit message
    _irsend.sendFurrionChillCube(kFurrionChillCubeOff, sizeof(kFurrionChillCubeOff), repeat);
  } else {
    _irsend.sendFurrionChillCube(getRaw(), kFurrionChillCubeStateLength, repeat);
  }   // other 96bit messages are not yet supported
}
#endif  // SEND_FURRION_CHILLCUBE

/// Get a copy of the internal state as a valid code for this protocol.
/// @return A valid code for this protocol based on the current internal state.
unsigned char* IRFurrionChillCubeAC::getRaw(void) {
  setInvertBytes();
  setCheckSumS3();
  return _.raw;
}

/// Set the internal state from a valid code for this protocol.
/// @param[in] new_code A valid code for this protocol.
/// @param[in] length Size of the array being passed in in bytes.
void IRFurrionChillCubeAC::setRaw(const uint8_t new_code[], const uint16_t length) {
  const uint16_t len = min(length, kFurrionChillCubeStateLength);
  const uint16_t lenOff = sizeof(kFurrionChillCubeOff);
// Is it an off message?
  if (memcmp(kFurrionChillCubeOff, new_code, min(lenOff, len)) == 0)
    setPower(false);  // It is.
  else
    setPower(true);
  memcpy(_.raw, new_code, len);
}

void IRFurrionChillCubeAC::setPower(const bool on) {
  powerFlag = on;
}

bool IRFurrionChillCubeAC::getPower(void) const {
  return powerFlag;
}

void IRFurrionChillCubeAC::setTempRaw(const uint8_t code) {
  _.TempS1 = _.TempS2 = code >> 2;  // save bits 3-6 in S1 and S2
  _.TempS3 = code >> 1;             // save bit 2 in Section3
  _.TempS4 = code;                  // save bit 1 in Section3
}

/// Set the temp. in degrees
/// @param[in] temp Desired temperature in Degrees.
/// @param[in] fahrenheit Use units of Fahrenheit and set that as units used.
///   false is Celsius (Default), true is Fahrenheit.
void IRFurrionChillCubeAC::setTemp(const uint8_t temp, const bool fahrenheit) {
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
  uint8_t temp = (_.TempS1 << 2) + (_.TempS3 << 1) + _.TempS4;
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
  _.FanS1 = _.FanS2 = speed >> 6;  // save 3 bits in S1 and S2
  _.FanS3 = speed & 0b111111;      // save 6 bits in Section3
}

uint16_t IRFurrionChillCubeAC::getFan(void) const {
  return (_.FanS1 << 6) + _.FanS3;
}

/// Set the desired operation mode.
/// @param[in] mode The desired operation mode.
void IRFurrionChillCubeAC::setMode(const uint8_t mode) {
  _.ModeS1 = _.ModeS2 = mode >> 1;  // save 2 bits in S1 and S2
  _.ModeS3 = mode & 0b1;            // save 1 bit in Section3
  if (mode == kFurrionChillCubeAuto || mode == kFurrionChillCubeDry) {
    _.FanS1 = _.FanS2 = 0b000;      // save 3 bits in S1 and S2
    _.FanS3 = kFurrionChillCubeFanAuto0;    // save 6 bits in Section3
  }
}

uint8_t IRFurrionChillCubeAC::getMode(void) const {
  return (_.ModeS1 << 1) + _.ModeS3;
}

/// Set the Quiet mode of the A/C.
/// @param[in] on true, the setting is on. false, the setting is off.
void IRFurrionChillCubeAC::setQuiet(const bool on) {
  _.Quiet = on;                 // save 1 bit in Section3
  if (on)  // if Quiet is on, set Fan to Auto
    setFan(kFurrionChillCubeFanAuto);     // set Fan -> Auto
}

/// Get the Quiet mode of the A/C.
/// @return true, the setting is on. false, the setting is off.
bool IRFurrionChillCubeAC::getQuiet(void) const { return _.Quiet; }

/// Convert a stdAc::opmode_t enum into its native mode.
/// @param[in] mode The enum to be converted.
/// @return The native equivalent of the enum.
uint8_t IRFurrionChillCubeAC::convertMode(const stdAc::opmode_t mode) {
  switch (mode) {
    case stdAc::opmode_t::kCool:
      return kFurrionChillCubeCool;
    case stdAc::opmode_t::kHeat:
      return kFurrionChillCubeHeat;
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
  switch (speed) {
    case stdAc::fanspeed_t::kMin:
      return kFurrionChillCubeFan20;
    case stdAc::fanspeed_t::kLow:
      return kFurrionChillCubeFan40;
    case stdAc::fanspeed_t::kMedium:
      return kFurrionChillCubeFan60;
    case stdAc::fanspeed_t::kHigh:
      return kFurrionChillCubeFan80;
    case stdAc::fanspeed_t::kMax:
      return kFurrionChillCubeFan100;
    default:
      return kFurrionChillCubeFanAuto;
  }
}

/// Convert a native mode into its stdAc equivalent.
/// @param[in] mode The native setting to be converted.
/// @return The stdAc equivalent of the native setting.
stdAc::opmode_t IRFurrionChillCubeAC::toCommonMode(const uint8_t mode) {
  switch (mode) {
    case kFurrionChillCubeCool: return stdAc::opmode_t::kCool;
    case kFurrionChillCubeHeat: return stdAc::opmode_t::kHeat;
    case kFurrionChillCubeDry: return stdAc::opmode_t::kDry;
    case kFurrionChillCubeFan: return stdAc::opmode_t::kFan;
    default: return stdAc::opmode_t::kAuto;
  }
}

/// Convert a native fan speed into its stdAc equivalent.
/// @param[in] speed The native setting to be converted.
/// @return The stdAc equivalent of the native setting.
stdAc::fanspeed_t IRFurrionChillCubeAC::toCommonFanSpeed(const uint16_t speed) {
  switch (speed) {
    case kFurrionChillCubeFan100: return stdAc::fanspeed_t::kMax;
    case kFurrionChillCubeFan80: return stdAc::fanspeed_t::kHigh;
    case kFurrionChillCubeFan60: return stdAc::fanspeed_t::kMedium;
    case kFurrionChillCubeFan40: return stdAc::fanspeed_t::kLow;
    case kFurrionChillCubeFan20: return stdAc::fanspeed_t::kMin;
    default: return stdAc::fanspeed_t::kAuto;
  }
}

/// Convert the current internal state into its stdAc::state_t equivalent.
/// @return The stdAc equivalent of the native settings.
stdAc::state_t IRFurrionChillCubeAC::toCommon(void) const {
  stdAc::state_t result{};
  result.protocol = decode_type_t::FurrionChillCube;
  result.power = getPower();
  result.mode = toCommonMode(getMode());
  result.celsius = !getUseFahrenheit();
  result.degrees = getTemp();
  result.fanspeed = toCommonFanSpeed(getFan());
  result.quiet = getQuiet();
  // Not supported.
  result.model = -1;
  result.turbo = false;
  result.swingv = stdAc::swingv_t::kOff;
  result.swingh = stdAc::swingh_t::kOff;
  result.light = false;
  result.filter = false;
  result.econo = false;
  result.clean = false;
  result.beep = false;
  result.clock = -1;
  result.sleep = -1;
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
  result += addModeToString(mode, kFurrionChillCubeAuto, kFurrionChillCubeCool,
                            kFurrionChillCubeHeat, kFurrionChillCubeDry, kFurrionChillCubeFan);
  result += addFanToString(fan, static_cast<int>(stdAc::fanspeed_t::kMax),
                           static_cast<int>(stdAc::fanspeed_t::kMin),
                           static_cast<int>(stdAc::fanspeed_t::kAuto),
                           static_cast<int>(stdAc::fanspeed_t::kAuto),
                           static_cast<int>(stdAc::fanspeed_t::kMedium));
  result += addTempToString(getTemp(), !getUseFahrenheit());
  result += addBoolToString(_.Quiet, kQuietStr);
  return result;
}

void IRFurrionChillCubeAC::setInvertBytes() {
  for (uint8_t i = 0; i <= 10; i += 2) {
    _.raw[i + 1] = ~_.raw[i];
  }
}

void IRFurrionChillCubeAC::setCheckSumS3() {
  _.ChecksumS3 =  sumBytes(&(_.raw[12]), 5);
}

#if DECODE_FURRION_CHILL_CUBE
/// Decode the supplied Furrion 144-bit / 18-byte A/C message.
/// Status: STABLE / Confirmed Working.
/// @param[in,out] results Ptr to the data to decode & where to store the decode
///   result.
/// @param[in] offset The starting index to use when attempting to decode the
///   raw data. Typically/Defaults to kStartOffset.
/// @param[in] nbits The number of data bits to expect.
/// @param[in] strict Flag indicating if we should perform strict matching.
/// @return A boolean. True if it can decode it, false if it can't.
bool IRrecv::decodeFurrionChillCube(decode_results *results, uint16_t offset,
                            const uint16_t nbits, const bool strict) {
  if (results->rawlen < 2 * nbits +
                        kFurrionChillCubeNrOfSections * (kHeader + kFooter) -
                        1 + offset)
    return false;  // Can't possibly be a valid FurrionChillCube message.
  if (strict && nbits != kFurrionChillCubeBits)
    return false;      // Not strictly a FurrionChillCube message.
  if (nbits % 8 != 0)  // nbits has to be a multiple of nr. of bits in a byte.
    return false;
  if (nbits % kFurrionChillCubeNrOfSections != 0)
    return false;  // nbits has to be a multiple of kFurrionChillCubeNrOfSections.
  const uint16_t kSectionBits = nbits / kFurrionChillCubeNrOfSections;
  const uint16_t kSectionBytes = kSectionBits / 8;
  const uint16_t kNBytes = kSectionBytes * kFurrionChillCubeNrOfSections;
  // Capture each section individually
  for (uint16_t pos = 0, section = 0;
       pos < kNBytes;
       pos += kSectionBytes, section++) {
    uint16_t used = 0;
    // Section Header + Section Data + Section Footer
    used = matchGeneric(results->rawbuf + offset, results->state + pos,
                        results->rawlen - offset, kSectionBits,
                        kFurrionHdrMark, kFurrionHdrSpace,
                        kFurrionBitMark, kFurrionOneSpace,
                        kFurrionBitMark, kFurrionZeroSpace,
                        kFurrionBitMark, kFurrionFooterSpace,
                        section >= kFurrionChillCubeNrOfSections - 1,
                        _tolerance, kMarkExcess, true);
    if (!used) return false;  // Didn't match.
    offset += used;
  }

  // Compliance

  // Success
  results->decode_type = decode_type_t::FurrionChillCube;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}
#endif  // DECODE_FURRION_CHILL_CUBE
