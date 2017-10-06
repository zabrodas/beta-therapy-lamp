static const int lampCtrlPin=9;
static const int speakerPin1=3;
static const int speakerPin2=4;
static const int wifiResetPin=2;
static const unsigned apJoinTimeout=30000u;
static const unsigned getStatusTimeout=1000u;

//const char ssid[] = "gzabrodina";
//const char pass[] = "pusya756nadya";
const char ssid[] = "az-theater";
const char pass[] = "internal17";
const char hostname[]="drunken-lamp";


void lamp_ctrl_on();
void lamp_ctrl_off();
void lamp_ctrl_sound1();
void lamp_ctrl_sound2();
void lamp_ctrl_burn();
void lamp_ctrl_test();
void lamp_ctrl_blink();


class Buffer {
  byte *data; int datasize;
  int w,r,c;
 public:
  Buffer(byte *data, int datasize) { w=r=c=0; this->data=data; this->datasize=datasize; }
  void put(byte b) {
     data[w]=b; if (++w>=datasize) w=0; c++;
     if (c>datasize) {
        if (++r>=datasize) r=0;
        c--;
     }
  }
  int available() {
    return c;
  }
  int peek() {
    if (c<=0) return -1;
    return data[r];
  }
  int get() {
    if (c<=0) return -1;
    byte b=data[r];
    if (++r>=datasize) r=0;
    c--;
    return b; 
  }
  int match(byte *templ, int len) {
      int r1=r,c1=c;
      for (int i=0; i<len; i++) {
        if (c1==0) return 2;
        byte a=data[r1];
        byte b=templ[i];
        if (a!=b) {
          return 0;
        }
        if (++r1>=datasize) r1=0;
        c1--;
      }
      r=r1; c=c1;
      return 1;
  }
  void drop() {
    r=w=c=0;
  }
};

class WiFiServer {
  byte bufferdata[500];  
  Buffer buf;
  int step_number;
  unsigned long step_time;
  unsigned long statusWatchDog;
  byte waitForStatusReply, serverCreated;
  unsigned long statusReplyWatchDog;
  int lastClientId, clientId, dataLen,sc;

  byte *dataToSend; int lenDataToSend;
 public:

#define STEP if (step_number<=__LINE__) { {step_number=__LINE__;  }
#define STEP_DELAYED(dt) if (step_number<=__LINE__) { {step_number=__LINE__; if (millis()-step_time<(dt)) return step_result;}
#define EXIT_STEP {step_number=__LINE__; step_time=millis(); return step_result;}
#define END_STEP EXIT_STEP }
#define SET_STEP_RESULT(r) { step_result=(r); }
#define REPEAT_STEP {return step_result;}
#define RETRY_STEP(dt) { if (millis()-step_time<(dt)) return step_result; }
#define TIMEOUT(dt) (millis()-step_time>=(dt))
#define RESTART_STEPS { Serial.print("\r\nWiFi reset in line "); Serial.print(__LINE__); Serial.print(" "); while (buf.available()>0) Serial.print((char)buf.get()); Serial.print("\r\n"); step_number=0; clientId=-1; step_time=millis(); return step_result;}
#define LOOP for (;;) { int LOOP_START_LINE=__LINE__;
#define END_LOOP {if (step_number>__LINE__) break; step_number=LOOP_START_LINE; return step_result;} } { if (step_number<__LINE__) step_number=__LINE__; }
#define BREAK {break;}
#define BUF_MATCH(s) (buf.match( (byte*)(s), sizeof(s)-1 ))
#define BLOCK LOOP
#define END_BLOCK BREAK; END_LOOP
#define GOTO(l) { step_number=0; goto l; }
#define WAIT_FOR_REPLY(reply, timeout) switch (BUF_MATCH(reply)) { case 0: buf.get(); case 2: RETRY_STEP(timeout); RESTART_STEPS; }
#define WAIT_FOR_STR(str) switch (BUF_MATCH(str)) { case 0: buf.get(); case 2: REPEAT_STEP; }



  WiFiServer(): buf(bufferdata,sizeof(bufferdata)) { step_number=0;  sc=0; lastClientId=-1; }

  void sendData(byte *data, int datalen) {
      dataToSend=data; lenDataToSend=datalen;
  }
  
