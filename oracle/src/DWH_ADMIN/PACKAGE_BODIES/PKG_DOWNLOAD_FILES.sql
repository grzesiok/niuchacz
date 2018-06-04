create or replace PACKAGE BODY PKG_DOWNLOAD_FILES AS

  procedure p_job_producer_handler as
    l_curr_date date;
    l_sysdate date := sysdate;
  begin
    for c in (select * from download_files_def)
    loop
      l_curr_date := c.last_download;
      while(l_curr_date < l_sysdate)
      loop
        l_curr_date := l_curr_date + c.next_download_offset;
        pkg_actions.p_enqueue(i_recipient => 'RECP_DOWNLOAD_FILES', i_autocommit => false, i_action => o_download_files_action(to_char(l_curr_date, c.url), null));
      end loop;
      update download_files_def
        set last_download = l_sysdate
      where download_file_def_id = c.download_file_def_id;
    end loop;
    commit;
  end;

  procedure p_job_consumer_handler as
  begin
    loop
      pkg_actions.p_consume_single_action(i_consumer => 'RECP_DOWNLOAD_FILES',
                                          i_waittime => 1);
    end loop;
  end;
END PKG_DOWNLOAD_FILES;
/
