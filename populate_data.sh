#!/bin/bash

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

# Function to check if a command exists
command_exists() {
    command -v "$1" &>/dev/null
}

is_missing_package=false 
# Check for gzip
if ! command_exists gzip; then
    echo "gzip no está instalado. Lo puede instalar de la siguiente manera:"
    echo "sudo apt-get install gzip"
    is_missing_package=true
fi

# Verificar si los paquetes python3-rjsmin, python3-csscompressor y python3-htmlmin están instalados
paquetes_a_verificar=(python3-rjsmin python3-csscompressor python3-htmlmin)

for paquete in "${paquetes_a_verificar[@]}"; do
    if ! dpkg -s "$paquete" > /dev/null  2>&1; then
        echo "El paquete $paquete no está instalado. Lo puede instalar de la siguiente manera:"
        echo "sudo apt-get install $paquete"
        is_missing_package=true
    fi
done

if [ $is_missing_package = true ]; then
    exit 1
fi

# Obtener el directorio de origen y destino
SRC_DIR="${1:-./html}"
DEST_DIR="${2:-./data}"

# Eliminar el directorio de destino si existe junto con su contenido
if [ -d "$DEST_DIR" ]; then
    echo "Eliminando la carpeta destino ya existente: $DEST_DIR"
    rm -rf "$DEST_DIR"
fi

# Crear el directorio de destino si no existe
mkdir -p "$DEST_DIR"

# Obtener una lista de archivos en el directorio de origen (sin incluir la carpeta de origen)
files=($(find "$SRC_DIR" -mindepth 1 -type f -exec bash -c 'echo "${1#$2/}"' _ {} "$SRC_DIR" \;))

# Aplicar gzip a los archivos CSS, HTML y JS y mover los comprimidos al directorio de destino correspondiente
for file in "${files[@]}"; do
    # Obtener la extensión del archivo
    extension="${file##*.}"

    # Comprimir solo los archivos CSS, HTML y JS
    if [[ "$extension" =~ ^(css|html|js)$ ]]; then
        # Directorio relativo del archivo en el directorio de destino
        dest_dir="$DEST_DIR/$(dirname "$file")"
        mkdir -p "$dest_dir"

        # Evitar archivos que ya están minificados
        if [[ "$file" == *".min."* ]]; then
            minifyed_file="$file"
            # Comprimir el archivo y moverlo al directorio de destino
            gzip -c "$SRC_DIR/$minifyed_file" >"$dest_dir/$(basename "$file").gz"
            echo "Se comprimió el archivo: $SRC_DIR/$minifyed_file -> $dest_dir/$(basename "$file").gz"
        else
            minifyed_file="${file%.*}.min.$extension"
            python3 minify.py $SRC_DIR/$file $SRC_DIR/$minifyed_file $extension 1>/dev/null
            # Comprimir el archivo y moverlo al directorio de destino
            gzip -c "$SRC_DIR/$minifyed_file" >"$dest_dir/$(basename "$file").gz"
            echo "Se comprimió y generó el archivo: $SRC_DIR/$minifyed_file -> $dest_dir/$(basename "$file").gz"
            rm -vf "$SRC_DIR/$minifyed_file"
        fi

    else
        # Directorio relativo del archivo en el directorio de destino
        dest_file="$DEST_DIR/$file"
        mkdir -p "$(dirname "$dest_file")"

        # Copiar el archivo al directorio de destino
        cp "$SRC_DIR/$file" "$dest_file"
        echo "Copiado: $SRC_DIR/$file -> $dest_file"
    fi
done

echo "Compresión y copia completa."
