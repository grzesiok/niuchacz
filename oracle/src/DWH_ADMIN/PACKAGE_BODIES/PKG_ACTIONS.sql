create or replace PACKAGE BODY PKG_ACTIONS AS

  procedure p_job_handler as
    pragma autonomous_transaction;
    l_action o_action;
  BEGIN
    loop
      l_action := f_dequeue(i_autocommit => false, i_waittime => 1);
      exit when (l_action is null);
      l_action.p_create;
      l_action.p_execbefore;
      l_action.p_exec;
      l_action.p_execafter;
      l_action.p_destroy;
      commit;
    end loop;
  exception
    when e_dbmsaq_timeout then null;
    when others then
      if(l_action.dbop_eid is not null) then
        null;--dbms_sql_monitor.end_operation(dbop_name => i_action.dbop_name, dbop_eid => i_action.dbop_eid);
      end if;
      rollback;
      raise_application_error(-20000, 'Error during processing action.', true);
  END;

  procedure p_enqueue(i_recipient varchar2 default sys_context('userenv', 'session_user'),
                      i_autocommit boolean default true,
                      i_action o_action) AS
  begin
    pkg_actions_internal.p_enqueue(i_recipient => i_recipient,
                                   i_autocommit => i_autocommit,
                                   i_action => i_action);
  END p_enqueue;

  function f_dequeue(i_consumer varchar2 default sys_context('userenv', 'session_user'),
                     i_waittime number,
                     i_autocommit boolean default true) return o_action AS
  begin
    return pkg_actions_internal.f_dequeue(i_consumer => i_consumer,
                                          i_waittime => i_waittime,
                                          i_autocommit => i_autocommit);
  END f_dequeue;

END PKG_ACTIONS;
/
