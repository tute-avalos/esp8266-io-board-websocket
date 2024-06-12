#!/bin/bash

# Copyright (C) 2024 Matías S. Ávalos (@tute_avalos)
#
# This file is part of esp8266-io-board-websocket.
#
# esp8266-io-board-websocket is free software: you can redistribute it and/or
# modify it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or (at your
# option) any later version.
#
# esp8266-io-board-websocket is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of 
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
# Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with esp8266-io-board-websocket.  If not, see 
# <https://www.gnu.org/licenses/>.

./populate_data.sh

source ~/.platformio/penv/bin/activate

pio run
pio run -t upload
pio run -t buildfs
pio run -t uploadfs

deactivate
