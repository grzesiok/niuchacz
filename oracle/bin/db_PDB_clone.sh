#/binbash
set -e

TNS_NAME=$1
SYS_PASSWORD=$2
FROM_DB_NAME=$3
TO_DB_NAME=$4

CONNECTION_STRING="sys/$SYS_PASSWORD@$TNS_NAME as sysdba"

PATH_PREFIX=$(sqlplus -s $CONNECTION_STRING <<EOF
whenever sqlerror exit sql.sqlcode;
set head off

select value||'/'||'$TNS_NAME'||'/'||'$TO_DB_NAME'||'/' into :path_prefix from v\$parameter where name = 'db_create_file_dest';
exit;
EOF
)
PATH_PREFIX=$(echo $PATH_PREFIX|tr -d '\n')
echo "PATH_PREFIX=$PATH_PREFIX"
mkdir -p $PATH_PREFIX
sqlplus -s $CONNECTION_STRING <<EOF
create pluggable database $TO_DB_NAME from $FROM_DB_NAME path_prefix='$PATH_PREFIX';
alter pluggable database $TO_DB_NAME open read write;
alter pluggable database $TO_DB_NAME save state;
exit;
EOF

exit $?
