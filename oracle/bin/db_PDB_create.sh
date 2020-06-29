#/binbash
set -e

TNS_NAME=$1
SYS_PASSWORD=$2
DB_NAME=$3
ADM_LOGINNAME=$4
ADM_PASSWORD=$5

CONNECTION_STRING="sys/$SYS_PASSWORD@$TNS_NAME as sysdba"
PATH_PREFIX=$(sqlplus -s $CONNECTION_STRING <<EOF
whenever sqlerror exit sql.sqlcode;
set head off

select value||'/'||'$TNS_NAME'||'/'||'$DB_NAME'||'/' into :path_prefix from v\$parameter where name = 'db_create_file_dest';
exit;
EOF
)
PATH_PREFIX=$(echo $PATH_PREFIX|tr -d '\n')
echo "PATH_PREFIX=$PATH_PREFIX"
mkdir -p $PATH_PREFIX
sqlplus -s $CONNECTION_STRING <<EOF
whenever sqlerror exit sql.sqlcode;

create pluggable database $DB_NAME admin user $ADM_LOGINNAME identified by $ADM_PASSWORD path_prefix='$PATH_PREFIX';
alter pluggable database $DB_NAME open read write;
alter pluggable database $DB_NAME save state;
alter session set container = $DB_NAME;
grant DBA to $ADM_LOGINNAME container = current;
create tablespace users;
alter user $ADM_LOGINNAME default tablespace users;
exit;
EOF

exit $?
