create table cfg_properties
(
  key# varchar2(30) primary key,
  value# varchar2(722)
) organization index
tablespace ts_loader_core_idx; 

insert into cfg_properties(key#, value#)
  values ('download_retries', '3');
insert into cfg_properties(key#, value#)
  values ('download_retry_wait', '1');
commit;
