#! /opt/bin/bash

CURRENT_DIR="$( dirname "${0}" )"
CURRENT_DIR="$( cd "${CURRENT_DIR}"; pwd )"

source "${CURRENT_DIR}/mysql_settings.sh"

case "${1}" in
    'year' )
        HOURS=$(( 24 * 30 * 12 ))
        FORMAT_X='%b\n%Y'
        GROUPBY='YEAR(create_time), MONTH(create_time)'
        GROUPWIDTH=$(( 3600 * 24 * 30 ))
        XTICS='autofreq'
        SELTIME="CONCAT(YEAR(create_time), '-', MONTH(create_time), '-01 00:00:00')"
        ;;

    'month' )
        HOURS=$(( 24 * 30 ))
        FORMAT_X='%d\n%b'
        GROUPBY='YEAR(create_time), MONTH(create_time), DAY(create_time)'
        GROUPWIDTH=$(( 3600 * 24 ))
        SELTIME="CONCAT(DATE(create_time), ' 00:00:00')"
        XTICS=$(( 3600 * 24 ))
        ;;

    'week' )
        HOURS=$(( 24 * 7 ))
        FORMAT_X='%d %b\n%a'
        GROUPBY='YEAR(create_time), MONTH(create_time), DAY(create_time), HOUR(create_time)'
        SELTIME="CONCAT(DATE(create_time), ' ', HOUR(create_time), ':00:00')"
        GROUPWIDTH=3600
        XTICS='autofreq'
        ;;

    * ) # 'day'
        HOURS=24
        FORMAT_X='%H:%M\n%d.%m'
        GROUPBY='YEAR(create_time), MONTH(create_time), DAY(create_time), HOUR(create_time)'
        SELTIME="CONCAT(DATE(create_time), ' ', HOUR(create_time), ':00:00')"
        GROUPWIDTH=3600
        XTICS=$(( 3600 ))
        ;;
esac

PNG_FILE_NAME="${CURRENT_DIR}/mysql_jjrpulser2_${1}.png"

NOW=$( /opt/bin/date '+%F %T' )

TABLE='cold_water'
COLD_OUT_FILE="$( /opt/bin/mktemp --tmpdir='/opt/tmp' pulser_data_XXXXXXXXXX.cold )"

SQL_REQUEST="SELECT concat_ws('|', ${SELTIME}, MAX(value) - MIN(value)) from ${TABLE} WHERE create_time BETWEEN DATE_SUB('${NOW}', INTERVAL ${HOURS} HOUR) and '${NOW}' GROUP BY ${GROUPBY} ORDER BY create_time"
ExecSQL "${SQL_REQUEST}" > "${COLD_OUT_FILE}"

###################################################################

TABLE='hot_water'
HOT_OUT_FILE="$( /opt/bin/mktemp --tmpdir='/opt/tmp' pulser_data_XXXXXXXXXX.hot )"

SQL_REQUEST="SELECT concat_ws('|', ${SELTIME}, MAX(value) - MIN(value)) from ${TABLE} WHERE create_time BETWEEN DATE_SUB('${NOW}', INTERVAL ${HOURS} HOUR) and '${NOW}' GROUP BY ${GROUPBY} ORDER BY create_time"
ExecSQL "${SQL_REQUEST}" > "${HOT_OUT_FILE}"

###################################################################

SQL_REQUEST="SELECT DATE_SUB('${NOW}', INTERVAL ${HOURS} HOUR)"
MIN_TIME="$( ExecSQL "${SQL_REQUEST}" )"
MAX_TIME="${NOW}"

PLOT_FILE="$( /opt/bin/mktemp --tmpdir='/opt/tmp' pulser_data_XXXXXXXXXX.plot )"
TMP_PNG_FILE="$( /opt/bin/mktemp --tmpdir='/opt/tmp' pulser_data_XXXXXXXXXX.png )"

cat << EOF > "${PLOT_FILE}"
set terminal png truecolor size 1350, 600
set output "${TMP_PNG_FILE}"
set datafile separator "|"
set timefmt '%Y-%m-%d %H:%M:%S'
set xdata time
set format x "${FORMAT_X}"
set boxwidth $(( GROUPWIDTH / 2 ))
set style fill solid
set xtics ${XTICS}
set xtics out
unset mxtics
# set ticscale 10
set grid

set xrange [ "${MIN_TIME}" : "${MAX_TIME}" ]

plot "${COLD_OUT_FILE}" using (timecolumn(1)):2 title "Cold water" with boxes lc rgb "#770000FF", \
     "${HOT_OUT_FILE}" using (timecolumn(1) + $(( GROUPWIDTH / 8 ))):2 title "Hot water" with boxes lc rgb "#77FF0000"
EOF

/opt/bin/gnuplot "${PLOT_FILE}"
mv -f "${TMP_PNG_FILE}" "${PNG_FILE_NAME}"

rm -f "${COLD_OUT_FILE}"
rm -f "${HOT_OUT_FILE}"
rm -f "${PLOT_FILE}"
