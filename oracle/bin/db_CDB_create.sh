#/binbash
set -e

CDB_NAME=$1
SYS_PASSWORD=$2

dbca -silent -createDatabase \
 -templateName General_Purpose.dbc \
 -gdbname $CDB_NAME -sid $CDB_NAME -responseFile NO_VALUE \
 -characterSet AL32UTF8 \
 -sysPassword $SYS_PASSWORD \
 -systemPassword $SYS_PASSWORD \
 -createAsContainerDatabase true \
 -numberOfPDBs 0 \
 -databaseType MULTIPURPOSE \
 -automaticMemoryManagement false \
 -totalMemory 8192 \
 -storageType FS \
 -datafileDestination "/u01data/" \
 -redoLogFileSize 100 \
 -emConfiguration NONE \
 -ignorePreReqs

exit $?
