create table core_actions_hist(
  log_time timestamp default systimestamp,
  action varchar2(4000),
  dbop_result clob
)
PCTFREE 0
ROW STORE COMPRESS ADVANCED NOLOGGING
TABLESPACE TS_LOADER_CORE_HIST
LOB(dbop_result) STORE AS SECUREFILE core_actions_lob1 (
  COMPRESS HIGH
  TABLESPACE TS_LOADER_CORE_HIST
)
PARTITION BY RANGE (log_time) INTERVAL (NUMTODSINTERVAL(1,'DAY'))
(PARTITION core_actions_hist_part_1 values LESS THAN (TO_DATE('01-05-2017','DD-MM-YYYY')));

exec dbms_stats.set_table_prefs('DWH_ADMIN','core_actions_hist','INCREMENTAL','TRUE');
exec dbms_stats.set_table_prefs('DWH_ADMIN','core_actions_hist','INCREMENTAL_STALENESS','USE_STALE_PERCENT');
exec dbms_stats.set_table_prefs('DWH_ADMIN','core_actions_hist','APPROXIMATE_NDV_ALGORITHM','HYPERLOGLOG');