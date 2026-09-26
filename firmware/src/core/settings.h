#pragma once

#include "types.h"

bool settingsBegin();
void settingsLoad(UserConfig &cfg);
void settingsSave(const UserConfig &cfg);
void settingsSaveGates(const UserConfig &cfg);
void settingsSaveBests(float best060, float bestShiftMs);
void settingsLoadBests(float &best060, float &bestShiftMs);
void settingsResetGates(UserConfig &cfg);
