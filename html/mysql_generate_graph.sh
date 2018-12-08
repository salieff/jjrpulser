#! /opt/bin/bash

CURRENT_DIR="$( dirname "${0}" )"
CURRENT_DIR="$( cd "${CURRENT_DIR}"; pwd )"

source "${CURRENT_DIR}/mysql_settings.sh"

case "${1}" in
    'year' )
        HOURS=$(( 24 * 30 * 12 ))
        FORMAT_X='%b\n%Y'
        XTICS='autofreq'
        ;;

    'month' )
        HOURS=$(( 24 * 30 ))
        FORMAT_X='%d\n%b'
        XTICS=$(( 3600 * 24 ))
        ;;

    'week' )
        HOURS=$(( 24 * 7 ))
        FORMAT_X='%d %b\n%a'
        XTICS='autofreq'
        ;;

    * ) # 'day'
        HOURS=24
        FORMAT_X='%H:%M\n%d.%m'
        XTICS=$(( 3600 ))
        ;;
esac

PNG_FILE_NAME="${CURRENT_DIR}/mysql_jjrpulser_${1}.png"

NOW=$( /opt/bin/date '+%F %T' )

# $1 - table name
# $2 - method, like MIN or MAX
# $3 - field name
function getBorderValue() {
    local table="$1"
    local method="$2"
    local field="$3"
    local offset="$4"

    local request="SELECT ${method}(${field}) from ${table} WHERE create_time BETWEEN DATE_SUB('${NOW}', INTERVAL ${HOURS} HOUR) and '${NOW}';"
    ExecSQL "${request}"
}

TABLE='cold_water'

MIN_COLD_VALUE="$( getBorderValue "${TABLE}" 'MIN' 'value' )"
COLD_OUT_FILE="$( /opt/bin/mktemp --tmpdir='/opt/tmp' pulser_data_XXXXXXXXXX.cold )"

SQL_REQUEST="SELECT concat_ws('|', create_time, value - ${MIN_COLD_VALUE}) from ${TABLE} WHERE create_time BETWEEN DATE_SUB('${NOW}', INTERVAL ${HOURS} HOUR) and '${NOW}' ORDER BY create_time"
ExecSQL "${SQL_REQUEST}" > "${COLD_OUT_FILE}"

###################################################################

TABLE='hot_water'

MIN_HOT_VALUE="$( getBorderValue "${TABLE}" 'MIN' 'value' )"
HOT_OUT_FILE="$( /opt/bin/mktemp --tmpdir='/opt/tmp' pulser_data_XXXXXXXXXX.hot )"

SQL_REQUEST="SELECT concat_ws('|', create_time, value - ${MIN_HOT_VALUE}) from ${TABLE} WHERE create_time BETWEEN DATE_SUB('${NOW}', INTERVAL ${HOURS} HOUR) and '${NOW}' ORDER BY create_time"
ExecSQL "${SQL_REQUEST}" > "${HOT_OUT_FILE}"

###################################################################

SQL_REQUEST="SELECT DATE_SUB('${NOW}', INTERVAL ${HOURS} HOUR)"
MIN_TIME="$( ExecSQL "${SQL_REQUEST}" )"
MAX_TIME="$NOW"

PLOT_FILE="$( /opt/bin/mktemp --tmpdir='/opt/tmp' pulser_data_XXXXXXXXXX.plot )"

cat << EOF > "${PLOT_FILE}"
set terminal png truecolor size 1350, 600
set output "${PNG_FILE_NAME}"
set datafile separator "|"
set timefmt '%Y-%m-%d %H:%M:%S'
set xdata time
set format x "${FORMAT_X}"
set style data steps
set xtics ${XTICS}
set xtics out
unset mxtics
# set ticscale 10
set grid

set xrange [ "${MIN_TIME}" : "${MAX_TIME}" ]
# set yrange [ 0 : ${DELTA} ]

plot "${COLD_OUT_FILE}" using 1:2 notitle with impulses lc rgb "#ADD8E6", \
     "${HOT_OUT_FILE}" using 1:2 notitle with impulses lc rgb "#CCFF69B4", \
     "${COLD_OUT_FILE}" using 1:2 title "Cold water" lc rgb "blue" lw 2, \
     "${HOT_OUT_FILE}" using 1:2 title "Hot water" lc rgb "red" lw 2
EOF

/opt/bin/gnuplot "${PLOT_FILE}"

rm -f "${COLD_OUT_FILE}"
rm -f "${HOT_OUT_FILE}"
rm -f "${PLOT_FILE}"
