--------------------------------------------------------
--  DDL for Package Body PKG_IMPORT
--------------------------------------------------------

  CREATE OR REPLACE EDITIONABLE PACKAGE BODY "DWH_LOAD"."PKG_IMPORT" AS

  c_job_name constant varchar2(255) := 'NETDUMPS_IMPORT';
  
  procedure p_import(i_filename varchar2) AS
    l_imported_rows number;
    l_error_rows number;
    l_error_code varchar2(255) := 'IMP'||to_char(systimestamp, 'yyyymmddhh24missff');
  BEGIN
    pkg_logging.log_file_start(i_job_name => c_job_name, i_file_name => i_filename);
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
  END p_import;

  procedure p_import AS
    cursor c_filelist(i_dir varchar2) is
      with
        tmp as (select filename, to_date(filename, '"netdump"yyyymmddhh24miss".csv"') as filedate from table(dwh_admin.pkg_files.get_file_list(i_dir))
                where isreadable = 'Y'
                  and filename like '%.csv'
                  and validate_conversion(filename as date, '"netdump"yyyymmddhh24miss".csv"') = 1)
      select filename from tmp
      order by filedate;
    cursor c_oldfilelist(i_dir varchar2) is
      with
        tmp as (select filename, to_date(filename, '"netdump"yyyymmddhh24miss".csv"') as filedate from table(dwh_admin.pkg_files.get_file_list(i_dir))
                where isreadable = 'Y'
                  and filename like '%.csv'
                  and validate_conversion(filename as date, '"netdump"yyyymmddhh24miss".csv"') = 1)
      select filename from tmp
      where filedate < add_months(sysdate, -12);
    l_dir_archive varchar2(4000);
    l_dir_current varchar2(4000);
  begin
    pkg_logging.log_job_start(i_job_name => c_job_name);
    select directory_path into l_dir_current
    from all_directories where directory_name = 'DWH_NETDUMPS_DIR';
    select directory_path into l_dir_archive
    from all_directories where directory_name = 'DWH_NETDUMPSHIST_DIR';
    for c in c_filelist(l_dir_current)
    loop
      p_import(c.filename);
      utl_file.frename('DWH_NETDUMPS_DIR', c.filename, 'DWH_NETDUMPSHIST_DIR', c.filename);
    end loop;
    for c in c_oldfilelist(l_dir_archive)
    loop
      utl_file.fremove('DWH_NETDUMPSHIST_DIR', c.filename);
    end loop;
    pkg_logging.log_job_end(i_status => pkg_logging.C_SUCCESS);
  exception
    when others then
      pkg_logging.log_critical_error(i_message => sqlerrm);
      raise_application_error(-20000, DBMS_UTILITY.FORMAT_ERROR_BACKTRACE, true);
  end;

END PKG_IMPORT;

/
