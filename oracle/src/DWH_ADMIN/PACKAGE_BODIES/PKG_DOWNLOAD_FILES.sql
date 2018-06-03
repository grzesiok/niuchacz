create or replace PACKAGE BODY PKG_DOWNLOAD_FILES AS

  procedure p_job_handler as
    l_curr_date date;
    l_sysdate date := sysdate;
    l_action o_action;
  begin
    for c in (select * from download_files_def)
    loop
      l_curr_date := c.last_download;
      while(l_curr_date < l_sysdate)
      loop
        l_curr_date := l_curr_date + c.next_download_offset;
        l_action := o_download_files_action(to_char(l_curr_date, c.url), null);
        pkg_actions.p_enqueue(i_recipient => c.user_name, i_autocommit => false, i_action => l_action);
        --dbms_output.put_line('o_download_action(to_char('||l_curr_date||', '||c.url||'), null);');
      end loop;
      update download_files_def
        set last_download = l_sysdate
      where download_file_def_id = c.download_file_def_id;
    end loop;
    commit;
  end;
END PKG_DOWNLOAD_FILES;
/
