int maxValue = PWMRANGE;
int greenValue = 0;
int redValue = maxValue;

int greenFadeSpeed = 1000;
int redFadeSpeed = -1000;

unsigned long lastTime = 0;

void setup()
{
    Serial.begin(74880);
    Serial.flush();
    Serial.printf("\r\nJJR Pulser Setup\r\n");
    Serial.flush();

    pinMode(12, OUTPUT);
    pinMode(13, OUTPUT);

    lastTime = millis();
}

void loop()
{
  unsigned long timestamp = millis();
  unsigned long timedelta = timestamp - lastTime;

  if (timedelta == 0)
      return;

  int greenDelta = greenFadeSpeed * (int)timedelta / 1000;
  if (greenDelta == 0)
      return;

  greenValue += greenDelta;
  if ((greenValue <= 0 && greenFadeSpeed < 0) || (greenValue >= maxValue && greenFadeSpeed > 0))
      greenFadeSpeed *= -1;

  if (greenValue <= 0)
      greenValue = 0;

  if (greenValue >= maxValue)
      greenValue = maxValue;

  analogWrite(12, greenValue);

  int redDelta = redFadeSpeed * (int)timedelta / 1000;
  if (redDelta == 0)
      return;

  redValue += redDelta;
  if ((redValue <= 0 && redFadeSpeed < 0) || (redValue >= maxValue && redFadeSpeed > 0))
      redFadeSpeed *= -1;

  if (redValue <= 0)
      redValue = 0;

  if (redValue >= maxValue)
      redValue = maxValue;

  analogWrite(13, redValue);

  lastTime = timestamp;
}
