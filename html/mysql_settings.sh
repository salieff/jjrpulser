CURRENT_DIR="$( dirname "${0}" )"
CURRENT_DIR="$( cd "${CURRENT_DIR}"; pwd )"

MYSQL_DB_NAME='jjrpulser'
MYSQL_USER=''
MYSQL_PASSWORD=''

function ExecSQL() {
    /opt/bin/mysql -sN "${MYSQL_DB_NAME}" -e "$1" 2>>"/opt/tmp/${MYSQL_DB_NAME}_debug.log"
    MYSQL_RET_ERR_CODE="$?"
    if [ "${MYSQL_RET_ERR_CODE}" != '0' ]
    then
        echo "[$( date '+%F %T' ) ${MYSQL_DB_NAME} ${MYSQL_RET_ERR_CODE}] $1" >>"/opt/tmp/${MYSQL_DB_NAME}_debug.log"
    fi
}

function WriteSetupCounters() {
    exec {lock_fd}>/tmp/jjr_sett.lock
    flock "${lock_fd}"

    echo "SETUP_COLD=$1" > "${CURRENT_DIR}/setup_counters.txt"
    echo "SETUP_HOT=$2" >> "${CURRENT_DIR}/setup_counters.txt"

    flock -u "${lock_fd}"
}

function ReadSetupCounters() {
    exec {lock_fd}>/tmp/jjr_sett.lock
    flock "${lock_fd}"

    if [ -f "${CURRENT_DIR}/setup_counters.txt" ]
    then
        source "${CURRENT_DIR}/setup_counters.txt"
        rm -f "${CURRENT_DIR}/setup_counters.txt"
    else
        SETUP_COLD=-1
        SETUP_HOT=-1
    fi

    flock -u "${lock_fd}"
}
