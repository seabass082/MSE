void setup() {
  Serial.begin(9600);
  pinMode(2,INPUT); //set up left line tracker
  pinMode(1, INPUT); //set up right line tracker

}

void loop() {
  int rawLeft = analogRead(A2);
  int rawRight = analogRead(A1);
  Serial.print("Left Line:");
  Serial.print(rawLeft);
  Serial.print("\n");
  Serial.print("Right Line:");
  Serial.print(rawRight);
  Serial.print("\n");
}