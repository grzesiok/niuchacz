create or replace PACKAGE BODY PKG_ACTIONS_INTERNAL AS

  procedure p_hist_insert(i_username varchar2 default sys_context('userenv', 'session_user'),
                          i_action varchar2,
                          i_dbop_result clob) AS
  BEGIN
    insert into core_actions_hist(username, action, dbop_result)
      values (i_username, i_action, i_dbop_result);
  END p_hist_insert;

END PKG_ACTIONS_INTERNAL;
/