  int receiveData() {

    int step_result=-1;
    
    while (Serial.available()) {
      char b=Serial.read();
      if (sc>0) {
        sc--;
      } else if (b=='[') {
        sc=2;
      } else if (b=='(') {
        sc=3;
      } else if (b<32) {
      } else {
        //Serial.print('[');
        //Serial.print(b);
        //Serial.print(']');
      }
      buf.put(b);
    }

    STEP
        pinMode(wifiResetPin, OUTPUT);
        digitalWrite(wifiResetPin, LOW);
    END_STEP
    STEP_DELAYED(10)
        buf.drop();
        digitalWrite(wifiResetPin, HIGH);
    END_STEP
    STEP    
        switch (BUF_MATCH("\r\nready\r\n")) {
          case 0:
            buf.get();
          case 2:
            RETRY_STEP(5000);
            RESTART_STEPS;
        }
    END_STEP 

    STEP
      Serial.print("\r\nATE0\r\n");
    END_STEP 
    STEP
      WAIT_FOR_REPLY("\r\nOK\r\n",500);
    END_STEP
    STEP
      Serial.print("AT+CWMODE=1\r\n");
    END_STEP 
    STEP
      WAIT_FOR_REPLY("\r\nOK\r\n",500);
    END_STEP
    STEP
      Serial.print("\r\nAT+CIPMUX=1\r\n");
    END_STEP
    STEP
      WAIT_FOR_REPLY("\r\nOK\r\n",500);
    END_STEP
    STEP
      Serial.print("\r\nAT+CWJAP=\"");
      Serial.print(ssid);
      Serial.print("\",\"");
      Serial.print(pass);
      Serial.print("\"\r\n");
    END_STEP
    STEP
      WAIT_FOR_REPLY("GOT IP",apJoinTimeout);
    END_STEP
    STEP
      WAIT_FOR_REPLY("\r\nOK\r\n",5000);
    END_STEP
    STEP
      statusWatchDog=millis()-apJoinTimeout+1000;
      waitForStatusReply=0;
      serverCreated=0;
    END_STEP

    LOOP
      STEP
        if (!dataToSend && millis()-statusWatchDog>=apJoinTimeout) { // send status request
          Serial.print("\r\nAT+CIPSTATUS\r\n");
          statusWatchDog=statusReplyWatchDog=millis();
          waitForStatusReply=1;
        }
      END_STEP
      STEP
        if (!waitForStatusReply && dataToSend) {
          Serial.print("\r\nAT+CIPSEND=");
          Serial.print(lastClientId);
          Serial.print(",");
          Serial.print(lenDataToSend);
          Serial.print("\r\n");
          GOTO(labelSendData);
        }
      END_STEP

      STEP
        int x=BUF_MATCH("\r\nSTATUS:");
        if (x==1) GOTO(labelStatusReceiving);
        int y=BUF_MATCH("\r\n+IPD,");
        if (y==1) GOTO(labelDataReceiving);
        if (x==0 && y==0) buf.get();
        if (waitForStatusReply && millis()-statusReplyWatchDog>=getStatusTimeout) {
            RESTART_STEPS;
        }
        GOTO(labelContinue);
      END_STEP

  labelStatusReceiving:
      STEP
        if (!waitForStatusReply) GOTO(labelContinue);
        if (!buf.available()) {
          RETRY_STEP(100);
          RESTART_STEPS;
        }
        char status=buf.get();
        switch (status) {
          case '2': case '3': case '4': // joined AP
            waitForStatusReply=0;
            if (!serverCreated) {
              Serial.print("\r\nAT+CIPSERVER=1,8080\r\n");
              serverCreated=1;
              lamp_ctrl_test();
            }
            GOTO(labelContinue);
          case '5': // No access to AP
          default: // wrong reply
            RESTART_STEPS;
        }
      END_STEP

  labelDataReceiving:
      STEP
        clientId=0;
      END_STEP
      STEP
        if (buf.available()>0) {
          char b=buf.get();
          if (b>='0' && b<='9') {
            clientId*=10; clientId+=b-'0';
            REPEAT_STEP;  // ready for next digit
          } else if (b==',') { // field delimiter
            dataLen=0;  
          } else {
            RESTART_STEPS; // wrong reply
          }
        } else {
          RETRY_STEP(100);  // wait and reset if timeout
          RESTART_STEPS;
        }
      END_STEP
      STEP
        if (buf.available()>0) {
          char b=buf.get();
          if (b>='0' && b<='9') {
            dataLen*=10; dataLen+=b-'0';
            REPEAT_STEP;  // ready for next digit
          } else if (b==':') { // field delimiter

          } else {
            RESTART_STEPS; // wrong reply
          }
        } else {
          RETRY_STEP(100);  // wait and reset if timeout
          RESTART_STEPS;
        }
      END_STEP
      STEP
        if (dataLen>0) {
          lastClientId=clientId;
          if (buf.available()>0) {
            byte b=buf.get(); // data received
            dataLen--;
            SET_STEP_RESULT(b);
            REPEAT_STEP;
          } else {
            RETRY_STEP(500);  // wait and reset if timeout
            RESTART_STEPS;
          }
        }
      END_STEP
      STEP
        GOTO(labelContinue);
      END_STEP

  labelSendData:
    STEP_DELAYED(500)
      for (int i=0; i<lenDataToSend; i++) Serial.print((char)dataToSend[i]);
      dataToSend=0;
      GOTO(labelContinue);
    END_STEP

  labelContinue:;
    END_LOOP

    RESTART_STEPS;
  }
};

