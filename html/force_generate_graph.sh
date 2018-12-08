#! /opt/bin/bash

CURRENT_DIR="$( dirname "${0}" )"
CURRENT_DIR="$( cd "${CURRENT_DIR}"; pwd )"

"${CURRENT_DIR}/mysql_generate_graph.sh" day
"${CURRENT_DIR}/mysql_generate_graph2.sh" day

"${CURRENT_DIR}/mysql_generate_graph.sh" week
"${CURRENT_DIR}/mysql_generate_graph2.sh" week

"${CURRENT_DIR}/mysql_generate_graph.sh" month
"${CURRENT_DIR}/mysql_generate_graph2.sh" month

"${CURRENT_DIR}/mysql_generate_graph.sh" year
"${CURRENT_DIR}/mysql_generate_graph2.sh" year
