let kegs = document.querySelectorAll('.js-kegs');
let kegNames = document.querySelectorAll('.js-nameChnage');
let tempReading = document.querySelector('.js-temp');
const wsUri = "ws://echo.websocket.org/";
const output = document.getElementById("output");

const btnsPour = document.querySelectorAll('.js-pour');
const btnsKegs = document.querySelectorAll('.js-newKeg');

const kegSize = 5; //in gallons
const oZinGallon = 128;
const ozInPint = 16;
const fullKegOz = kegSize * oZinGallon;
let kegFillLevel = [fullKegOz, fullKegOz];

function init() {

	//setInterval(getFakeData, 500);
	//testWebSocket();
}

function testWebSocket() {
	websocket = new WebSocket(wsUri);
	websocket.onopen = function(evt) {
		onOpen(evt)
	};
	websocket.onclose = function(evt) {
		onClose(evt)
	};
	websocket.onmessage = function(evt) {
		onMessage(evt)
	};
	websocket.onerror = function(evt) {
		onError(evt)
	};
}

function onOpen(evt) {
	writeToScreen("CONNECTED");
	doSend("WebSocket rocks");
}

function onClose(evt) {
	writeToScreen("DISCONNECTED");
}

function onMessage(evt) {
	writeToScreen('<span style="color: blue;">RESPONSE: ' + evt.data + '</span>');
	websocket.close();
}

function onError(evt) {
	writeToScreen('<span style="color: red;">ERROR:</span> ' + evt.data);
}

function doSend(message) {
	writeToScreen("SENT: " + message);
	websocket.send(message);
}

function writeToScreen(message) {
	var pre = document.createElement("p");
  pre.style.wordWrap = "break-word";
  pre.innerHTML = message;
  output.appendChild(pre);
}

function precentOfMax(amount, maxAmount) {
  return (amount / maxAmount) * 100;
}

function pourMeAColdOne(e) {
	e.preventDefault();
	const target = e.target;
	let kegNum;
	let newPercent;
	if(target.classList.contains('one')) {
		kegNum = 0;
	} else {
		kegNum = 1;
	}
	kegFillLevel[kegNum] = kegFillLevel[kegNum] - ozInPint;
	newPercent = precentOfMax(kegFillLevel[kegNum], fullKegOz);

	kegs[kegNum].querySelector('.progress').style.transform = 'translateY(-' + newPercent + '%)';
	kegs[kegNum].querySelector('.keg').textContent = newPercent.toPrecision(2) + '%';

}

function newKeg(e) {
	e.preventDefault();
	const target = e.target;
	if(target.classList.contains('one')) {
		kegNum = 0;
	} else {
		kegNum = 1;
	}
	kegFillLevel[kegNum] = fullKegOz;
	newPercent = precentOfMax(kegFillLevel[kegNum], fullKegOz);
	kegs[kegNum].querySelector('.progress').style.transform = 'translateY(-' + newPercent + '%)';
	kegs[kegNum].querySelector('.keg').textContent = newPercent + '%';

}

window.addEventListener("load", init);
btnsKegs[0].addEventListener('click', newKeg);
btnsKegs[1].addEventListener('click', newKeg);
btnsPour[0].addEventListener('click', pourMeAColdOne);
btnsPour[1].addEventListener('click', pourMeAColdOne);
