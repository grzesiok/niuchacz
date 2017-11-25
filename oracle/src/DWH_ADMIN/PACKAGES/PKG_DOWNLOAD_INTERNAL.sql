CREATE OR REPLACE 
PACKAGE PKG_DOWNLOAD_INTERNAL authid definer AS 

  procedure p_hist_insert(i_starttime timestamp,
                          i_stoptime timestamp,
                          i_username varchar2 default sys_context('userenv', 'session_user'),
                          i_url varchar2,
                          i_content in out nocopy blob);

END PKG_DOWNLOAD_INTERNAL;
/