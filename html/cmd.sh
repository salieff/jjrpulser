#!/opt/bin/bash

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
            ExecSQL "UPDATE settings SET cold_value = ${COLD}, hot_value = ${HOT}"
            STATUS_OK="$?"

            echo "Холодная вода: ${COLD}"
            echo "Горячая вода: ${HOT}"

            if [ "${STATUS_OK}" = '0' ]
            then
                echo 'Настройки записаны для отправки в устройство'
                echo 'Устройство заберет их при следующей передаче показаний'
            else
                echo 'Ошибка записи настроек в базу'
            fi
        fi
        ;;

    'add_value' )
        COLD="$( getParam 'cold' '-1' )"
        HOT="$( getParam 'hot' '-1' )"
        MAC="$( getParam 'mac' '' )"

        SETUP_SQL="BEGIN; SELECT concat_ws('|', cold_value, hot_value) FROM settings; UPDATE settings SET cold_value = -1, hot_value = -1; COMMIT;"
        SETUP_OUT="$( ExecSQL "${SETUP_SQL}" | tail -1 )"
        SETUP_COLD="$( echo "${SETUP_OUT}" | sed -e 's/|.*$//' )"
        SETUP_HOT="$( echo "${SETUP_OUT}" | sed -e 's/^.*|//' )"

        echo "SETUP_COLD ${SETUP_COLD}"
        echo "SETUP_HOT ${SETUP_HOT}"

        if [ "${SETUP_COLD}" != '-1' ]
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

        if [ "${SETUP_HOT}" != '-1' ]
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

        if [ "${COLD}" != '-1' ]
        then
            ExecSQL "INSERT INTO cold_water(value) VALUES(${COLD})"
            echo "Записано в базу, холодная вода: ${COLD}"
        fi

        if [ "${HOT}" != '-1' ]
        then
            ExecSQL "INSERT INTO hot_water(value) VALUES(${HOT})"
            echo "Записано в базу, горячая вода: ${HOT}"
        fi
        ;;

    * )
        echo "Неизвестная команда ${CMD}"
        ;;
esac
