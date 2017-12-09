--------------------------------------------------------
--  DDL for Type O_IMPORT_OSHA_INSPECTIONS
--------------------------------------------------------

  CREATE OR REPLACE EDITIONABLE TYPE "O_IMPORT_OSHA_INSPECTIONS" under O_IMPORT_OSHA
(
  overriding member function f_get_filterpath return varchar2,
  overriding member function f_is_active return boolean,
  overriding member procedure p_import_file(i_filename varchar2)
)
/
CREATE OR REPLACE EDITIONABLE TYPE BODY "O_IMPORT_OSHA_INSPECTIONS" AS
  
  overriding member function f_get_filterpath return varchar2 AS
  BEGIN
    return '^(osha_inspection-)([0-9]+)(\.csv)$';
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
    pkg_logging.log_file_start(i_job_name => 'OSHA_INSPECTIONS_IMPORT', i_file_name => i_filename);
    execute immediate 'alter table ext_osha_inspections location('''||i_filename||''')';
    insert /*+ append */into osha_inspections
    select * from ext_osha_inspections
    log errors into err$_osha_inspections(l_error_code) reject limit unlimited;
    l_imported_rows := sql%rowcount;
    commit;
    if(l_imported_rows = 0) then
      raise_application_error(-20000, 'Empty dataset? Please check ERR$_INSPECTIONS for filename = '||i_filename);
    end if;
    select count(*) into l_error_rows from err$_osha_inspections
    where ora_err_tag$ = l_error_code;
    pkg_logging.log_file_end(i_status => pkg_logging.C_SUCCESS,
                            i_error_count => l_error_rows,
                            i_record_count => l_imported_rows+l_error_rows,
                            i_insert_count => l_imported_rows);
  END p_import_file;

END;

/
