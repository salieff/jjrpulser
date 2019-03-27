#!/bin/bash

CURRENT_DIR="$( dirname "${0}" )"
CURRENT_DIR="$( cd "${CURRENT_DIR}"; pwd )"

COLD_VALUE="$( cat "${CURRENT_DIR}/stress-test.cold" )"
HOT_VALUE="$( cat "${CURRENT_DIR}/stress-test.hot" )"

HTTP_REQ_SENT=0
HTTP_REQ_COMMITED=0
HTTP_REQ_FAILED=0

function incrementCold() {
    (( COLD_VALUE += 10 ))
    echo "${COLD_VALUE}" > "${CURRENT_DIR}/stress-test.cold"
}

function incrementHot() {
    (( HOT_VALUE += 10 ))
    echo "${HOT_VALUE}" > "${CURRENT_DIR}/stress-test.hot"
}

function makeRequest() {
    date '+%F %T'

    (( HTTP_REQ_SENT++ ))
    exec 3>&1 
    HTTP_CODE="$( curl -sS -w '%{http_code}' -o >(cat >&3) "$1" 2>>"${CURRENT_DIR}/curl_debug.log" )"
    RET_ERR_CODE="$?"

    if [ "${RET_ERR_CODE}" != '0' ]
    then
        echo "[$( date '+%F %T' ) ${RET_ERR_CODE}] $1" >>"${CURRENT_DIR}/curl_debug.log"
        (( HTTP_REQ_FAILED++ ))
        return
    fi

    (( HTTP_REQ_COMMITED++ ))

    if [ "${HTTP_CODE}" != '200' ]
    then
        echo "[$( date '+%F %T' ) HTTP:${HTTP_CODE}] $1" >>"${CURRENT_DIR}/curl_debug.log"
        (( HTTP_REQ_FAILED++ ))
    fi
}

while true
do
    incrementCold
    makeRequest "http://nanopi-fire3/jjrpulser/cmd.sh?cmd=add_value&mac=e4:6f:13:f3:bc:a6&cold=${COLD_VALUE}"

    incrementHot
    makeRequest "http://nanopi-fire3/jjrpulser/cmd.sh?cmd=add_value&mac=e4:6f:13:f3:bc:a6&hot=${HOT_VALUE}"

    incrementCold
    incrementHot
    makeRequest "http://nanopi-fire3/jjrpulser/cmd.sh?cmd=add_value&mac=e4:6f:13:f3:bc:a6&cold=${COLD_VALUE}&hot=${HOT_VALUE}"

    UPTIME_DAYS="$( uptime | perl -pe 's/^.*up (\d+) days,.*/$1/' )"
    UPTIME_HOURS="$( uptime | perl -pe 's/^.*up \d+ days,\s+(\d+):\d+,.*/$1/' )"
    UPTIME_MINUTES="$( uptime | perl -pe 's/^.*up \d+ days,\s+\d+:(\d+),.*/$1/' )"
    MEM_FREE="$( cat /proc/meminfo | fgrep 'MemFree:' | perl -pe 's/^MemFree:\s+(\d+)\s+kB.*/$1/' )"

    REQUEST="http://nanopi-fire3/jjrpulser/cmd.sh?cmd=statistics&mac=e4:6f:13:f3:bc:a6";
    REQUEST="${REQUEST}&uptime_days=${UPTIME_DAYS}";
    REQUEST="${REQUEST}&uptime_hours=${UPTIME_HOURS}";
    REQUEST="${REQUEST}&uptime_minutes=${UPTIME_MINUTES}";
    REQUEST="${REQUEST}&uptime_seconds=0";
    REQUEST="${REQUEST}&uptime_millis=0";
    REQUEST="${REQUEST}&free_heap=${MEM_FREE}";
    REQUEST="${REQUEST}&http_req_sent=${HTTP_REQ_SENT}";
    REQUEST="${REQUEST}&http_req_commited=${HTTP_REQ_COMMITED}";
    REQUEST="${REQUEST}&http_req_failed=${HTTP_REQ_FAILED}";
    makeRequest "${REQUEST}"

    echo
    sleep 15
done
