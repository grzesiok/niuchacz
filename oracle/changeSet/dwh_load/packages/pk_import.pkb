create or replace PACKAGE BODY PKG_IMPORT AS

  function f_get_list_of_imports(l_import_timestamp timestamp) return t_import is
    l_import_types t_string;
    l_import_objects t_import := t_import();
  begin
    select ut.type_name bulk collect into l_import_types
    from user_types ut,
         user_type_methods utm,
         (select ut2.type_name
          from user_types ut2
          start with ut2.type_name = 'O_IMPORT'
          connect by nocycle prior ut2.type_name = ut2.supertype_name) import_types
    where method_name = 'F_IS_ACTIVE'
      and ut.type_name = utm.type_name
      and ut.instantiable = 'YES'
      and ut.type_name = import_types.type_name
      and utm.overriding = 'YES';
    for i in 1..l_import_types.count
    loop
      dbms_output.put_line('Parsing object: '||i||'->'||l_import_types(i)||'...');
      l_import_objects.extend;
      execute immediate 'begin :1 := '||l_import_types(i)||'(:2); end;'
        using in out l_import_objects(l_import_objects.count), l_import_timestamp;
    end loop;
    return l_import_objects;
  end;

  procedure p_import AS
    l_all_imports t_import;
  
    cursor c_filelist(i_dir varchar2, i_filter varchar2) is
      with
        tmp as (select filename, lastmodified from table(pk_files.get_file_list(i_dir))
                where isreadable = 'Y'
                  and regexp_like(filename, i_filter, 'i')
                order by filename)
      select filename from tmp;
    l_dir_archive varchar2(4000);
    l_dir_current varchar2(4000);
  begin
    pk_logging.log_job_start(p_in_job_name => 'IMPORT');
    l_all_imports := f_get_list_of_imports(systimestamp);
    for i in 1..l_all_imports.count
    loop
      continue when (not l_all_imports(i).f_is_active);
      dbms_output.put_line('Executing object: '||i||'...');
      select directory_path into l_dir_current
      from all_directories where directory_name = l_all_imports(i).f_get_directory_name;
      select directory_path into l_dir_archive
      from all_directories where directory_name = l_all_imports(i).f_get_directory_histname;
      for c in c_filelist(l_dir_current, l_all_imports(i).f_get_filterpath)
      loop
        pk_logging.log_file_start(p_in_job_name => 'IMPORT',
                                  p_in_file_name => c.filename);
        l_all_imports(i).p_import_file(c.filename);
        pk_logging.log_file_end(p_in_status => pk_logging.c_logstatus_success);
        utl_file.frename(l_all_imports(i).f_get_directory_name, c.filename, l_all_imports(i).f_get_directory_histname, c.filename);
      end loop;
      for c in c_filelist(l_dir_archive, l_all_imports(i).f_get_filterpath)
      loop
        utl_file.fremove(l_all_imports(i).f_get_directory_histname, c.filename);
      end loop;
    end loop;
    pk_logging.log_job_end(p_in_status => pk_logging.c_logstatus_success);
  exception
    when others then
      pk_logging.log_error(p_in_errortype => pk_logging.c_logerrtype_critical,
                           p_in_message => sqlerrm);
      raise_application_error(-20000, DBMS_UTILITY.FORMAT_ERROR_BACKTRACE, true);
  end;

END PKG_IMPORT;
/
