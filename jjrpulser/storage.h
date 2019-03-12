#ifndef JJR_PULSER_STORAGE_H
#define JJR_PULSER_STORAGE_H

#define WATER_COLD_INCREMENT 10
#define WATER_HOT_INCREMENT 10

#define HTTP_CONN_LIST_MAX 32

// 15 minutes in milliseconds
#define STATISTICS_SEND_PERIOD (15 * 60 * 1000)

class Blinker;

namespace DataStorage {

void setup(const char *ssid, const char *passwd, Blinker *gb, Blinker *rb);
void work();
void incrementCounters(bool cold, bool hot);
void printStatistics();

}; // namespace DataStorage

#endif // JJR_PULSER_STORAGE_H
