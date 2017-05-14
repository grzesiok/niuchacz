create or replace PACKAGE BODY PKG_IMPORT AS
  
  procedure p_import_netstatdump(i_filename varchar2) AS
    l_imported_rows number;
    l_error_rows number;
    l_error_code varchar2(255) := 'IMP'||to_char(systimestamp, 'yyyymmddhh24missff');
  BEGIN
    pkg_logging.log_file_start(i_job_name => 'NETSTATDUMPS_IMPORT', i_file_name => i_filename);
    execute immediate 'alter table ext_netstatdump location('''||i_filename||''')';
    insert /*+ append */into NETSTATDUMPS(FILENAME,SNAPSHOTTIME,PROTOCOL,LOCALIP,LOCALPORT,FOREIGNIP,FOREIGNPORT,APPPID,APPNAME)
    select i_filename,to_date(SNAPSHOTTIME, 'yyyy-mm-dd hh:mi:ss'),PROTOCOL,
           substr(LOCALADDR, 1, instr(LOCALADDR, ':')-1), substr(LOCALADDR, instr(LOCALADDR, ':')+1),
           substr(FOREIGNADDR, 1, instr(FOREIGNADDR, ':')-1), substr(FOREIGNADDR, instr(FOREIGNADDR, ':')+1),
           substr(APPDETAILS, 1, instr(APPDETAILS, '/')-1), substr(APPDETAILS, instr(APPDETAILS, '/')+1)
    from ext_netstatdump
    log errors into err$_netstatdumps(l_error_code) reject limit unlimited;
    l_imported_rows := sql%rowcount;
    commit;
    if(l_imported_rows = 0) then
      raise_application_error(-20000, 'Empty dataset? Please check ERR$_NETSTATDUMPS for filename = '||i_filename);
    end if;
    select count(*) into l_error_rows from err$_netstatdumps
    where ora_err_tag$ = l_error_code;
    pkg_logging.log_file_end(i_status => pkg_logging.C_SUCCESS,
                            i_error_count => l_error_rows,
                            i_record_count => l_imported_rows+l_error_rows,
                            i_insert_count => l_imported_rows);
  END p_import_netstatdump;
  
  procedure p_import_netdump(i_filename varchar2) AS
    l_imported_rows number;
    l_error_rows number;
    l_error_code varchar2(255) := 'IMP'||to_char(systimestamp, 'yyyymmddhh24missff');
  BEGIN
    pkg_logging.log_file_start(i_job_name => 'NETDUMPS_IMPORT', i_file_name => i_filename);
    execute immediate 'alter table ext_netdump location('''||i_filename||''')';
    insert /*+ append */into NETDUMPS(FILENAME,FRAMELEN,FRAMEPACKETFLAGS,FRAMEPROTOCOLS,FRAMETIME,
                                      ETHDST,ETHSRC,ETHTYPE,
                                      IPCHECKSUM,IPDST,IPFLAGSDF,IPFLAGSMF,IPFLAGSRB,IPFLAGSSF,IPLEN,IPPROTO,IPSRC,IPTOS,IPTTL,IPVERSION,
                                      ICMPCHECKSUM,ICMPCODE,ICMPEXT,ICMPEXTCHECKSUM,ICMPEXTCLASS,ICMPEXTCTYPE,ICMPEXTDATA,ICMPEXTLENGTH,
                                      ICMPEXTRES,ICMPEXTVERSION,ICMPLENGTH,ICMPLIFETIME,ICMPMTU,ICMPNORESP,ICMPNUMADDRS,ICMPPOINTER,
                                      ICMPPREFLEVEL,ICMPRECEIVETIMESTAMP,ICMPREDIRGW,ICMPRESERVED,ICMPRESPTIME,ICMPSEQ,ICMPSEQLE,
                                      ICMPTRANSMITTIMESTAMP,ICMPTYPE,
                                      TCPACK,TCPCHECKSUM,TCPDSTPORT,tcpseq,tcpnxtseq,
                                      TCPFLAGSACK,TCPFLAGSCWR,TCPFLAGSECN,TCPFLAGSFIN,TCPFLAGSNS,TCPFLAGSPUSH,TCPFLAGSRES,TCPFLAGSRESET,
                                      TCPFLAGSSYN,TCPFLAGSURG,TCPLEN,TCPOPTIONS,TCPSRCPORT,TCPWINDOWSIZE,
                                      UDPCHECKSUM,UDPDSTPORT,UDPLENGTH,UDPSRCPORT,
                                      DNSLENGTH,DNSOPENPGPKEY,DNSOPT,DNSQRYCLASS,DNSQRYNAME,DNSQRYNAMELEN,DNSQRYQU,DNSQRYTYPE)
    select i_filename,FRAMELEN,FRAMEPACKETFLAGS,FRAMEPROTOCOLS,to_timestamp(FRAMETIME, 'Mon DD, YYYY HH24:MI:SS.FF "CEST"', 'NLS_DATE_LANGUAGE=English'),
           ETHDST,ETHSRC,ETHTYPE,
           hextoraw(substr(IPCHECKSUM, 3)),IPDST,IPFLAGSDF,IPFLAGSMF,IPFLAGSRB,IPFLAGSSF,IPLEN,IPPROTO,IPSRC,IPTOS,IPTTL,IPVERSION,
           hextoraw(substr(ICMPCHECKSUM, 3)),ICMPCODE,ICMPEXT,hextoraw(substr(ICMPEXTCHECKSUM, 3)),ICMPEXTCLASS,ICMPEXTCTYPE,ICMPEXTDATA,ICMPEXTLENGTH,
           ICMPEXTRES,ICMPEXTVERSION,ICMPLENGTH,ICMPLIFETIME,ICMPMTU,ICMPNORESP,ICMPNUMADDRS,ICMPPOINTER,
           ICMPPREFLEVEL,ICMPRECEIVETIMESTAMP,ICMPREDIRGW,ICMPRESERVED,ICMPRESPTIME,ICMPSEQ,ICMPSEQLE,
           ICMPTRANSMITTIMESTAMP,ICMPTYPE,
           TCPACK,hextoraw(substr(TCPCHECKSUM, 3)),TCPDSTPORT,tcpseq,tcpnxtseq,
           TCPFLAGSACK,TCPFLAGSCWR,TCPFLAGSECN,TCPFLAGSFIN,TCPFLAGSNS,TCPFLAGSPUSH,TCPFLAGSRES,TCPFLAGSRESET,
           TCPFLAGSSYN,TCPFLAGSURG,TCPLEN,TCPOPTIONS,TCPSRCPORT,TCPWINDOWSIZE,
           hextoraw(substr(UDPCHECKSUM, 3)),UDPDSTPORT,UDPLENGTH,UDPSRCPORT,
           DNSLENGTH,DNSOPENPGPKEY,DNSOPT,DNSQRYCLASS,DNSQRYNAME,DNSQRYNAMELEN,DNSQRYQU,DNSQRYTYPE
    from ext_netdump
    log errors into err$_netdumps(l_error_code) reject limit unlimited;
    l_imported_rows := sql%rowcount;
    commit;
    if(l_imported_rows = 0) then
      raise_application_error(-20000, 'Empty dataset? Please check ERR$_NETDUMPS for filename = '||i_filename);
    end if;
    select count(*) into l_error_rows from err$_netdumps
    where ora_err_tag$ = l_error_code;
    pkg_logging.log_file_end(i_status => pkg_logging.C_SUCCESS,
                            i_error_count => l_error_rows,
                            i_record_count => l_imported_rows+l_error_rows,
                            i_insert_count => l_imported_rows);
  END p_import_netdump;

  procedure p_import AS
    type r_import is record(
      dirpath varchar2(255),
      histdirpath varchar2(255),
      filterpath varchar2(255));
    type t_imports is table of r_import index by varchar2(255);
    va_imports t_imports;
    l_currentimport varchar2(255);
  
    cursor c_filelist(i_dir varchar2, i_filter varchar2) is
      with
        tmp as (select filename, to_date(filename, i_filter) as filedate from table(dwh_admin.pkg_files.get_file_list(i_dir))
                where isreadable = 'Y'
                  and filename like '%.csv'
                  and validate_conversion(filename as date, i_filter) = 1)
      select filename from tmp
      order by filedate;
    cursor c_oldfilelist(i_dir varchar2, i_filter varchar2) is
      with
        tmp as (select filename, to_date(filename, i_filter) as filedate from table(dwh_admin.pkg_files.get_file_list(i_dir))
                where isreadable = 'Y'
                  and filename like '%.csv'
                  and validate_conversion(filename as date, i_filter) = 1)
      select filename from tmp
      where filedate < add_months(sysdate, -12);
    l_dir_archive varchar2(4000);
    l_dir_current varchar2(4000);
  begin
    pkg_logging.log_job_start(i_job_name => 'IMPORT');
    va_imports('p_import_netdump').dirpath := 'DWH_NETDUMPS_DIR';
    va_imports('p_import_netdump').histdirpath := 'DWH_NETDUMPSHIST_DIR';
    va_imports('p_import_netdump').filterpath := '"netdump"yyyymmddhh24miss".csv"';
    va_imports('p_import_netstatdump').dirpath := 'DWH_NETSTATDUMPS_DIR';
    va_imports('p_import_netstatdump').histdirpath := 'DWH_NETSTATDUMPSHIST_DIR';
    va_imports('p_import_netstatdump').filterpath := '"netstatdump"yyyymmddhh24miss".csv"';
    l_currentimport := va_imports.first;
    while(l_currentimport is not null)
    loop
      select directory_path into l_dir_current
      from all_directories where directory_name = va_imports(l_currentimport).dirpath;
      select directory_path into l_dir_archive
      from all_directories where directory_name = va_imports(l_currentimport).histdirpath;
      for c in c_filelist(l_dir_current, va_imports(l_currentimport).filterpath)
      loop
        if(l_currentimport = 'p_import_netdump') then
          p_import_netdump(c.filename);
        elsif(l_currentimport = 'p_import_netstatdump') then
          p_import_netstatdump(c.filename);
        end if;
        utl_file.frename(va_imports(l_currentimport).dirpath, c.filename, va_imports(l_currentimport).histdirpath, c.filename);
      end loop;
      for c in c_oldfilelist(l_dir_archive, va_imports(l_currentimport).filterpath)
      loop
        utl_file.fremove(va_imports(l_currentimport).histdirpath, c.filename);
      end loop;
      l_currentimport := va_imports.next(l_currentimport);
    end loop;
    pkg_logging.log_job_end(i_status => pkg_logging.C_SUCCESS);
  exception
    when others then
      pkg_logging.log_critical_error(i_message => sqlerrm);
      raise_application_error(-20000, DBMS_UTILITY.FORMAT_ERROR_BACKTRACE, true);
  end;

END PKG_IMPORT;
/