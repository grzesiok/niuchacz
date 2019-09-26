CREATE OR REPLACE EDITIONABLE TYPE BODY "O_IMPORT_NETDUMP" AS

  overriding member function f_get_directory_name return varchar2 AS
  BEGIN
    return 'DWH_NETDUMPS_DIR';
  END f_get_directory_name;
  
  overriding member function f_get_directory_histname return varchar2 AS
  BEGIN
    return 'DWH_NETDUMPSHIST_DIR';
  END f_get_directory_histname;

  overriding member function f_get_filterpath return varchar2 AS
  BEGIN
    return '^(netdump-)([0-9]+)(\.csv)$';
  END f_get_filterpath;
  
  overriding member function f_is_active return boolean AS
  BEGIN
    return true;
  END f_is_active;

  overriding member procedure p_import_file(i_filename varchar2) AS
    l_imported_rows number;
    l_error_rows number;
    l_error_code varchar2(255) := 'IMP'||to_char(systimestamp, 'yyyymmddhh24missff');
  BEGIN
    execute immediate 'alter table ext_netdump location('''||i_filename||''')';
    insert /*+ append */into f_netdump(eth_timestamp, eth_src, eth_dst, ip_src, ip_dst, hostname_src, hostname_dst, ip_len, ip_sum)
    select to_timestamp('01/01/1970 00:00.'||to_char(en.ts_usec), 'MM/DD/YYYY HH24:MI.FF')+numtodsinterval(en.ts_sec, 'SECOND') as ts,
           en.eth_src, en.eth_dst, en.ip_src, en.ip_dst, en.hostname_src, en.hostname_dst, en.ip_len, en.ip_sum
    from ext_netdump en
    log errors into err$_f_netdump(l_error_code) reject limit unlimited;
    l_imported_rows := sql%rowcount;
    commit;
    if(l_imported_rows = 0) then
      raise_application_error(-20000, 'Empty dataset? Please check ERR$_NETDUMP for filename = '||i_filename);
    end if;
    select count(*) into l_error_rows from err$_f_netdump
    where ora_err_tag$ = l_error_code;
    if(l_error_rows > 0) then
      raise_application_error(-20000, 'Some records are wrong!');
    end if;
  END p_import_file;

END;

