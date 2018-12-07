MYSQL_DB_NAME='jjrpulser'
MYSQL_USER=''
MYSQL_PASSWORD=''

function ExecSQL() {
    /opt/bin/mysql -sN "${MYSQL_DB_NAME}" -e "$1" 2>>"/opt/tmp/${MYSQL_DB_NAME}_debug.log"
}
