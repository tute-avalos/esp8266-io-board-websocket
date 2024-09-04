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

function onMessage(event) {
  const data = JSON.parse(event.data)
  document.getElementById("btn1").innerHTML = (data.btn1 == 1) ? "on" : "off"
  document.getElementById("btn1").innerHTML += '<div class="button-inner center"></div>'
  document.getElementById("btn2").innerHTML = (data.btn2 == 1) ? "on" : "off"
  document.getElementById("btn2").innerHTML += '<div class="button-inner center"></div>'
  //document.getElementById("rgb").style.backgroundColor = data.rgb
  document.getElementById("rgb").jscolor.setPreviewElementBg(`${data.rgb}`)
  //document.getElementById("ldr").textContent = data.ldr
  document.getElementById("ldr").style.backgroundColor = `rgb(${data.ldr/4},${data.ldr/4},${data.ldr/4})`
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

setInterval(getData, 50)
