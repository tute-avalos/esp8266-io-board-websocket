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
 * @file PeriodicTaskManager.h
 * @author Matías S. Ávalos (msavalos@gmail.com)
 * @brief Simple Periodic Tasks Managment. Header file.
 * @version 0.2
 * @date 2023-07-04
 * 
 * @copyright Copyright (c) 2022-2023
 * 
 */
#ifndef __PERIODICTASKMANAGER_H__
#define __PERIODICTASKMANAGER_H__

#include "Arduino.h"

#ifndef MAX_TASKS
#define MAX_TASKS 20
#endif
/**
 * @brief Periodic Task Managment
 * 
 * Interface para ejecutar tareas (funciones) de forma periódica cada X ms
 * 
 */
class PeriodicTaskManager {
private:
  struct Task {
    const char *name;
    void (*task)(uint8_t);
    uint32_t ticks_ms;
    uint32_t next_ms;
    uint8_t id;
    bool paused;
  };
  Task _tasks[MAX_TASKS];
  uint8_t _genid = 0;
  uint8_t _runing = 0;

private:
  int16_t searchByName(const char *name);
  int16_t searchById(int16_t id);

public:
  uint8_t add(void (*task)(uint8_t), const char *name, uint32_t ticks_ms);
  bool changeTicks(int16_t id, uint32_t ms);
  bool changeTicks(const char *name, uint32_t ms);
  bool delay(int16_t id, uint32_t ms);
  bool delay(const char *name, uint32_t ms);
  bool pause(int16_t id);
  bool pause(const char *name);
  bool unpause(int16_t id);
  bool unpause(const char *name);
  bool remove(int16_t id);
  bool remove(const char *name);
  void refresh();

public:
  PeriodicTaskManager();
  virtual ~PeriodicTaskManager();
};

#endif // __PERIODICTASKMANAGER_H__
