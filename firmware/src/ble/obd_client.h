#pragma once

void obdClientBegin();
void obdClientTick();
bool obdClientConnected();
void obdClientQueryDtc();
void obdClientClearDtc();
