#/binbash
set -e

TNS_NAME=$1
SYS_PASSWORD=$2
FROM_DB_NAME=$3
TO_DB_NAME=$4

CONNECTION_STRING="sys/$SYS_PASSWORD@$TNS_NAME as sysdba"

FROM_PATH_PREFIX=$(sqlplus -s $CONNECTION_STRING <<EOF
whenever sqlerror exit sql.sqlcode;
set head off

select value||'/'||'$TNS_NAME'||'/'||'$FROM_DB_NAME'||'/' into :path_prefix from v\$parameter where name = 'db_create_file_dest';
exit;
EOF
)
FROM_PATH_PREFIX=$(echo $FROM_PATH_PREFIX|tr -d '\n')
echo "FROM_PATH_PREFIX=$FROM_PATH_PREFIX"
TO_PATH_PREFIX=$(sqlplus -s $CONNECTION_STRING <<EOF
whenever sqlerror exit sql.sqlcode;
set head off

select value||'/'||'$TNS_NAME'||'/'||'$TO_DB_NAME'||'/' into :path_prefix from v\$parameter where name = 'db_create_file_dest';
exit;
EOF
)
TO_PATH_PREFIX=$(echo $TO_PATH_PREFIX|tr -d '\n')
echo "TO_PATH_PREFIX=$TO_PATH_PREFIX"
mkdir -p $TO_PATH_PREFIX
cp -R $FROM_PATH_PREFIX/* $TO_PATH_PREFIX
sqlplus -s $CONNECTION_STRING <<EOF
create pluggable database $TO_DB_NAME from $FROM_DB_NAME path_prefix='$TO_PATH_PREFIX';
alter pluggable database $TO_DB_NAME open read write;
alter pluggable database $TO_DB_NAME save state;
exit;
EOF

exit $?
