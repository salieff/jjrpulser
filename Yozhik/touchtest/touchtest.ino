#define TOUCH_PAW_LEFT 12
#define TOUCH_PAW_RIGHT 13

#define TOUCH_BACK1 14
#define TOUCH_BACK2 27
#define TOUCH_BACK3 32

void setup()
{
    Serial.begin(115200);
}

#define PRINT_TOUCH(arg) \
    if (touchRead(TOUCH_##arg) < 20) \
        Serial.print(#arg"\t"); \
    else \
        Serial.print("[NONE]\t");

void loop()
{
    PRINT_TOUCH(PAW_LEFT)
    PRINT_TOUCH(PAW_RIGHT)
    PRINT_TOUCH(BACK1)
    PRINT_TOUCH(BACK2)
    PRINT_TOUCH(BACK3)
    Serial.println();
}
