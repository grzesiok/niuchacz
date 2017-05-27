alter session set container = pdbdwh;

create user dwh_load identified by dwh_load default tablespace TS_LOADER_LOAD  account unlock;
grant connect, resource to dwh_load;
alter user dwh_load quota unlimited on TS_LOADER_LOAD;
grant create materialized view to dwh_load;
grant create synonym to dwh_load;

create user dwh_core identified by dwh_core default tablespace TS_LOADER_CORE  account unlock;
grant connect, resource to dwh_core;
alter user dwh_core quota unlimited on TS_LOADER_DATA;
alter user dwh_core quota unlimited on TS_LOADER_DATA_IDX;
alter user dwh_core quota unlimited on TS_LOADER_CORE;
alter user dwh_core quota unlimited on TS_LOADER_CORE_HIST;
alter user dwh_core quota unlimited on TS_LOADER_CORE_IDX;
grant create synonym to dwh_core;
grant select any table to dwh_core;
grant create view to dwh_core;
grant create job to dwh_core;

create user dwh_logging identified by dwh_logging default tablespace TS_LOADER_DATA account unlock;
grant connect, resource to dwh_logging;
alter user dwh_logging quota unlimited on TS_LOADER_DATA;
alter user dwh_logging quota unlimited on TS_LOADER_DATA_IDX;
grant create procedure to dwh_logging;

create user dwh_admin identified by dwh_admin default tablespace TS_LOADER_CORE account unlock;
grant connect, resource to dwh_admin;
alter user dwh_admin quota unlimited on TS_LOADER_CORE;
alter user dwh_admin quota unlimited on TS_LOADER_CORE_IDX;
alter user dwh_admin quota unlimited on TS_LOADER_CORE_HIST;
grant create procedure to dwh_admin;
grant select any dictionary to dwh_admin;
grant execute on dbms_lock to dwh_admin;
grant execute on dbms_crypto to dwh_admin;

create user dwh_test identified by dwh_test default tablespace TS_LOADER_DATA account unlock;
grant connect, resource to dwh_test;
grant create table to dwh_test;
alter user dwh_test quota unlimited on TS_LOADER_DATA;
grant create procedure to dwh_test;
grant select any dictionary to dwh_test;
grant create job to dwh_test;

create directory dwh_netdumps_dir as 'ext_tables/netdumps';
grant read, write on directory dwh_netdumps_dir to dwh_load;

create directory dwh_netdumpshist_dir as 'ext_tables/netdumps/history';
grant read, write on directory dwh_netdumpshist_dir to dwh_load;

create directory dwh_netstatdumps_dir as 'ext_tables/netstatdumps';
grant read, write on directory dwh_netstatdumps_dir to dwh_load;

create directory dwh_netstatdumpshist_dir as 'ext_tables/netstatdumps/history';
grant read, write on directory dwh_netstatdumpshist_dir to dwh_load;

grant read on directory DWH_NETDUMPS_DIR to dwh_admin;
exec dbms_java.grant_permission('DWH_ADMIN', 'SYS:java.io.FilePermission', '<<ALL FILES>>', 'read');

begin
  dbms_network_acl_admin.drop_acl(acl=> 'DWH_CORE_ACL.xml');
  dbms_network_acl_admin.create_acl(
    acl => 'DWH_CORE_ACL.xml',
    description => 'ACLs for DWH_CORE',
    principal => 'DWH_CORE',
    is_grant => true,
    privilege => 'resolve',
    start_date => null,
    end_date => null);
  DBMS_NETWORK_ACL_ADMIN.ASSIGN_ACL(acl  => 'DWH_CORE_ACL.xml', host => '*');
  commit;
end;
/
begin
  dbms_network_acl_admin.create_acl(
        acl         => 'DWH_ADMIN_ACL',
        description => 'ACL for dwh_loader',
        principal   => 'DWH_ADMIN',
        is_grant    =>  true,
        privilege   => 'connect');
  dbms_network_acl_admin.assign_acl(
    acl => 'DWH_ADMIN_ACL',
    host => 'api.nbp.pl',
    lower_port => 80);
end;
/

--privileges
grant select on dwh_load.netdumps to dwh_core;
grant execute on dwh_logging.pkg_logging to dwh_core;
grant execute on dwh_logging.pkg_logging to dwh_admin;
grant execute on dwh_logging.pkg_logging to dwh_load;
grant execute on dwh_admin.o_action to dwh_test;
grant UNDER ANY TYPE to dwh_test;
grant execute on dwh_admin.PKG_ACTIONS to dwh_test;