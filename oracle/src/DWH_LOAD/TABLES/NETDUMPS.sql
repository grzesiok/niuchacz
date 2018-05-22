CREATE TABLE NETDUMPS(
	ts timestamp,
	eth_shost varchar2(255),
	eth_dhost varchar2(255),
	eth_type number,
	ip_vhl number,
	ip_tos number,
	ip_len number,
	ip_id number,
	ip_off number,
	ip_ttl number,
	ip_p number,
	ip_sum number,
	ip_src varchar2(15),
	ip_dst varchar2(15)
)
PCTFREE 0
ROW STORE COMPRESS BASIC NOLOGGING
TABLESPACE TS_LOADER_LOAD
PARTITION BY RANGE (ts) INTERVAL (NUMTODSINTERVAL(1,'DAY'))
(PARTITION netdumps_part_1 values LESS THAN (TO_DATE('01-05-2017','DD-MM-YYYY')));

exec dbms_errlog.create_error_log(dml_table_name => 'NETDUMPS');
exec dbms_stats.set_table_prefs('DWH_LOAD','NETDUMPS','INCREMENTAL','TRUE');
exec dbms_stats.set_table_prefs('DWH_LOAD','NETDUMPS','INCREMENTAL_STALENESS','USE_STALE_PERCENT');
exec dbms_stats.set_table_prefs('DWH_LOAD','NETDUMPS','APPROXIMATE_NDV_ALGORITHM','HYPERLOGLOG');