// Copyright (C) 2024 Matías S. Ávalos (@tute_avalos)
// 
// This file is part of PeriodicTaskManager.
// 
// PeriodicTaskManager is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// PeriodicTaskManager is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with PeriodicTaskManager.  If not, see <https://www.gnu.org/licenses/>.

/**
 * @file PeriodicTaskManager.cpp
 * @author Matías S. Ávalos (msavalos@gmail.com)
 * @brief Simple Periodic Tasks Managment. Implementation file.
 * @version 0.2
 * @date 2023-07-04
 * 
 * TODO: Escribir alguna descripción piola.
 * 
 * @copyright Copyright (c) 2022-2023
 * 
 */
#include <string.h>
#include "PeriodicTaskManager.h"

PeriodicTaskManager::PeriodicTaskManager() {
  for (int32_t i = 0; i < MAX_TASKS; i++) {
    _tasks[i].task = NULL;
    _tasks[i].ticks_ms = 0;
    _tasks[i].paused = false;
  }
}

PeriodicTaskManager::~PeriodicTaskManager()
{
}

int16_t PeriodicTaskManager::searchById(int16_t id) {
  for (uint16_t i = 0; i < MAX_TASKS; i++) {
    if (_tasks[i].id == id) return i;
  }
  return -1;
}

int16_t PeriodicTaskManager::searchByName(const char *name) {
  for (uint16_t i = 0; i < MAX_TASKS; i++) {
    if (!strcmp(_tasks[i].name, name)) return _tasks[i].id;
  }
  return -1;
}

uint8_t PeriodicTaskManager::add(void (*task)(uint8_t), const char *name, uint32_t ticks_ms) {
  uint8_t id = 0;
  if (ticks_ms > 0 and task != NULL and _runing < MAX_TASKS) {
    uint8_t freeSpot = 0;
    while (_tasks[freeSpot].task != NULL) freeSpot++;
    _tasks[freeSpot].id = _genid;
    _tasks[freeSpot].name = name;
    _tasks[freeSpot].task = task;
    _tasks[freeSpot].ticks_ms = ticks_ms;
    _tasks[freeSpot].paused = false;
    _tasks[freeSpot].next_ms = ticks_ms + millis();
    id = _genid;
#ifdef NDEBUG
    Serial.print(F("Added task \""));
    Serial.print(_tasks[freeSpot].name);
    Serial.print(F("\" to spot "));
    Serial.print(freeSpot);
    Serial.print(F(" of "));
    Serial.print(MAX_TASKS - 1);
    Serial.print(F(" at "));
    Serial.println(millis());
#endif
    _genid++;
    _runing++;
  }
  return id;
}

bool PeriodicTaskManager::delay(int16_t id, uint32_t ms) {
  int16_t index = this->searchById(id);
  if(index == -1) return false;
  _tasks[index].next_ms += ms;
#ifdef NDEBUG
  Serial.print(F("Delayed task \""));
  Serial.print(_tasks[index].name);
  Serial.print(F("\" "));
  Serial.print(ms);
  Serial.print(F("ms at "));
  Serial.println(millis());
#endif
  return true;
}

bool PeriodicTaskManager::delay(const char *name, uint32_t ms) {
  return this->delay(this->searchByName(name), ms);
}

bool PeriodicTaskManager::pause(int16_t id) {
  int16_t index = this->searchById(id);
  if(index == -1) return false;
#ifdef NDEBUG
  if (not _tasks[index].paused) {
    Serial.print(F("Paused task \""));
    Serial.print(_tasks[index].name);
    Serial.print(F("\" at "));
    Serial.println(millis());
  } else {
    Serial.printf("Task %i already paused\r\n", id);
  }
#endif
  _tasks[index].paused = true;
  return true;
}

bool PeriodicTaskManager::pause(const char *name) {
  return this->pause(this->searchByName(name));
}

bool PeriodicTaskManager::unpause(int16_t id) {
  int16_t index = this->searchById(id);
  if(index == -1) return false;
#ifdef NDEBUG
  if (_tasks[index].paused) {
    Serial.print(F("Unpaused task \""));
    Serial.print(_tasks[index].name);
    Serial.print(F("\" at "));
    Serial.println(millis());
  } else {
    Serial.printf("Task %i already running\r\n", id);
  }
#endif
  _tasks[index].paused = false;
  _tasks[index].next_ms = millis() + _tasks[index].ticks_ms;
  return true;
}

bool PeriodicTaskManager::unpause(const char *name) {
  return this->unpause(this->searchByName(name));
}

bool PeriodicTaskManager::remove(int16_t id) {
  int16_t index = this->searchById(id);
  if(index == -1) return false;
  _tasks[index].task = NULL;
  _tasks[index].ticks_ms = 0;
#ifdef NDEBUG
  Serial.print(F("Removed task \""));
  Serial.print(_tasks[index].name);
  Serial.print(F("\" at "));
  Serial.println(millis());
#endif
    return true;
}

bool PeriodicTaskManager::remove(const char *name) {
  return this->remove(this->searchByName(name));
}

bool PeriodicTaskManager::changeTicks(int16_t id, uint32_t ms) {
  int16_t index = this->searchById(id);
  if(index == -1) return false;
#ifdef NDEBUG
  Serial.print(F("Changed task \""));
  Serial.print(_tasks[index].name);
  Serial.print(F("\" from "));
  Serial.print(_tasks[index].ticks_ms);
  Serial.print(F("ms to "));
  Serial.print(ms);
  Serial.print(F("ms at "));
  Serial.println(millis());
#endif
  _tasks[index].ticks_ms = ms;
  return true;
}

bool PeriodicTaskManager::changeTicks(const char *name, uint32_t ms) {
  return this->changeTicks(this->searchByName(name), ms);
}

void PeriodicTaskManager::refresh() {
  uint32_t now = millis();
  for (int16_t i = 0; i < MAX_TASKS; i++) {
    if (_tasks[i].task != NULL and _tasks[i].ticks_ms != 0 and not _tasks[i].paused and _tasks[i].next_ms <= now) {
#ifdef NDEBUG
      Serial.print(F("Executing task \""));
      Serial.print(_tasks[i].name);
      Serial.print(F("\" at "));
      Serial.println(now);
#endif
      _tasks[i].task(i);
      _tasks[i].next_ms += _tasks[i].ticks_ms;
    }
  }
}
