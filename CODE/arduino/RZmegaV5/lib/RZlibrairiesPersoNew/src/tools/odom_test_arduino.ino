//Fichier : /tools/odom_test_arduino.ino

#include <Encoder.h>

Encoder encL(2,19);
Encoder encR(18,3);

void setup() {
  Serial.begin(115200);
  Serial.println("ODOM CALIB TEST READY");
  delay(500);
}

long lastL = 0, lastR = 0;
unsigned long lastTs = 0;

void loop() {
  long L = encL.read();
  long R = encR.read();

  long dL = L - lastL;
  long dR = R - lastR;

  unsigned long now = millis();
  unsigned long dt = now - lastTs;

  if (dt >= 100) {
    Serial.print("ticks L/R = ");
    Serial.print(L); Serial.print(" / "); Serial.print(R);

    Serial.print(" | delta L/R = ");
    Serial.print(dL); Serial.print(" / "); Serial.print(dR);

    Serial.print(" | dt= "); Serial.print(dt); Serial.println(" ms");

    lastL = L;
    lastR = R;
    lastTs = now;
  }
}
