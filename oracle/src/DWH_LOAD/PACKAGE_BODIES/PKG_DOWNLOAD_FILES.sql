create or replace PACKAGE BODY PKG_DOWNLOAD_FILES AS

  procedure p_job_producer_df_handler as
    l_curr_date date;
    l_sysdate date := trunc(sysdate, 'DD');
    l_download_options json_object_t;
  begin
    for c in (select * from download_files_def where activeflag = 'Y' and trunc(next_download, 'DD') <= l_sysdate)
    loop
      l_download_options := json_object_t(nvl(c.next_download_options, '{}'));
      if(l_download_options.get_number('skip_old_files') = 1) then
        l_curr_date := l_sysdate;
      else
        l_curr_date := c.next_download;
      end if;
      while(l_curr_date < l_sysdate+c.next_download_offset)
      loop
        dwh_admin.pkg_actions.p_enqueue(i_queue_name => g_queue_downloadfiles,
                                        i_autocommit => false,
                                        i_action => o_download_files_action(c.download_file_def_id, to_char(l_curr_date, c.url), null));
        l_curr_date := l_curr_date+c.next_download_offset;
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
      begin
        dwh_admin.pkg_actions.p_consume_single_action(i_queue_name => g_queue_downloadfiles,
                                                      i_waittime => 1);
      exception
        when dwh_admin.pkg_actions.e_dbmsaq_timeout
          then return;
        when others
          then null;
      end;
    end loop;
  end;
  
  procedure p_job_consumer_if_handler as
  begin
    loop
      begin
        dwh_admin.pkg_actions.p_consume_single_action(i_queue_name => g_queue_importfiles,
                                                      i_waittime => 1);
      exception
        when dwh_admin.pkg_actions.e_dbmsaq_timeout
          then return;
        when others
          then null;
      end;
    end loop;
  end;
END PKG_DOWNLOAD_FILES;
/
