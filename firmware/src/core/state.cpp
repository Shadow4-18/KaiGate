#include "core/state.h"

AppState &AppState::get() {
  static AppState inst;
  return inst;
}

AppState::AppState() { mutex_ = xSemaphoreCreateRecursiveMutex(); }

void AppState::lock() { xSemaphoreTakeRecursive(mutex_, portMAX_DELAY); }
void AppState::unlock() { xSemaphoreGiveRecursive(mutex_); }
