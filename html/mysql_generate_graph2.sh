#! /opt/bin/bash

CURRENT_DIR="$( dirname "${0}" )"
CURRENT_DIR="$( cd "${CURRENT_DIR}"; pwd )"

source "${CURRENT_DIR}/mysql_settings.sh"

case "${1}" in
    'week' )
        HOURS=$(( 24 * 7 ))
        FORMAT_X='%d %b\n%a'
        DTFUNC='YEAR(create_time), MONTH(create_time), DAY(create_time), HOUR(create_time)'
        BOXWIDTH=2000
        XTICS='autofreq'
        ;;

    'month' )
        HOURS=$(( 24 * 30 ))
        FORMAT_X='%d\n%b'
        DTFUNC='YEAR(create_time), MONTH(create_time), DAY(create_time)'
        BOXWIDTH=$(( 1000 * 24 ))
        XTICS=$(( 60 * 60 * 24 ))
        ;;

    'year' )
        HOURS=$(( 24 * 30 * 12 ))
        FORMAT_X='%d %b\n%Y'
        DTFUNC='YEAR(create_time), MONTH(create_time)'
        BOXWIDTH=$(( 1000 * 24 * 30 ))
        XTICS='autofreq'
        ;;

    * )
        HOURS=24
        FORMAT_X='%d %b\n%H:%M'
        DTFUNC='YEAR(create_time), MONTH(create_time), DAY(create_time), HOUR(create_time)'
        BOXWIDTH=1000
        XTICS=$(( 60 * 60 * 2 ))
        ;;
esac

PNG_FILE_NAME="${CURRENT_DIR}/mysql_jjrpulser2_${1}.png"

NOW=$( /opt/bin/date '+%F %T' )

TABLE='cold_water'
COLD_OUT_FILE="$( /opt/bin/mktemp --tmpdir='/opt/tmp' pulser_data_XXXXXXXXXX.cold )"

SQL_REQUEST="SELECT concat_ws('|', create_time, MAX(value) - MIN(value)) from ${TABLE} WHERE create_time BETWEEN DATE_SUB('${NOW}', INTERVAL ${HOURS} HOUR) and '${NOW}' GROUP BY ${DTFUNC} ORDER BY create_time"
ExecSQL "${SQL_REQUEST}" > "${COLD_OUT_FILE}"

###################################################################

TABLE='hot_water'
HOT_OUT_FILE="$( /opt/bin/mktemp --tmpdir='/opt/tmp' pulser_data_XXXXXXXXXX.hot )"

SQL_REQUEST="SELECT concat_ws('|', create_time, MAX(value) - MIN(value)) from ${TABLE} WHERE create_time BETWEEN DATE_SUB('${NOW}', INTERVAL ${HOURS} HOUR) and '${NOW}' GROUP BY ${DTFUNC} ORDER BY create_time"
ExecSQL "${SQL_REQUEST}" > "${HOT_OUT_FILE}"

###################################################################

SQL_REQUEST="SELECT DATE_SUB('${NOW}', INTERVAL ${HOURS} HOUR)"
MIN_TIME="$( ExecSQL "${SQL_REQUEST}" )"
MAX_TIME="$NOW"

PLOT_FILE="$( /opt/bin/mktemp --tmpdir='/opt/tmp' pulser_data_XXXXXXXXXX.plot )"

cat << EOF > "${PLOT_FILE}"
set terminal png truecolor size 1024, 600
set output "${PNG_FILE_NAME}"
set datafile separator "|"
set timefmt '%Y-%m-%d %H:%M:%S'
set xdata time
set format x "${FORMAT_X}"
set boxwidth ${BOXWIDTH}
set style fill solid
set xtics ${XTICS}
set grid

set xrange [ "${MIN_TIME}" : "${MAX_TIME}" ]

plot "${COLD_OUT_FILE}" using 1:2 title "Cold water" with boxes lc rgb "#770000FF", \
     "${HOT_OUT_FILE}" using (timecolumn(1) + ${BOXWIDTH}/2):2 title "Hot water" with boxes lc rgb "#77FF0000"
EOF

/opt/bin/gnuplot "${PLOT_FILE}"

rm -f "${COLD_OUT_FILE}"
rm -f "${HOT_OUT_FILE}"
rm -f "${PLOT_FILE}"
