#ifndef JJR_PULSER_STORAGE_H
#define JJR_PULSER_STORAGE_H

#include <ESP8266WiFi.h>
#include <asyncHTTPrequest.h>

#include "blinker.h"

#define WATER_COLD_INCREMENT 10
#define WATER_HOT_INCREMENT 10

#define HTTP_CONN_LIST_MAX 32

namespace DataStorage {

    void setup(const char *ssid, const char *passwd, Blinker *gb, Blinker *rb);
    void work();
    void incrementCounters(bool cold, bool hot);
    uint32_t httpErrors();

}; // namespace DataStorage

#endif // JJR_PULSER_STORAGE_H
