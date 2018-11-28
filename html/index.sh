#!/opt/bin/bash

DB_FILE='/opt/share/www/jjrpulser/jjrpulser.db'

echo "Content-type: text/html"
echo "Cache-Control: no-store, no-cache, must-revalidate"
echo "Cache-Control: post-check=0, pre-check=0"
echo "Pragma: no-cache"

echo ""

cat index_header.html

COLD_VALUE="$( /opt/bin/sqlite3 "${DB_FILE}" 'SELECT MAX(value) from cold_water' )"
COLD_VALUE_HI=$(( COLD_VALUE / 1000 ))
COLD_VALUE_LO=$(( COLD_VALUE - COLD_VALUE_HI * 1000 ))
COLD_VALUE_FORMATTED="$( printf "%05d,%03d" $COLD_VALUE_HI $COLD_VALUE_LO )"

HOT_VALUE="$( /opt/bin/sqlite3 "${DB_FILE}" 'SELECT MAX(value) from hot_water' )"
HOT_VALUE_HI=$(( HOT_VALUE / 1000 ))
HOT_VALUE_LO=$(( HOT_VALUE - HOT_VALUE_HI * 1000 ))
HOT_VALUE_FORMATTED="$( printf "%05d,%03d" $HOT_VALUE_HI $HOT_VALUE_LO )"

COLD_TIME="$( /opt/bin/sqlite3 "${DB_FILE}" "SELECT datetime(MAX(create_time), 'unixepoch', 'localtime') from cold_water" )"
HOT_TIME="$( /opt/bin/sqlite3 "${DB_FILE}" "SELECT datetime(MAX(create_time), 'unixepoch', 'localtime') from hot_water" )"

TODAY_TM="$( /opt/bin/date '+%F %T' )"

cat << EOF
<p><h2>Сегодня: ${TODAY_TM}</h2></p>

<table>
 <tr>
 <td>
  <div class="water-counter">
    <img src="water_cold.png">
    <span>${COLD_VALUE_FORMATTED}</span>
  </div>
 </td>

 <td>
  <div class="water-counter">
    <img src="water_hot.png">
    <span>${HOT_VALUE_FORMATTED}</span>
  </div>
 </td>
 </tr>

 <tr>
 <td align="center">
  <h2><font color="#2C80CF">${COLD_TIME}</font></h2>
 </td>

 <td align="center">
  <h2><font color="red">${HOT_TIME}</font></h2>
 </td>
 </tr>
</table>
<br>
EOF

cat index_input.html

SETUP_COLD="$( /opt/bin/sqlite3 "${DB_FILE}" 'SELECT cold_value FROM settings' )"
SETUP_HOT="$( /opt/bin/sqlite3 "${DB_FILE}" 'SELECT hot_value FROM settings' )"

if [ "${SETUP_COLD}" != '-1' -o "${SETUP_HOT}" != '-1' ]
then
    echo '
    <fieldset class="fieldset-auto-width">
        <legend>Настройки, ожидающие отправки в устройство</legend>
            <table>'

    if [ "${SETUP_COLD}" != '-1' -o "${SETUP_HOT}" != '-1' ]
    then
        echo "
        <tr>
            <td>Холодная вода:</td>
            <td>${SETUP_COLD}</td>
        </tr>"
    fi

    if [ "${SETUP_COLD}" != '-1' -o "${SETUP_HOT}" != '-1' ]
    then
        echo "
        <tr>
            <td>Горячая вода:</td>
            <td>${SETUP_HOT}</td>
        </tr>"
    fi

    echo '
        </table>
    </fieldset>'
fi

cat index_footer.html
