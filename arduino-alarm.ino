#include <P1AM.h>
#include <SPI.h>
#include <SD.h>
#include <Servo.h>
#include <Ethernet.h>
#include <RTCZero.h>

// Config data
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEF
};

byte ip[] = { 192, 168, 1, 170 };

RTCZero rtc;

IPAddress timeServer(216, 239, 35, 8); // Google time server

EthernetServer server(80);

EthernetUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets
const int timeZone = -4;  // Eastern Standard Time (USA)

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime() {
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * 3600;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

// FORM
char requestBuf[500];

char form[] = "<!DOCTYPE html>\n<html lang=\"en\">\n<head> <meta charset=\"utf-8\"> <title>Arduino Alarm</title> <link href=\"https://fonts.googleapis.com/css?family=Roboto&display=swap\" rel=\"stylesheet\"></head><style>body *{font-family: \'Roboto\', sans-serif; font-size: 15px; color: #eee; display: block; margin: 8px auto; text-align: center;}button{border: none; border-radius: 4px; padding: 8px 16px; color: #eaeaea; background-color: #494949}button:hover{background-color: rgb(126, 126, 126);}.modal-container{position: fixed; z-index: 1; padding-top: 50px; left: 0; top: 0; width: 100%; height: 100%; overflow: auto; transition-duration: 0.3s; background-color: rgb(0, 0, 0); background-color: rgba(0, 0, 0, 0.4);}.modal{position: relative; background-color: #353535; margin: 0 auto; padding: 0px; box-shadow: 0 4px 8px 0 rgba(0, 0, 0, 0.05), 0 6px 20px 0 rgba(0, 0, 0, 0.04);}#prompt div.modal{width: 400px; height: auto; border-radius: 4px; padding: 16px;}#prompt div.modal p{margin-top: 0px; margin-bottom: 10px; font-size: 15.4px; color: #eee;}#prompt div.modal p:last-of-type{margin-bottom: 50px;}#prompt button{margin-right: 12px;}.alarm-container{background-color: #555; padding: 2px; margin-bottom: 12px;}body>button{width: 100%;}button#newAlarm{margin-bottom: 12px;}.create-alarm .modal *{display: inline-flex;}.create-alarm .modal p{background-color: #555; padding: 9.5px; margin-top: 8px !important;}input{background-color: #555; border: none; width: 50px; height: 36px;}button.selected{color: black; background-color: #eee;}</style><script>const Ack=0; const Question=1; let ip=null; let alarms=new Array(); function addListener(selector, event, handler){document.querySelectorAll(selector).forEach(el=> el.addEventListener(event, handler));}function displayPrompt(text, type=Ack){return new Promise(resolve=>{let container=document.createElement(\"DIV\"); container.className=\"modal-container\"; container.id=\"prompt\"; let modal=document.createElement(\"DIV\"); modal.className=\"modal\"; container.appendChild(modal); if (typeof text===\"string\"){let strings=text.split(\"\\n\"); for (let i=0; i < strings.length; i++){let promptText=document.createElement(\"P\"); promptText.innerText=strings[i]; modal.appendChild(promptText);}}else if (text.nodeType !==undefined){modal.appendChild(text);}else if (Array.prototype.isPrototypeOf(text)){for (let i=0; i < text.length; i++){if (text[i].nodeType===undefined){return;}}for (let i=0; i < text.length; i++){modal.appendChild(text[i]);}}function deleteOnClick(scope){let modalContainer=scope.parentNode.parentNode; modalContainer.parentNode.removeChild(modalContainer);}if (type===Ack){let confirm=document.createElement(\"BUTTON\"); confirm.innerText=\"Ok\"; confirm.addEventListener(\"click\", function (){deleteOnClick(this); resolve();}); modal.appendChild(confirm);}else if (type===Question){let yes=document.createElement(\"BUTTON\"); yes.innerText=\"Yes\"; yes.addEventListener(\"click\", function (){deleteOnClick(this); resolve(true);}); modal.appendChild(yes); let no=document.createElement(\"BUTTON\"); no.innerText=\"No\"; no.addEventListener(\"click\", function (){deleteOnClick(this); resolve(false);}); modal.appendChild(no);}document.querySelector(\"body\").appendChild(container);});}function rebuildAlarms(){document.querySelectorAll(\"#alarmListContainer .alarm-container\").forEach(el=> el.remove()); alarms.forEach(alarm=> generateAlarmUI(alarm));}function generateAlarmUI(alarm){let container=document.createElement(\"DIV\"); container.className=\"alarm-container\"; let title=document.createElement(\"P\"); title.innerText=\"Alarm:\"; container.appendChild(title); let date=alarm.date; let hour=date.getHours(); let ampm=\"AM\"; if (hour > 12){hour -=12; ampm=\"PM\";}if (hour===0) hour=12; let minute=date.getMinutes().toString(); if (minute.length===1) minute=`0${minute}`; let timeString=`${hour.toString()}:${minute}${ampm}`; let time=document.createElement(\"P\"); time.innerText=timeString; container.appendChild(time); let deleteButton=document.createElement(\"BUTTON\"); deleteButton.innerText=\"Delete\"; deleteButton.addEventListener(\"click\", async ()=>{let response=await displayPrompt(\"Are you sure you want to delete this alarm?\", Question); if (response) alarms.splice(alarms.indexOf(alarm), 1); rebuildAlarms();}); container.appendChild(deleteButton); document.querySelector(\"#alarmListContainer\").appendChild(container);}function addAlarm(hour, minute){let date=new Date(); date.setHours(hour); date.setMinutes(minute); alarms.push({date: date}); rebuildAlarms();}async function setAlarms(){let arr=new Array(); alarms.forEach(alarm=>{arr.push({hour: alarm.date.getHours(), minute: alarm.date.getMinutes()});}); let response=await fetch(`http://${ip}/?operation=set&string=${JSON.stringify(arr)}`);}async function fetchAlarms(){let response=await fetch(`http://${ip}/?operation=fetch-alarm`); let synced=JSON.parse(await response.text()); alarms=new Array(); synced.forEach(alarm=>{date=new Date(); date.setHours(alarm.hour); date.setMinutes(alarm.minute); alarms.push({date: date});}); rebuildAlarms();}document.addEventListener(\"DOMContentLoaded\", function (){ip=window.location.href.substr(7); ip=ip.slice(0, ip.indexOf(\"/\")); addListener(\"#newAlarm\", \"click\", ()=>{document.querySelector(\"div.create-alarm\").style.display=\"\";}); addListener(\"#createAlarm\", \"click\", ()=>{let hour=parseInt(document.querySelector(\"#hour\").value); let minute=document.querySelector(\"#minute\").value; if (document.querySelector(\"#AM\").className !==\"selected\") hour +=12; addAlarm(hour, minute); document.querySelector(\"#cancel\").click();}); addListener(\"#syncAlarms\", \"click\", async ()=>{await setAlarms();}); addListener(\"#cancel\", \"click\", ()=>{document.querySelector(\"div.create-alarm\").style.display=\"none\";}); addListener(\"#AM\", \"click\", ()=>{document.querySelector(\"#PM\").className=\"\"; document.querySelector(\"#AM\").className=\"selected\";}); addListener(\"#PM\", \"click\", ()=>{document.querySelector(\"#AM\").className=\"\"; document.querySelector(\"#PM\").className=\"selected\";}); fetchAlarms();});</script><body style=\"background-color:#333030\"> <button id=\"newAlarm\">New Alarm</button> <div id=\"alarmListContainer\"> </div><button id=\"syncAlarms\">Sync Alarms</button> <div class=\"modal-container create-alarm\" id=\"prompt\" style=\"display: none;\"> <div class=\"modal\"> <div> <input id=\"hour\" type=\"number\" value=\"5\" min=\"1\" max=\"12\"> <p>:</p><input id=\"minute\" type=\"number\" value=\"0\" min=\"0\" max=\"59\"> </div><button id=\"AM\" class=\"selected\">AM</button> <button id=\"PM\">PM</button> <button id=\"createAlarm\">Create Alarm</button> <button id=\"cancel\">Cancel</button> </div></div></body>";
File webForm;
// Motor Config
Servo motor;  // create servo object to control a servo

int controllerPin = 7;  // analog pin used to connect the potentiometer
int freqStart = 1000;
int freqMax = 2000;
int frequency = freqStart;

// Alarm Config

const int deadzone = 1;

struct Alarm {
  int hour;
  int minute;
};

const int AlarmSize = sizeof(struct Alarm);

struct Alarm *alarms[10];

void clearAlarms() {
  for(int i = 0; i < 10; i++) {
    free(alarms[i]);
    alarms[i] = NULL;
  }
}

void generateNewAlarms(char* buf) {
  clearAlarms();
  Serial.println("Alarm Set Request:");
  Serial.println(buf);
  char* current = buf;
  for(int i = 0; i < 10; i++) {
    current = strchr(current, '{');
    if(current == NULL) break;
    current = strstr(current, "hour%22:");
    current += 8;
    char hourStr[2];
    memcpy(hourStr, current, 2);
    int hour = atoi(hourStr);

    current = strstr(current, "minute%22:");
    current += 10;
    char minStr[2];
    memcpy(minStr, current, 2);
    int minute = atoi(minStr);

    alarms[i] = (Alarm*)malloc(AlarmSize);
    alarms[i]->hour = hour;
    alarms[i]->minute = minute;
  }
  generateAlarmJSON();
}

char alarmJSON[500];

void generateAlarmJSON() {
  memset(alarmJSON, NULL, sizeof(alarmJSON));
  int jsonPos = 0;
  int alarmsFound = 0;
  strcpy(&alarmJSON[jsonPos++], "[");
  for(int i = 0; i < 10; i++) {
    if(alarms[i] != NULL) {
      if(alarmsFound) strcpy(&alarmJSON[jsonPos++], ",");
      strcpy(&alarmJSON[jsonPos], "{\"hour\":");
      jsonPos += 8;
      char hour[2];
      itoa(alarms[i]->hour, hour, 10);
      strcpy(&alarmJSON[jsonPos], hour);
      jsonPos += strlen(hour);
      strcpy(&alarmJSON[jsonPos], ",\"minute\":");
      jsonPos += 10;
      char minute[2];
      itoa(alarms[i]->minute, minute, 10);
      strcpy(&alarmJSON[jsonPos], minute);
      jsonPos += strlen(minute);
      strcpy(&alarmJSON[jsonPos++], "}");
      alarmsFound++;
    }
  }
  strcpy(&alarmJSON[jsonPos++], "]");
  printAlarmJSON();
}

void printAlarmJSON() {
  for(int i = 0; i < 500; i++) {
    if(alarmJSON[i] == NULL) break;
    Serial.print(alarmJSON[i]);
  }
  Serial.println();
}

File alarmSave;
byte *byteBuff;
unsigned long remaining = 0;

void saveToSD() {
  if(SD.exists("LAST")) {
    SD.remove("LAST");
  }
  alarmSave = SD.open("LAST", FILE_WRITE);
  for(int i = 0; i < 10; i++) {
    if(alarms[i] != NULL) {
      byteBuff = (byte *)alarms[i];
      alarmSave.write(byteBuff, AlarmSize);
    }
  }
  alarmSave.close();
}

void loadFromSD() {
  if(!SD.exists("LAST")) return;
  clearAlarms();
  alarmSave = SD.open("LAST", FILE_READ);
  remaining = alarmSave.size();
  for(int i = 0; i < 10; i++) {
    if(remaining < AlarmSize) break;
    alarms[i] = (Alarm*)malloc(AlarmSize);
    byteBuff = (byte *)alarms[i];
    alarmSave.read(byteBuff, AlarmSize);
    remaining -= AlarmSize;
  }
  alarmSave.close();
  generateAlarmJSON();
}

bool alarmTriggering = false;
uint32_t triggeredEpoch = 0;

void triggerAlarms() {
  int hours = rtc.getHours();
  int minutes = rtc.getMinutes();
  for(int i = 0; i < 10; i++) {
    if(alarms[i] != NULL) {
      if(hours == alarms[i]->hour && minutes > alarms[i]->minute - deadzone) {
        if(!alarmTriggering) {
          Serial.println("Alarm triggered!");
          alarmTriggering = true;
          triggeredEpoch = rtc.getEpoch();
          free(alarms[i]);
          alarms[i] = NULL;
          saveToSD();
          generateAlarmJSON();
        }
      }
    }
  }
}

int motorPower = freqMax;
int timeToRun = 30;

void handleTrigger() {
  if(alarmTriggering && rtc.getEpoch() < triggeredEpoch + timeToRun) {
    motor.writeMicroseconds(motorPower);
  } else if(alarmTriggering) {
    Serial.println("Alarm turned off!");
    alarmTriggering = false;
    motor.writeMicroseconds(1500);
  }
}

uint32_t lastSync = 0;

void setup() {
  Serial.begin(9600);
  // while(!Serial);

  if (!SD.begin(SDCARD_SS_PIN)) {
    Serial.println("initialization failed!");
  }

  loadFromSD();

  pinMode(SWITCH_BUILTIN, INPUT);
  motor.attach(controllerPin);  // attaches the servo on pin 9 to the servo object
  motor.writeMicroseconds(1500);

  rtc.begin();

  Ethernet.begin(mac, ip);
  server.begin();
  Udp.begin(localPort);

  lastSync = getNtpTime();
  rtc.setEpoch(lastSync);

  Serial.println("IP Address is:");
  Serial.println(Ethernet.localIP());
}

void loop() {
  EthernetClient client = server.available();
    if (client) {
      if(client.connected()) {
      Serial.println("Client found: ");
      if(client.available()) {
        int bufPos = 0;
        memset(requestBuf, 0x00, sizeof(requestBuf));
        while(client.available()) {
          requestBuf[bufPos++] = client.read();
        }
        Serial.println(requestBuf);
        Serial.println();
        Serial.println("responding...");
        // read bytes from the incoming client and write them back
        // to any clients connected to the server:
        if(strstr(requestBuf, "GET / HTTP/1.1") != NULL) {
          client.println("HTTP/1.1 200 OK");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Content-Type: text/html");
          client.println();
          int formLength = strlen(form);
          for(int i = 0; i < formLength; i++) {
            server.write(form[i]);
          }
          // webForm = SD.open("index.html", FILE_READ);
          // while(webForm.available()) {
          //   server.write(webForm.read());
          // }
          // webForm.close();
        }
        if(strstr(requestBuf, "GET /?operation=fetch") != NULL) {
          client.println("HTTP/1.1 200 OK");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Content-Type: text/plain");
          client.println();
          // client.print("bababooey");
          for(int i = 0; i < 500; i++) {
            if(alarmJSON[i] == NULL) break;
            // Serial.print(alarmJSON[i]);
            client.print(alarmJSON[i]);
          }
          client.println();
        }
        if(strstr(requestBuf, "GET /?operation=set&string=") != NULL) {
          client.println("HTTP/1.1 200 OK");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println("Content-Type: text/plain");
          client.println();

          client.print("bababooey");
          client.println();
          generateNewAlarms(strstr(requestBuf, "operation=set&string="));
          saveToSD();
        }
        Serial.println("Responded.");
      }
    }
    delay(1);
    client.stop();
    Serial.println("Client disconnected.");
  }
  triggerAlarms();
  handleTrigger();

  if(rtc.getEpoch() > lastSync + 3600) {
    lastSync = getNtpTime();
    rtc.setEpoch(lastSync);
  }

  delay(200);
}
