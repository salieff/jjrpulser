CURRENT_DIR="$( dirname "${0}" )"
CURRENT_DIR="$( cd "${CURRENT_DIR}"; pwd )"

MYSQL_DB_NAME='jjrpulser'
MYSQL_USER='www-data'
MYSQL_PASSWORD=''

function ExecSQL() {
    /usr/bin/mysql -u "${MYSQL_USER}" -sN "${MYSQL_DB_NAME}" -e "$1" 2>>"/tmp/${MYSQL_DB_NAME}_debug.log"
    MYSQL_RET_ERR_CODE="$?"
    if [ "${MYSQL_RET_ERR_CODE}" != '0' ]
    then
        echo "[$( date '+%F %T' ) ${MYSQL_DB_NAME} ${MYSQL_RET_ERR_CODE}] $1" >>"/tmp/${MYSQL_DB_NAME}_debug.log"
    fi
}

function WriteSetupCounters() {
    exec {lock_fd}>"${CURRENT_DIR}/jjr_sett.lock"
    flock "${lock_fd}"

    echo "SETUP_COLD=$1" > "${CURRENT_DIR}/setup_counters.txt"
    echo "SETUP_HOT=$2" >> "${CURRENT_DIR}/setup_counters.txt"

    flock -u "${lock_fd}"
}

function ReadSetupCounters() {
    exec {lock_fd}>"${CURRENT_DIR}/jjr_sett.lock"
    [ "$1" = 'softmode' ] && flock -s "${lock_fd}" || flock "${lock_fd}"

    if [ -f "${CURRENT_DIR}/setup_counters.txt" ]
    then
        source "${CURRENT_DIR}/setup_counters.txt"
        [ "$1" = 'softmode' ] || rm -f "${CURRENT_DIR}/setup_counters.txt"
    else
        SETUP_COLD=-1
        SETUP_HOT=-1
    fi

    flock -u "${lock_fd}"
}

function WriteStatistics() {
    exec {lock_fd}>"${CURRENT_DIR}/jjr_stat.lock"
    flock "${lock_fd}"

    echo "UPTIMEDAYS=$1" > "${CURRENT_DIR}/statistics.txt"
    echo "UPTIMEHOURS=$2" >> "${CURRENT_DIR}/statistics.txt"
    echo "UPTIMEMINUTES=$3" >> "${CURRENT_DIR}/statistics.txt"
    echo "UPTIMESECONDS=$4" >> "${CURRENT_DIR}/statistics.txt"
    echo "UPTIMEMILLIS=$5" >> "${CURRENT_DIR}/statistics.txt"
    echo "FREEHEAP=$6" >> "${CURRENT_DIR}/statistics.txt"
    echo "HTTPREQSENT=$7" >> "${CURRENT_DIR}/statistics.txt"
    echo "HTTPREQCOMMITED=$8" >> "${CURRENT_DIR}/statistics.txt"
    echo "HTTPREQFAILED=$9" >> "${CURRENT_DIR}/statistics.txt"
    echo "STATDATETIME=\"$( /bin/date '+%F %T' )\"" >> "${CURRENT_DIR}/statistics.txt"

    flock -u "${lock_fd}"
}

function ReadStatistics() {
    exec {lock_fd}>"${CURRENT_DIR}/jjr_stat.lock"
    flock -s "${lock_fd}"

    if [ -f "${CURRENT_DIR}/statistics.txt" ]
    then
        source "${CURRENT_DIR}/statistics.txt"
    else
        UPTIMEDAYS=-1
        UPTIMEHOURS=-1
        UPTIMEMINUTES=-1
        UPTIMESECONDS=-1
        UPTIMEMILLIS=-1
        FREEHEAP=-1
        HTTPREQSENT=-1
        HTTPREQCOMMITED=-1
        HTTPREQFAILED=-1
    fi

    flock -u "${lock_fd}"
}
