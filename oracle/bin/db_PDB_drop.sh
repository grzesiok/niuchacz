#/binbash
set -e

TNS_NAME=$1
SYS_PASSWORD=$2
DB_NAME=$3

if [ -z "$DB_NAME" ]; then
  echo "You must provide proper DB_NAME"
  exit 1
fi
if [ -z "$TNS_NAME" ]; then
  echo "You must provide proper TNS_NAME"
  exit 1
fi
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
if [ ! -d "$PATH_PREFIX" ]; then
  echo "This directory not exists"
  exit 1
fi
rm -R $PATH_PREFIX
sqlplus -s $CONNECTION_STRING <<EOF
whenever sqlerror exit sql.sqlcode;

alter pluggable database $DB_NAME close immediate;
drop pluggable database $DB_NAME including datafiles;
exit;
EOF

exit $?
