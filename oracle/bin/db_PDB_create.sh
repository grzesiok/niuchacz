#/binbash

TNS_NAME=$1
SYS_PASSWORD=$2
DB_NAME=$3
ADM_LOGINNAME=$4
ADM_PASSWORD=$5

CONNECTION_STRING="sys/$SYS_PASSWORD@$TNS_NAME as sysdba"

sqlplus -s $CONNECTION_STRING <<EOF
whenever sqlerror exit sql.sqlcode;

create pluggable database $DB_NAME admin user $ADM_LOGINNAME identified by $ADM_PASSWORD;
alter pluggable database $DB_NAME open read write;
alter pluggable database $DB_NAME save state;
alter session set container = $DB_NAME;
grant DBA to $ADM_LOGINNAME container = current;
create tablespace users;
alter user $ADM_LOGINNAME default tablespace users;
exit;
EOF

exit $?
