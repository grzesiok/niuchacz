create table download_files
(
  start_time# timestamp,
  stop_time# timestamp,
  username# varchar2(128),
  url# varchar2(4000),
  content# blob
)
tablespace ts_loader_core_hist
pctfree 0
lob(content#) store as securefile lob$download_filecontent#
(
  tablespace ts_loader_files
  disable storage in row
  compress high
  nocache
);