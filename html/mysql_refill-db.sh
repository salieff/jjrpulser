#! /opt/bin/bash

source "${CURRENT_DIR}/mysql_settings.sh"

SQL_OUT_FILE="${CURRENT_DIR}/jjrpulser.sql"
START_COLD_VALUE=1494915
START_HOT_VALUE=725620
START_DATE_TIME=$( date -d '2017-01-01 00:00:00' '+%s' )
STOP_DATE_TIME=$( date -d '2019-06-01 00:00:00' '+%s' )

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
            local insertTime=$(( $START_DATE_TIME + $(( i * COLD_PERIOD )) ))
            local strTime="$( date -d @${insertTime} '+%F %T' )"
            (( START_COLD_VALUE += 10 ))
            echo "INSERT INTO cold_water(value, create_time) VALUES($START_COLD_VALUE, '$strTime');" >> "${SQL_OUT_FILE}"
        done
    fi

    if [ $WATER_TYPE -eq 1 -o $WATER_TYPE -eq 2 ]
    then
        for (( i = 0; i < HOT_COUNT; ++i ))
        do
            local insertTime=$(( $START_DATE_TIME + $(( i * HOT_PERIOD )) ))
            local strTime="$( date -d @${insertTime} '+%F %T' )"
            (( START_HOT_VALUE += 10 ))
            echo "INSERT INTO hot_water(value, create_time) VALUES($START_HOT_VALUE, '$strTime');" >> "${SQL_OUT_FILE}"
        done
    fi
}

echo 'BEGIN;' > "${SQL_OUT_FILE}"

while true
do
    (( START_DATE_TIME += $(( ( RANDOM % 18000 ) + 3600 )) ))# 1-5 hours
    [ $START_DATE_TIME -gt $STOP_DATE_TIME ] && break
    FillWaterSession
done

echo "INSERT INTO settings(cold_value, hot_value) VALUES(-1, -1);" >> "${SQL_OUT_FILE}"
echo 'COMMIT;' >> "${SQL_OUT_FILE}"
