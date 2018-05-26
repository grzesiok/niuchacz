create or replace PACKAGE BODY PKG_DOWNLOAD_INTERNAL AS

  procedure p_hist_insert(i_starttime timestamp,
                          i_stoptime timestamp,
                          i_username varchar2 default sys_context('userenv', 'session_user'),
                          i_url varchar2,
                          i_content in out nocopy blob) AS
  BEGIN
    insert into download_files(start_time#, stop_time#, username#, url#, content#)
      values (i_starttime, i_stoptime, i_username, i_url, i_content);
  END p_hist_insert;

END PKG_DOWNLOAD_INTERNAL;
/
