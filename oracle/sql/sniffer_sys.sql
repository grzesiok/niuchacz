alter system set local_listener='(DESCRIPTION=(ADDRESS=(PROTOCOL=tcp)(HOST=localhost)(PORT=1521)))';
alter system register;

alter session set container = pdbniuchacz;

--Create tablespaces
create tablespace ts_logging_data
    datafile '&&PATH/ts_logging_data_1.dbf'
    size 10M
    autoextend on next 10M maxsize unlimited
    extent management local
    segment space management auto;
create tablespace ts_logging_idx
    datafile '&&PATH/ts_logging_idx_1.dbf'
    size 10M
    autoextend on next 10M maxsize unlimited
    extent management local
    segment space management auto;
create tablespace ts_loader_idx
    datafile '&&PATH/ts_loader_idx_1.dbf'
    size 1G
    autoextend on next 100M maxsize unlimited
    extent management local
    segment space management auto;
create tablespace ts_loader_data
    datafile '&&PATH/ts_loader_data_1.dbf'
    size 5G
    autoextend on next 100M maxsize unlimited
    extent management local
    segment space management auto;
create tablespace ts_loader_files
    datafile '&&PATH/ts_loader_files_1.dbf'
    size 3G
    autoextend on next 100M maxsize unlimited
    extent management local
    segment space management auto;

create user dwh_load identified by dwh_load default tablespace users account unlock;
grant connect, resource to dwh_load;
alter user dwh_load quota unlimited on ts_loader_files;
alter user dwh_load quota unlimited on ts_loader_data;
alter user dwh_load quota unlimited on ts_loader_idx;
alter user dwh_load quota 0M on users;
grant create procedure to dwh_load;

create user dwh_logging identified by dwh_logging default tablespace users account unlock;
grant connect, resource to dwh_logging;
alter user dwh_logging quota unlimited on ts_logging_data;
alter user dwh_logging quota unlimited on ts_logging_idx;
alter user dwh_logging quota 0M on users;
grant create procedure to dwh_logging;

create directory dwh_netdumps_dir as 'ext_tables/netdumps';
grant read, write on directory dwh_netdumps_dir to dwh_load;

create directory dwh_netdumpshist_dir as 'ext_tables/netdumps/history';
grant read, write on directory dwh_netdumpshist_dir to dwh_load;

CREATE OR REPLACE TRIGGER open_pdbs 
  AFTER STARTUP ON DATABASE 
BEGIN 
   EXECUTE IMMEDIATE 'ALTER PLUGGABLE DATABASE ALL OPEN'; 
END open_pdbs;
/

@$ORACLE_HOME/rdbms/admin/profload.sql

--privileges
