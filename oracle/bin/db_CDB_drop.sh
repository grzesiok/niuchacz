#/binbash
set -e

CDB_NAME=$1
SYS_PASSWORD=$2

dbca -silent -deleteDatabase \
 -sourceDB $CDB_NAME \
 -sysDBAUserName sys \
 -sysDBAPassword $SYS_PASSWORD

exit $?
