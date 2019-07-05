#/binbash

TNS_NAME=$1
SYS_PASSWORD=$2
DB_NAME=$3

CONNECTION_STRING="sys/$SYS_PASSWORD@$TNS_NAME as sysdba"

sqlplus -s $CONNECTION_STRING <<EOF
whenever sqlerror exit sql.sqlcode;

alter pluggable database $DB_NAME close immediate;
drop pluggable database $DB_NAME including datafiles;
exit;
EOF

exit $?
