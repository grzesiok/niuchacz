create or replace PACKAGE PKG_ACTIONS authid current_user AS

  e_dbmsaq_timeout exception;
  pragma exception_init(e_dbmsaq_timeout, -25228);

  g_queue_coreactions constant varchar2(128) := 'Q_CORE_ACTIONS';

  procedure p_job_handler;
  procedure p_exec(i_action o_action);
  procedure p_consume_single_action(i_queue_name varchar2,
                                    i_consumer varchar2 default sys_context('userenv', 'session_user'),
                                    i_waittime number);
  procedure p_enqueue(i_queue_name varchar2,
                      i_recipient varchar2 default sys_context('userenv', 'session_user'),
                      i_autocommit boolean default true,
                      i_action o_action);
  function f_dequeue(i_queue_name varchar2,
                     i_consumer varchar2 default sys_context('userenv', 'session_user'),
                     i_waittime number,
                     i_autocommit boolean default true) return o_action;

END PKG_ACTIONS;
/
