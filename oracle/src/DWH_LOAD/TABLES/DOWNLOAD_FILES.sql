create table download_files
(
  start_time# timestamp,
  stop_time# timestamp,
  url# varchar2(4000),
  content# blob
)
tablespace ts_loader_files
pctfree 0
lob(content#) store as securefile lob$download_filecontent#
(
  tablespace ts_loader_files
  disable storage in row
  compress high
  nocache
);
