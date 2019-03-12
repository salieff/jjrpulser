#ifndef JJR_PULSER_TEST_FUNCTIONS_H
#define JJR_PULSER_TEST_FUNCTIONS_H

#include "blinker.h"

// 15 seconds
#define TEST_SEND_PERIOD (15 * 1000)

class Blinker;

void processConsoleInput(Blinker *greenBlinker, Blinker *redBlinker);
void testSend();

#endif // JJR_PULSER_TEST_FUNCTIONS_H
