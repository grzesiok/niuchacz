#/binbash
set -e

TNS_NAME=$1
ADM_LOGINGNAME=$2
ADM_PASSWORD=$3

CONNECTION_STRING="$ADM_LOGINGNAME/$ADM_PASSWORD@$TNS_NAME"

sqlplus -s $CONNECTION_STRING <<EOF
whenever sqlerror exit sql.sqlcode;

@../sql/sniffer_sys.sql
exit;
EOF

exit $?
