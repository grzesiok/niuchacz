create or replace PACKAGE BODY PKG_ACTIONS_INTERNAL AS

  procedure p_heartbeat_queue(i_queue_name varchar2,
                              i_exception_queue_name varchar2 default null) as
    l_queue_is_active varchar2(1);
    l_upp_queue_name user_queues.name%type;
    l_upp_exception_queue_name user_queues.name%type;
  begin
    l_upp_queue_name := upper(i_queue_name);
    /* checking if queue is in correct state (base queue must have possibility to enqueue and dequeue) */
    select case when exists(select 1 from user_queues
                            where name = l_upp_queue_name
                              and (trim(enqueue_enabled) = 'NO' or trim(dequeue_enabled) = 'NO'))
        then 'N' else 'Y' end into l_queue_is_active
    from dual;
    /* starting/restarting queue if it is needed */
    if(l_queue_is_active = 'N') then
      dbms_aqadm.stop_queue(queue_name => l_upp_queue_name);
      dbms_aqadm.start_queue(queue_name => l_upp_queue_name,
                             enqueue => true,
                             dequeue => true);
    end if;
    if(i_exception_queue_name is not null) then
      l_upp_exception_queue_name := upper(i_exception_queue_name);
      /* checking if queue is in correct state (exception queue must have possibility only to enqueue elements) */
      select case when exists(select 1 from user_queues
                              where name = l_upp_exception_queue_name
                                and (trim(enqueue_enabled) = 'YES' or trim(dequeue_enabled) = 'NO'))
          then 'Y' else 'N' end into l_queue_is_active
      from dual;
      /* starting/restarting queue if it is needed */
      if(l_queue_is_active = 'N') then
        dbms_aqadm.stop_queue(queue_name => l_upp_exception_queue_name);
        dbms_aqadm.start_queue(queue_name => l_upp_exception_queue_name,
                               enqueue => false,
                               dequeue => true);
      end if;
    end if;
  end;

  procedure p_enqueue(i_recipient varchar2 default sys_context('userenv', 'session_user'),
                      i_autocommit boolean default true,
                      i_action o_action) AS
    l_queue_options dbms_aq.enqueue_options_t;
    l_message_properties dbms_aq.message_properties_t;
    l_message_id raw(16);
    l_recipients dbms_aq.aq$_recipient_list_t;
    l_cmd xmltype;
  begin
    p_heartbeat_queue(i_queue_name => 'q_core_actions',
                      i_exception_queue_name => 'aq$_core_actions_queue_e');
    l_cmd := o_action.f_deserialize(i_action);
    l_recipients(1) := sys.aq$_agent(name => i_recipient, address => null, protocol => null);
    l_message_properties.recipient_list := l_recipients;
    dbms_aq.enqueue(queue_name => 'q_core_actions',
                    enqueue_options => l_queue_options,
                    message_properties => l_message_properties,
                    payload => l_cmd,
                    msgid => l_message_id);
    if(i_autocommit) then
      commit;
    end if;
  END p_enqueue;

  function f_dequeue(i_consumer varchar2 default sys_context('userenv', 'session_user'),
                     i_waittime number default dbms_aq.forever,
                     i_autocommit boolean default true) return o_action AS
    l_queue_options dbms_aq.dequeue_options_t;
    l_message_properties dbms_aq.message_properties_t;
    l_message_id raw(16);
    l_cmd xmltype;
    l_action o_action;
  begin
    l_queue_options.consumer_name := i_consumer;
    l_queue_options.visibility := dbms_aq.on_commit;
    l_queue_options.wait := i_waittime;
    dbms_aq.dequeue(queue_name => 'q_core_actions',
                    dequeue_options => l_queue_options,
                    message_properties => l_message_properties,
                    payload => l_cmd,
                    msgid => l_message_id);
    execute immediate 'declare
                         l_cmd xmltype;
                         l_action '||l_cmd.getrootelement()||';
                       begin
                         l_cmd := :1;
                         l_cmd.toobject(l_action);
                         :2 := l_action;
                       end;' using in l_cmd, in out l_action;
    if(i_autocommit) then
      commit;
    end if;
    return l_action;
  END f_dequeue;

  procedure p_hist_insert(i_username varchar2 default sys_context('userenv', 'session_user'),
                          i_action o_action,
                          i_dbop_result in out nocopy clob) AS
  BEGIN
    insert into core_actions_hist(username, action, dbop_result)
      values (i_username, o_action.f_deserialize(i_action), i_dbop_result);
  END p_hist_insert;

END PKG_ACTIONS_INTERNAL;
/
