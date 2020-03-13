int last4 = 1;
int last5 = 1;

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

    pinMode(4, INPUT);
    pinMode(5, INPUT);

    pinMode(12, OUTPUT);
    pinMode(13, OUTPUT);

    lastTime = millis();
}

void processLeds()
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

int changeValueProtectSign(int val, int newVal)
{
    if (val >= 0)
        return abs(newVal);
    else
        return -1 * abs(newVal);
}

void processButtons()
{
    int i4 = digitalRead(4);
    if (i4 != last4)
    {
        last4 = i4;
        Serial.printf("[%lu] PIN4: %d\r\n", millis(), i4);

        if (i4)
            greenFadeSpeed = changeValueProtectSign(greenFadeSpeed, 1000);
        else
            greenFadeSpeed = changeValueProtectSign(greenFadeSpeed, 15000);
    }

    int i5 = digitalRead(5);
    if (i5 != last5)
    {
        last5 = i5;
        Serial.printf("[%lu] PIN5: %d\r\n", millis(), i5);

        if (i5)
            redFadeSpeed = changeValueProtectSign(redFadeSpeed, 1000);
        else
            redFadeSpeed = changeValueProtectSign(redFadeSpeed, 15000);
    }
}

void loop()
{
    processLeds();
    processButtons();
}
