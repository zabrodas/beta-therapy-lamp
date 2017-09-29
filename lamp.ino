const int lampCtrlPin=9;
const int speakerPin=10;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.print("Start");
  pinMode(lampCtrlPin, OUTPUT);
  pinMode(speakerPin, OUTPUT);
}

int t0=0;

void loop() {
  // put your main code here, to run repeatedly:
  int level=millis()&8191;
  int t=level&255;
  if (t<t0) {
    Serial.print(level==0?"\n":" ");
    Serial.print(level,DEC);
    analogWrite(lampCtrlPin, level/(8192/256));
  }
  t0=t;
  digitalWrite(speakerPin,(level/8)&1);
}
