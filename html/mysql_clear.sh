#!/opt/bin/bash

CURRENT_DIR="$( dirname "${0}" )"
CURRENT_DIR="$( cd "${CURRENT_DIR}"; pwd )"

source "${CURRENT_DIR}/mysql_settings.sh"

/opt/bin/mysql -e "BEGIN;
                   DROP DATABASE ${MYSQL_DB_NAME};
                   CREATE DATABASE ${MYSQL_DB_NAME};
                   SHOW DATABASES;
                   USE ${MYSQL_DB_NAME};
                   CREATE TABLE cold_water(value INTEGER, create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP);
                   CREATE TABLE hot_water(value INTEGER, create_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP);
                   CREATE TABLE settings(cold_value INTEGER, hot_value INTEGER);
                   CREATE INDEX cold_water_create_time_idx ON cold_water (create_time);
                   CREATE INDEX hot_water_create_time_idx ON hot_water (create_time);
                   CREATE INDEX cold_water_full_idx ON cold_water (value, create_time);
                   CREATE INDEX hot_water_full_idx ON hot_water (value, create_time);
                   COMMIT;"