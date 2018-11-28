#!/opt/bin/bash

DB_FILE='/opt/share/www/jjrpulser/jjrpulser.db'

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
            /opt/bin/sqlite3 "${DB_FILE}" "UPDATE settings set cold_value = ${COLD}, hot_value = ${HOT}"
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

        SETUP_COLD="$( /opt/bin/sqlite3 "${DB_FILE}" 'SELECT cold_value FROM settings' )"
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
            /opt/bin/sqlite3 "${DB_FILE}" "UPDATE settings SET cold_value = -1"
        fi

        SETUP_HOT="$( /opt/bin/sqlite3 "${DB_FILE}" 'SELECT hot_value FROM settings' )"
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
            /opt/bin/sqlite3 "${DB_FILE}" "UPDATE settings SET hot_value = -1"
        fi

        if [ "${COLD}" != '-1' ]
        then
            /opt/bin/sqlite3 "${DB_FILE}" "INSERT INTO cold_water(value) VALUES(${COLD})"
            echo "Записано в базу, холодная вода: ${COLD}"
        fi

        if [ "${HOT}" != '-1' ]
        then
            /opt/bin/sqlite3 "${DB_FILE}" "INSERT INTO hot_water(value) VALUES(${HOT})"
            echo "Записано в базу, горячая вода: ${HOT}"
        fi
        ;;

    * )
        echo "Неизвестная команда ${CMD}"
        ;;
esac
