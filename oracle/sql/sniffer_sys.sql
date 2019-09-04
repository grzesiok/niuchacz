--Create tablespaces
create tablespace ts_logging_data
    datafile size 10M
    autoextend on next 10M maxsize unlimited
    extent management local
    segment space management auto;
create tablespace ts_logging_idx
    datafile size 10M
    autoextend on next 10M maxsize unlimited
    extent management local
    segment space management auto;
create tablespace ts_loader_idx
    datafile size 1G
    autoextend on next 100M maxsize unlimited
    extent management local
    segment space management auto;
create tablespace ts_loader_data
    datafile size 5G
    autoextend on next 100M maxsize unlimited
    extent management local
    segment space management auto;
create tablespace ts_loader_files
    datafile size 3G
    autoextend on next 100M maxsize unlimited
    extent management local
    segment space management auto;

create user dwh_load identified by dwh_load default tablespace users account unlock;
grant connect, resource to dwh_load;
alter user dwh_load quota unlimited on ts_loader_files;
alter user dwh_load quota unlimited on ts_loader_data;
alter user dwh_load quota unlimited on ts_loader_idx;
alter user dwh_load quota unlimited on users;
grant create procedure to dwh_load;
begin
  for c in (select * from dba_directories where directory_name in ('DWH_NETDUMPS_DIR', 'DWH_NETDUMPSHIST_DIR'))
  loop
    dbms_java.grant_permission('DWH_LOAD', 'SYS:java.io.FilePermission', c.directory_path, 'read');
    dbms_java.grant_permission('DWH_LOAD', 'SYS:java.io.FilePermission', c.directory_path||'/*', 'read');
  end loop;
end;
/

create user dwh_logging identified by dwh_logging default tablespace users account unlock;
grant connect, resource to dwh_logging;
alter user dwh_logging quota unlimited on ts_logging_data;
alter user dwh_logging quota unlimited on ts_logging_idx;
alter user dwh_logging quota unlimited on users;
grant create procedure to dwh_logging;

create directory dwh_netdumps_dir as 'ext_tables/netdumps';
grant read, write on directory dwh_netdumps_dir to dwh_load;

create directory dwh_netdumpshist_dir as 'ext_tables/netdumps/history';
grant read, write on directory dwh_netdumpshist_dir to dwh_load;

--privileges
