// Copyright (C) 2024 Matías S. Ávalos (@tute_avalos)
// 
// This file is part of esp8266-io-board-websocket.
// 
// esp8266-io-board-websocket is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// esp8266-io-board-websocket is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with esp8266-io-board-websocket.  If not, see <https://www.gnu.org/licenses/>.

let gateway = `ws://${window.location.hostname}/ws`
let websocket

window.addEventListener('load', onLoad)

function onLoad(event) {
  initWebSocket()
}

function initWebSocket() {
  websocket = new WebSocket(gateway)
  websocket.onopen = onOpen
  websocket.onclose = onClose
  websocket.onmessage = onMessage
}

function onOpen(event) {
  console.log('Connection opened')
}

function onClose(event) {
  alert('Connection closed')
  setTimeout(initWebSocket, 2000)
}

function writeTextToLCD(row1, row2) {
  const charElements = document.querySelectorAll('.char');
  row1 = row1.padEnd(16, ' ').slice(0, 16);
  row2 = row2.padEnd(16, ' ').slice(0, 16);
  charElements.forEach((charElement, index) => {
    let row = (index < 16) ? row1 : row2;
    charElement.textContent = row[index % 16] || ' ';
  });
}

function setVisibility(element, on_off) {
  const el = document.querySelector(element);
  if (on_off) {
    el.classList.remove('hidden');
  } else {
    el.classList.add('hidden');
  }
}

function onMessage(event) {
  const data = JSON.parse(event.data)
  document.getElementById("btn1").innerHTML = (data.btn1 == 1) ? "on" : "off"
  document.getElementById("btn1").innerHTML += '<div class="button-inner center"></div>'
  document.getElementById("btn2").innerHTML = (data.btn2 == 1) ? "on" : "off"
  document.getElementById("btn2").innerHTML += '<div class="button-inner center"></div>'
  //document.getElementById("rgb").style.backgroundColor = data.rgb
  document.getElementById("rgb").jscolor.setPreviewElementBg(`${data.rgb}`)
  //document.getElementById("ldr").textContent = data.ldr
  document.getElementById("ldr").style.backgroundColor = `rgb(${data.ldr / 4},${data.ldr / 4},${data.ldr / 4})`
  setVisibility('.lcd', data.lcd_connected);
  if (data.lcd_connected) {
    writeTextToLCD(data.lcd1row, data.lcd2row);
  }
  setVisibility('.aht10', data.aht_connected);
  if(data.aht_connected) {
    document.getElementById('tmp').textContent = `T: ${data.tmp}°C`;
    document.getElementById('hum').textContent = `H: ${data.hum}%`;
  }
  setVisibility('.bh1750', data.bh_connected);
  if(data.bh_connected) {
    document.getElementById('lx').textContent = `${data.lx} lx`;
  }
}

function getData() {
  websocket.send("dat")
}

function setRGB(picker) {
  websocket.send(`rgb=${picker.toHEXString()}`)
}

function on_off(btn) {
  websocket.send(btn)
}

function showPrompt() {
  document.getElementById('custom-prompt').classList.remove('hidden');
}

document.getElementById('submit-prompt').addEventListener('click', () => {
  const input1 = document.getElementById('input1').value.padEnd(16, ' ').slice(0, 16);
  const input2 = document.getElementById('input2').value.padEnd(16, ' ').slice(0, 16);

  websocket.send(`lcd=0${input1}`);
  websocket.send(`lcd=1${input2}`);

  // Ocultar el prompt después de obtener los valores
  document.getElementById('custom-prompt').classList.add('hidden');
});

document.getElementById('cancel-prompt').addEventListener('click', () => {
  // Solo oculta el prompt sin hacer nada
  document.getElementById('custom-prompt').classList.add('hidden');
});

setInterval(getData, 50)
