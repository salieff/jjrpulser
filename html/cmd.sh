#!/bin/bash

CURRENT_DIR="$( dirname "${0}" )"
CURRENT_DIR="$( cd "${CURRENT_DIR}"; pwd )"

source "${CURRENT_DIR}/mysql_settings.sh"

OLD_IFS=$IFS
IFS='=&'
PARAMS_ARR=( $QUERY_STRING )
IFS=$OLD_IFS

function getParam() {
    for (( i = 0; i < ${#PARAMS_ARR[@]}; ++i ))
    do
        if [ "${PARAMS_ARR[$i]}" = "$1" ]
        then
            echo -n "${PARAMS_ARR[$(( i + 1 ))]}"
            return
        fi
    done

    echo -n "$2"
}

echo "Content-type: text/plain; charset=utf-8"
echo "Cache-Control: no-store, no-cache, must-revalidate"
echo "Cache-Control: post-check=0, pre-check=0"
echo "Pragma: no-cache"
echo "Refresh: 5; url=index.sh"
echo ""

CMD="$( getParam 'cmd' '' )"

case "${CMD}" in
    'setup' )
        COLD="$( getParam 'cold' '-1' )"
        HOT="$( getParam 'hot' '-1' )"
        if [ "${COLD}" != '-1' -o "${HOT}" != '-1' ]
        then
            WriteSetupCounters "${COLD}" "${HOT}"

            echo "Холодная вода: ${COLD}"
            echo "Горячая вода: ${HOT}"

            echo 'Настройки записаны для отправки в устройство'
            echo 'Устройство заберет их при следующей передаче показаний'
        fi
        ;;

    'add_value' )
        MAC="$( getParam 'mac' '' )"
        COLD="$( getParam 'cold' '-1' )"
        HOT="$( getParam 'hot' '-1' )"

        ReadSetupCounters

        if [ -n "${SETUP_COLD}" -a "${SETUP_COLD}" != '-1' ]
        then
            SETUP_COLD=$(( SETUP_COLD / 10 ))
            SETUP_COLD=$(( SETUP_COLD * 10 ))

            if [ "${COLD}" != '-1' ]
            then
                (( SETUP_COLD += 10 ))
                COLD=${SETUP_COLD}
            fi

            echo "setup_new_cold = ${SETUP_COLD}"
        fi

        if [ -n "${SETUP_HOT}" -a "${SETUP_HOT}" != '-1' ]
        then
            SETUP_HOT=$(( SETUP_HOT / 10 ))
            SETUP_HOT=$(( SETUP_HOT * 10 ))

            if [ "${HOT}" != '-1' ]
            then
                (( SETUP_HOT += 10 ))
                HOT=${SETUP_HOT}
            fi

            echo "setup_new_hot = ${SETUP_HOT}"
        fi

        if [ -n "${COLD}" -a "${COLD}" != '-1' ]
        then
            ExecSQL "INSERT INTO cold_water(value) VALUES(${COLD})"
            echo "Записано в базу, холодная вода: ${COLD}"
        fi

        if [ -n "${HOT}" -a "${HOT}" != '-1' ]
        then
            ExecSQL "INSERT INTO hot_water(value) VALUES(${HOT})"
            echo "Записано в базу, горячая вода: ${HOT}"
        fi
        ;;

    'statistics' )
        MAC="$( getParam 'mac' '' )"
        UPTIMEDAYS="$( getParam 'uptime_days' '-1' )"
        UPTIMEHOURS="$( getParam 'uptime_hours' '-1' )"
        UPTIMEMINUTES="$( getParam 'uptime_minutes' '-1' )"
        UPTIMESECONDS="$( getParam 'uptime_seconds' '-1' )"
        UPTIMEMILLIS="$( getParam 'uptime_millis' '-1' )"
        FREEHEAP="$( getParam 'free_heap' '-1' )"
        HTTPREQSENT="$( getParam 'http_req_sent' '-1' )"
        HTTPREQCOMMITED="$( getParam 'http_req_commited' '-1' )"
        HTTPREQFAILED="$( getParam 'http_req_failed' '-1' )"

        WriteStatistics "${UPTIMEDAYS}" \
                        "${UPTIMEHOURS}" \
                        "${UPTIMEMINUTES}" \
                        "${UPTIMESECONDS}" \
                        "${UPTIMEMILLIS}" \
                        "${FREEHEAP}" \
                        "${HTTPREQSENT}" \
                        "${HTTPREQCOMMITED}" \
                        "${HTTPREQFAILED}"
        ;;

    * )
        echo "Неизвестная команда ${CMD}"
        ;;
esac
