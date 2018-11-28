#! /bin/bash

DBNAME='jjrpulser.db'
rm -f "$DBNAME"
rm -f "${DBNAME}.text"

sqlite3 "$DBNAME" "
BEGIN;
CREATE TABLE cold_water(value INTEGER, create_time INTEGER DEFAULT (strftime('%s', 'now')));
CREATE TABLE hot_water(value INTEGER, create_time INTEGER DEFAULT (strftime('%s', 'now')));
CREATE TABLE settings(cold_value INTEGER, hot_value INTEGER);
CREATE INDEX cold_water_create_time_idx ON cold_water (create_time DESC);
CREATE INDEX hot_water_create_time_idx ON hot_water (create_time DESC);
COMMIT;
"


START_COLD_VALUE=1494915
START_HOT_VALUE=725620
#START_DATE_TIME='2017-01-01 00:00:00'
START_DATE_TIME=$( date -d '2017-01-01 00:00:00' '+%s' )
DATE_TIME_INCREMENT=0

function FillWaterSession() {
    local SESSION_DURATION=$(( ( RANDOM % 2340 )  + 60 )) # 1-40 min
    local COLD_PERIOD=$(( ( RANDOM % 50 )  + 10 )) # 10 liters per 10-60 sec
    local COLD_COUNT=$(( SESSION_DURATION / COLD_PERIOD ))
    local HOT_PERIOD=$(( ( RANDOM % 50 )  + 10 )) # 10 liters per 10-60 sec
    local HOT_COUNT=$(( SESSION_DURATION / HOT_PERIOD ))

    local WATER_TYPE=$(( ( RANDOM % 3 ) )) # 0 - cold, 1 - hot, 2 - both

    if [ $WATER_TYPE -eq 0 -o $WATER_TYPE -eq 2 ]
    then
	for (( i = 0; i < COLD_COUNT; ++i ))
	do
#	    local SESSION_INCREMENT=$(( ( i * COLD_PERIOD ) + DATE_TIME_INCREMENT ))
	    local SESSION_INCREMENT=$(( ( i * COLD_PERIOD ) + DATE_TIME_INCREMENT + START_DATE_TIME ))
	    (( START_COLD_VALUE += 10 ))
#	    echo "INSERT INTO cold_water(value, create_time) VALUES($START_COLD_VALUE, DATETIME('$START_DATE_TIME', '+$SESSION_INCREMENT seconds'));" >> "${DBNAME}.text"
	    echo "INSERT INTO cold_water(value, create_time) VALUES($START_COLD_VALUE, $SESSION_INCREMENT);" >> "${DBNAME}.text"
	done
    fi

    if [ $WATER_TYPE -eq 1 -o $WATER_TYPE -eq 2 ]
    then
	for (( i = 0; i < HOT_COUNT; ++i ))
	do
#	    local SESSION_INCREMENT=$(( ( i * HOT_PERIOD ) + DATE_TIME_INCREMENT ))
	    local SESSION_INCREMENT=$(( ( i * HOT_PERIOD ) + DATE_TIME_INCREMENT + START_DATE_TIME ))
	    (( START_HOT_VALUE += 10 ))
#	    echo "INSERT INTO hot_water(value, create_time) VALUES($START_HOT_VALUE, DATETIME('$START_DATE_TIME', '+$SESSION_INCREMENT seconds'));" >> "${DBNAME}.text"
	    echo "INSERT INTO hot_water(value, create_time) VALUES($START_HOT_VALUE, $SESSION_INCREMENT);" >> "${DBNAME}.text"
	done
    fi
}

echo 'BEGIN;' >> "${DBNAME}.text"

while true
do
    JUMP_SECONDS=$(( ( RANDOM % 18000 ) + 3600 )) # 1-5 hours
    (( DATE_TIME_INCREMENT += JUMP_SECONDS ))
    [ $DATE_TIME_INCREMENT -gt 62208000 ] && break # About 2 years

    FillWaterSession
done

echo 'COMMIT;' >> "${DBNAME}.text"

cat "${DBNAME}.text" | sqlite3 "${DBNAME}"
