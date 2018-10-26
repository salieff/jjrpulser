int last4 = 1;
int last5 = 1;

void setup()
{
    Serial.begin(74880);
    Serial.flush();
    Serial.printf("\r\nJJR Pulser Setup\r\n");
    Serial.flush();

    pinMode(4, INPUT);
    pinMode(5, INPUT);
}

void loop()
{
    int i4 = digitalRead(4);
    if (i4 != last4)
    {
        last4 = i4;
        Serial.printf("[%lu] PIN4: %d\r\n", millis(), i4);
    }

    int i5 = digitalRead(5);
    if (i5 != last5)
    {
        last5 = i5;
        Serial.printf("[%lu] PIN5: %d\r\n", millis(), i5);
    }
}