WiFiServer wifiServer;

class HttpServer {
  byte bufferdata[50];  
  Buffer buf;
  int step_number;
  unsigned long step_time;
 public:
  HttpServer(): buf(bufferdata,sizeof(bufferdata)) { step_number=0; }

  void sendReply() {
    const static byte httpReply[]=
      "HTTP/1.0 200 OK\r\n"
      "Content-Type: text/plain\r\n"
      "Content-Length: 4\r\n"
      "\r\n"
      "LAMP\r\n";
    wifiServer.sendData(httpReply,sizeof(httpReply)-1);
  }
  int loop() {
    int step_result=-1;

    int b=wifiServer.receiveData();
    if (b>=0) {
      buf.put(b);
    }
    LOOP
      STEP
        WAIT_FOR_STR("GET /");
      END_STEP
      STEP
        int wrongPath=1;
        switch (BUF_MATCH("on ")) {
          case 1: sendReply(); lamp_ctrl_on(); EXIT_STEP
          case 2: wrongPath=0;
        }
        switch (BUF_MATCH("off ")) {
          case 1: sendReply(); lamp_ctrl_off(); EXIT_STEP
          case 2: wrongPath=0;
        }
        switch (BUF_MATCH("sound1 ")) {
          case 1: sendReply(); lamp_ctrl_sound1(); EXIT_STEP
          case 2: wrongPath=0;
        }
        switch (BUF_MATCH("sound2 ")) {
          case 1: sendReply(); lamp_ctrl_sound2(); EXIT_STEP
          case 2: wrongPath=0;
        }
        switch (BUF_MATCH("burn ")) {
          case 1: sendReply(); lamp_ctrl_burn(); EXIT_STEP
          case 2: wrongPath=0;
        }
        switch (BUF_MATCH("test ")) {
          case 1: sendReply(); lamp_ctrl_test(); EXIT_STEP
          case 2: wrongPath=0;
        }
        switch (BUF_MATCH("blink ")) {
          case 1: sendReply(); lamp_ctrl_blink(); EXIT_STEP
          case 2: wrongPath=0;
        }
        if (wrongPath) {
          buf.get();
        } else {
          REPEAT_STEP
        }
      END_STEP
    END_LOOP
  }
};

HttpServer httpServer;

