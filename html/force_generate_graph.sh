#! /opt/bin/bash

CURRENT_DIR="$( dirname "${0}" )"
CURRENT_DIR="$( cd "${CURRENT_DIR}"; pwd )"

"${CURRENT_DIR}/mysql_generate_graph.sh" day &>/dev/null &
"${CURRENT_DIR}/mysql_generate_graph2.sh" day &>/dev/null &

"${CURRENT_DIR}/mysql_generate_graph.sh" week &>/dev/null &
"${CURRENT_DIR}/mysql_generate_graph2.sh" week &>/dev/null &

"${CURRENT_DIR}/mysql_generate_graph.sh" month &>/dev/null &
"${CURRENT_DIR}/mysql_generate_graph2.sh" month &>/dev/null &

"${CURRENT_DIR}/mysql_generate_graph.sh" year &>/dev/null &
"${CURRENT_DIR}/mysql_generate_graph2.sh" year &>/dev/null &
