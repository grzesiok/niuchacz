create or replace PACKAGE BODY PKG_DOWNLOAD_FILES AS

  procedure p_job_producer_df_handler as
    l_curr_date date;
    l_sysdate date := sysdate;
  begin
    for c in (select * from download_files_def where activeflag = 'Y')
    loop
      l_curr_date := c.next_download;
      while(l_curr_date < l_sysdate)
      loop
        l_curr_date := l_curr_date + c.next_download_offset;
        dwh_admin.pkg_actions.p_enqueue(i_queue_name => g_queue_downloadfiles,
                                        i_autocommit => false,
                                        i_action => o_download_files_action(c.download_file_def_id, to_char(l_curr_date, c.url), null));
      end loop;
      update download_files_def
        set next_download = l_sysdate+1
      where download_file_def_id = c.download_file_def_id;
    end loop;
    commit;
  end;

  procedure p_job_consumer_df_handler as
  begin
    loop
      dwh_admin.pkg_actions.p_consume_single_action(i_queue_name => g_queue_downloadfiles,
                                                    i_waittime => 1);
    end loop;
  end;
  
  procedure p_job_consumer_if_handler as
  begin
    loop
      dwh_admin.pkg_actions.p_consume_single_action(i_queue_name => g_queue_importfiles,
                                                    i_waittime => 1);
    end loop;
  end;
END PKG_DOWNLOAD_FILES;
/