class LampControl {
  int step_number;
  int request;
  unsigned long duration, step_time;
public:  
  LampControl() { step_number=0; request=2; }
  int loop() {
    int step_result=-1;
    switch (request) {
      case 0: break;
      case 1: request=0; GOTO(labelOn);
      case 2: request=0; GOTO(labelOff);
      case 3: request=0; GOTO(labelSound1);
      case 4: request=0; GOTO(labelSound2);
      case 5: request=0; GOTO(labelburn);
      case 6: request=0; GOTO(labeltest);
      case 7: request=0; GOTO(labelBlink);
      default: request=0; return;
    }

  labeltest:
    STEP
      analogWrite(lampCtrlPin, 128);
      duration=millis();
    END_STEP
    LOOP
      STEP_DELAYED(20)
        digitalWrite(speakerPin1, LOW);
        digitalWrite(speakerPin2, HIGH);
      END_STEP
      STEP_DELAYED(20)
        digitalWrite(speakerPin1, HIGH);
        digitalWrite(speakerPin2, LOW);
        if (millis()-duration>=200u) {
          BREAK;
        }
      END_STEP
   END_LOOP
   STEP
        digitalWrite(speakerPin1, LOW);
        digitalWrite(speakerPin2, LOW);
        GOTO(labelOff);
   END_STEP

  labelBlink:
    LOOP
      STEP_DELAYED(10)
        digitalWrite(lampCtrlPin, HIGH);
      END_STEP
      STEP_DELAYED(10)
        digitalWrite(lampCtrlPin, LOW);
      END_STEP
    END_LOOP
  
  labelOff:
    STEP
      digitalWrite(lampCtrlPin, LOW);
    END_STEP
    STEP
      REPEAT_STEP
    END_STEP

  labelOn:
    STEP
      digitalWrite(lampCtrlPin, HIGH);
    END_STEP
    STEP
      REPEAT_STEP
    END_STEP

  labelSound1:
    STEP
      analogWrite(lampCtrlPin, 96);
      duration=millis();
    END_STEP
    LOOP
      STEP_DELAYED(20)
        digitalWrite(speakerPin1, LOW);
        digitalWrite(speakerPin2, HIGH);
      END_STEP
      STEP_DELAYED(20)
        digitalWrite(speakerPin1, HIGH);
        digitalWrite(speakerPin2, LOW);
        if (millis()-duration>=3000u) {
          BREAK;
        }
      END_STEP
   END_LOOP
   STEP_DELAYED(20)
        analogWrite(lampCtrlPin, 128);
        digitalWrite(speakerPin1, LOW);
        digitalWrite(speakerPin2, LOW);
   END_STEP
   STEP
    GOTO(labelBlink);
   END_STEP
  
  labelSound2:
    STEP
      analogWrite(lampCtrlPin, 196);
      duration=millis();
    END_STEP
    LOOP
      STEP_DELAYED(10)
        digitalWrite(speakerPin1, LOW);
        digitalWrite(speakerPin2, HIGH);
      END_STEP
      STEP_DELAYED(10)
        digitalWrite(speakerPin1, HIGH);
        digitalWrite(speakerPin2, LOW);
        if (millis()-duration>=3000u) {
          BREAK;
        }
      END_STEP
   END_LOOP
   STEP_DELAYED(10)
        analogWrite(lampCtrlPin, 128);
        digitalWrite(speakerPin1, LOW);
        digitalWrite(speakerPin2, LOW);
   END_STEP
   STEP
    GOTO(labelBlink);
   END_STEP

  labelburn:
    STEP
      analogWrite(lampCtrlPin, 64);
      duration=millis();
    END_STEP
    LOOP
      STEP_DELAYED(5)
        digitalWrite(speakerPin1, LOW);
        digitalWrite(speakerPin2, HIGH);
      END_STEP
      STEP_DELAYED(5)
        digitalWrite(speakerPin1, HIGH);
        digitalWrite(speakerPin2, LOW);
        if (millis()-duration>=2000u) {
          BREAK;
        }
      END_STEP
   END_LOOP
   STEP
      digitalWrite(lampCtrlPin,HIGH);
      duration=millis();
   END_STEP
   LOOP
      STEP_DELAYED(0)
        digitalWrite(speakerPin1, LOW);
        digitalWrite(speakerPin2, HIGH);
      END_STEP
      STEP_DELAYED(0)
        digitalWrite(speakerPin1, HIGH);
        digitalWrite(speakerPin2, LOW);
        if (millis()-duration>=100u) {
          BREAK;
        }
      END_STEP
   END_LOOP
   STEP
      digitalWrite(lampCtrlPin, LOW);
      digitalWrite(speakerPin1, LOW);
      digitalWrite(speakerPin2, LOW);
   END_STEP
   STEP
    REPEAT_STEP
   END_STEP
  }

  void lamp_ctrl_on() { request=1; }
  void lamp_ctrl_off() { request=2; }
  void lamp_ctrl_sound1() { request=3; }
  void lamp_ctrl_sound2() { request=4; }
  void lamp_ctrl_burn() { request=5; }
  void lamp_ctrl_test() { request=6; }
  void lamp_ctrl_blink() { request=7; }
};

LampControl lampControl;

void lamp_ctrl_on() { lampControl.lamp_ctrl_on(); }
void lamp_ctrl_off() { lampControl.lamp_ctrl_off(); }
void lamp_ctrl_sound1() { lampControl.lamp_ctrl_sound1(); }
void lamp_ctrl_sound2() { lampControl.lamp_ctrl_sound2(); }
void lamp_ctrl_burn() { lampControl.lamp_ctrl_burn(); }
void lamp_ctrl_test() { lampControl.lamp_ctrl_test(); }
void lamp_ctrl_blink() { lampControl.lamp_ctrl_blink(); }


void setup() {

  pinMode(speakerPin1, OUTPUT);
  pinMode(speakerPin2, OUTPUT);
  digitalWrite(speakerPin1, LOW);
  digitalWrite(speakerPin2, LOW);
  
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.print("Start");

  pinMode(lampCtrlPin, OUTPUT);
//  TCCR2A = TCCR2A & 0xe0 | 3;
//  TCCR2B = TCCR2B & 0xe0 | 0x0c;
  analogWrite(lampCtrlPin, 0);
}


void loop() {
  httpServer.loop();
  lampControl.loop();
}


