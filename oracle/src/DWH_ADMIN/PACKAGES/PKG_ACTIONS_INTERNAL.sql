create or replace PACKAGE PKG_ACTIONS_INTERNAL authid definer accessible by (package pkg_actions, type o_action) AS 

  procedure p_hist_insert(i_username varchar2 default sys_context('userenv', 'session_user'),
                          i_action varchar2,
                          i_dbop_result clob);
END PKG_ACTIONS_INTERNAL;
/