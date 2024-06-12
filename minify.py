#!/usr/bin/env python3

# Copyright (C) 2024 Matías S. Ávalos (@tute_avalos)
# 
# This file is part of esp8266-io-board-websocket.
# 
# esp8266-io-board-websocket is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# esp8266-io-board-websocket is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with esp8266-io-board-websocket.  If not, see <https://www.gnu.org/licenses/>.

import sys
from csscompressor import compress as compress_css
from rjsmin import jsmin as minify_js
from htmlmin import minify as minify_html

def minify_file(file_path, file_type):
    # Leer contenido del archivo
    with open(file_path, 'r') as f:
        content = f.read()

    # Minificar según el tipo de archivo
    if file_type == 'css':
        minified_content = compress_css(content)
    elif file_type == 'js':
        minified_content = minify_js(content)
    elif file_type == 'html':
        minified_content = minify_html(content)
    else:
        print(f"Error: Unknown file type: {file_type}")
        sys.exit(1)

    return minified_content

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python minify.py <input_file> <output_file> <file_type>")
        sys.exit(1)

    input_file = sys.argv[1]
    output_file = sys.argv[2]
    file_type = sys.argv[3]

    minified_content = minify_file(input_file, file_type)

    # Escribir el contenido minificado en el archivo de salida
    with open(output_file, 'w') as f:
        f.write(minified_content)

    print(f"{file_type.upper()} minification complete.")
