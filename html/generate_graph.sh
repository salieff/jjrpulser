#! /opt/bin/bash

DB_FILE_NAME="${1}"

case "${2}" in
    'week' )
        HOURS=$(( 24 * 7 ))
        FORMAT_X='%d %b\n%a'
        ;;

    'month' )
        HOURS=$(( 24 * 30 ))
        FORMAT_X='%d %b'
        ;;

    'year' )
        HOURS=$(( 24 * 30 * 12 ))
        FORMAT_X='%d %b\n%Y'
        ;;

    * )
        HOURS=24
        FORMAT_X='%d %b\n%H:%M'
        ;;
esac

PNG_FILE_NAME="${3}"

NOW=$( /opt/bin/date '+%s' )

function tzSeconds() {
    local tzStr="$( /opt/bin/date '+%::z' )"

    local sign="$( echo "${tzStr}" | sed -e 's/^\([\+\-]\)*.*/\1/' )"
    [ -z "${sign}" ] && sign='+'

    local hours="$( echo "${tzStr}" | sed -e 's/^[\+\-]0*\([[:digit:]]\+\):.*/\1/' )"
    local minutes="$( echo "${tzStr}" | sed -e 's/^[\+\-][[:digit:]]\+:0*\([[:digit:]]\+\):.*/\1/' )"
    local seconds="$( echo "${tzStr}" | sed -e 's/^[\+\-][[:digit:]]\+:[[:digit:]]\+:0*\([[:digit:]]\+\)$/\1/' )"

    echo ${sign} $(( hours * 3600 + minutes * 60 + seconds ))
}

# $1 - table name
# $2 - method, like MIN or MAX
# $3 - field name
# $4 - offset, like "+ 123"
function getBorderValue() {
    local table="$1"
    local method="$2"
    local field="$3"
    local offset="$4"

    local request="SELECT ${method}(${field}) ${offset} from ${table} WHERE create_time BETWEEN (${NOW} - ${HOURS}*3600) and ${NOW}"
    /opt/bin/sqlite3 "${DB_FILE_NAME}" "${request}"
}

TIME_OFFSET="$( tzSeconds )"

TABLE='cold_water'

MIN_COLD_VALUE="$( getBorderValue "${TABLE}" 'MIN' 'value' '' )"
COLD_OUT_FILE="$( /opt/bin/mktemp --tmpdir='/opt/tmp' pulser_data_XXXXXXXXXX.cold )"

SQL_REQUEST="SELECT create_time ${TIME_OFFSET}, value - ${MIN_COLD_VALUE} from ${TABLE} WHERE create_time BETWEEN (${NOW} - ${HOURS}*3600) and ${NOW} ORDER BY create_time"
/opt/bin/sqlite3 "${DB_FILE_NAME}" "${SQL_REQUEST}" > "${COLD_OUT_FILE}"

###################################################################

TABLE='hot_water'

MIN_HOT_VALUE="$( getBorderValue "${TABLE}" 'MIN' 'value' '' )"
HOT_OUT_FILE="$( /opt/bin/mktemp --tmpdir='/opt/tmp' pulser_data_XXXXXXXXXX.hot )"

SQL_REQUEST="SELECT create_time ${TIME_OFFSET}, value - ${MIN_HOT_VALUE} from ${TABLE} WHERE create_time BETWEEN (${NOW} - ${HOURS}*3600) and ${NOW} ORDER BY create_time"
/opt/bin/sqlite3 "${DB_FILE_NAME}" "${SQL_REQUEST}" > "${HOT_OUT_FILE}"

###################################################################

MIN_UTC_TIME=$(( NOW - HOURS * 3600 ))
MAX_UTC_TIME=$NOW

PLOT_FILE="$( /opt/bin/mktemp --tmpdir='/opt/tmp' pulser_data_XXXXXXXXXX.plot )"

cat << EOF > "${PLOT_FILE}"
set terminal png size 1024, 600
set output "${PNG_FILE_NAME}"
set datafile separator "|"
set timefmt '%s'
set xdata time
set format x "${FORMAT_X}"
set style data steps
set grid

set xrange [ ${MIN_UTC_TIME} ${TIME_OFFSET} : ${MAX_UTC_TIME} ${TIME_OFFSET} ]
# set yrange [ 0 : ${DELTA} ]

plot "${COLD_OUT_FILE}" using 1:2 notitle with impulses lc rgb "#ADD8E6", \
     "${HOT_OUT_FILE}" using 1:2 notitle with impulses lc rgb "#FF69B4", \
     "${COLD_OUT_FILE}" using 1:2 title "Cold water" lc rgb "blue" lw 2, \
     "${HOT_OUT_FILE}" using 1:2 title "Hot water" lc rgb "red" lw 2
EOF

/opt/bin/gnuplot "${PLOT_FILE}"

rm -f "${COLD_OUT_FILE}"
rm -f "${HOT_OUT_FILE}"
rm -f "${PLOT_FILE}"
