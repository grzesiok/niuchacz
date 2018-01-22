create or replace PACKAGE BODY PKG_ACTIONS AS

  procedure p_job_handler as
    pragma autonomous_transaction;
    l_action o_action;
  BEGIN
    loop
      l_action := f_dequeue(i_autocommit => false, i_waittime => 1);
      exit when (l_action is null);
      o_action.p_process(l_action);
      commit;
    end loop;
  exception
    when e_dbmsaq_timeout then null;
  END;

  procedure p_enqueue(i_recipient varchar2 default sys_context('userenv', 'session_user'), i_action o_action) AS
  begin
    pkg_actions_internal.p_enqueue(i_recipient => i_recipient, i_action => i_action);
  END p_enqueue;

  function f_dequeue(i_consumer varchar2 default sys_context('userenv', 'session_user'), i_waittime number, i_autocommit boolean default true) return o_action AS
  begin
    return pkg_actions_internal.f_dequeue(i_consumer => i_consumer, i_waittime => i_waittime, i_autocommit => i_autocommit);
  END f_dequeue;

END PKG_ACTIONS;
/
