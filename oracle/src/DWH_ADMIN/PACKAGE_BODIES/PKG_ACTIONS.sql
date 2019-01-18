create or replace PACKAGE BODY PKG_ACTIONS AS

  procedure p_job_handler as
  BEGIN
    loop
      p_consume_single_action(i_queue_name => g_queue_coreactions,
                              i_waittime => 1);
    end loop;
  END;

  procedure p_exec(i_action o_action) as
    l_action o_action := i_action;
  begin
    if(l_action is null) then
      return;
    end if;
    l_action.p_create;
    l_action.p_execbefore;
    l_action.p_exec;
    l_action.p_execafter;
    l_action.p_destroy;
  end;

  procedure p_consume_single_action(i_queue_name varchar2,
                                    i_consumer varchar2 default sys_context('userenv', 'session_user'),
                                    i_waittime number) as
    pragma autonomous_transaction;
    l_action o_action;
  begin
    l_action := f_dequeue(i_queue_name => i_queue_name,
                          i_consumer => i_consumer,
                          i_autocommit => false,
                          i_waittime => 1);
    p_exec(i_action => l_action);
    commit;
  exception
    --when e_dbmsaq_timeout then null;
    when others then
      rollback;
      raise_application_error(-20000, 'Error during processing action.', true);
  end;

  procedure p_enqueue(i_queue_name varchar2,
                      i_recipient varchar2 default sys_context('userenv', 'session_user'),
                      i_autocommit boolean default true,
                      i_action o_action) AS
  begin
    pkg_actions_internal.p_enqueue(i_queue_name => i_queue_name,
                                   i_recipient => i_recipient,
                                   i_autocommit => i_autocommit,
                                   i_action => i_action);
  END p_enqueue;

  function f_dequeue(i_queue_name varchar2,
                     i_consumer varchar2 default sys_context('userenv', 'session_user'),
                     i_waittime number,
                     i_autocommit boolean default true) return o_action AS
  begin
    return pkg_actions_internal.f_dequeue(i_queue_name => i_queue_name,
                                          i_consumer => i_consumer,
                                          i_waittime => i_waittime,
                                          i_autocommit => i_autocommit);
  END f_dequeue;

END PKG_ACTIONS;
/
