#pragma once

#include <stddef.h>

void gattServerBegin();
void gattServerTick();
bool gattAppConnected();
void gattNotifyTelemetry();
void gattNotifyImu();
void gattNotifyDtc();
void gattNotifyStatus(const char *json);
void gattNotifyMediaAction(const char *action);
