create table NETSTATDUMPS(
  FILENAME varchar2(255),
  SNAPSHOTTIME date,
  protocol varchar2(10),
  localip VARCHAR2(17),
  localport number,
  foreignip VARCHAR2(17),
  foreignport number,
  apppid number,
  appname varchar2(255)
)
pctfree 0
ROW STORE COMPRESS BASIC NOLOGGING
TABLESPACE TS_LOADER_LOAD;

exec dbms_errlog.create_error_log(dml_table_name => 'NETSTATDUMPS');