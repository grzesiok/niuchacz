create or replace PACKAGE PKG_ACTIONS_INTERNAL authid definer accessible by (package pkg_actions, type o_action) AS 

  procedure p_enqueue(i_queue_name varchar2,
                      i_recipient varchar2 default sys_context('userenv', 'session_user'),
                      i_autocommit boolean default true,
                      i_action o_action);
  function f_dequeue(i_queue_name varchar2,
                     i_consumer varchar2 default sys_context('userenv', 'session_user'),
                     i_waittime number default dbms_aq.forever,
                     i_autocommit boolean default true) return o_action;
  procedure p_hist_insert(i_username varchar2 default sys_context('userenv', 'session_user'),
                          i_action o_action,
                          i_dbop_result in out nocopy clob);
END PKG_ACTIONS_INTERNAL;
/
