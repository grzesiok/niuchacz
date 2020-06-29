#/binbash
set -e

CONNECTION_STRING="/ as sysdba"

sqlplus -s $CONNECTION_STRING <<EOF
whenever sqlerror exit sql.sqlcode;

alter system set db_files=2000 scope=spfile;
alter system set db_create_file_dest='/u01data' scope=spfile;
shutdown immediate;
startup;
alter system set local_listener='(DESCRIPTION=(ADDRESS=(PROTOCOL=tcp)(HOST=localhost)(PORT=1521)))';
alter system register;
exit;
EOF

exit $?
