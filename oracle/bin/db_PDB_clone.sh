#/binbash

TNS_NAME=$1
SYS_PASSWORD=$2
FROM_DB_NAME=$3
TO_DB_NAME=$4

CONNECTION_STRING="sys/$SYS_PASSWORD@$TNS_NAME as sysdba"

sqlplus -s $CONNECTION_STRING <<EOF
whenever sqlerror exit sql.sqlcode;

create pluggable database $TO_DB_NAME from $FROM_DB_NAME;
alter pluggable database $TO_DB_NAME open read write;
alter pluggable database $TO_DB_NAME save state;
exit;
EOF

exit $?
