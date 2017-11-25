create or replace PACKAGE BODY PKG_ACTIONS_INTERNAL AS

  procedure p_enqueue(i_recipient varchar2 default sys_context('userenv', 'session_user'), i_action o_action) AS
    l_queue_options dbms_aq.enqueue_options_t;
    l_message_properties dbms_aq.message_properties_t;
    l_message_id raw(16);
    l_recipients dbms_aq.aq$_recipient_list_t;
    l_cmd xmltype;
  begin
    l_cmd := o_action.f_deserialize(i_action);
    l_recipients(1) := sys.aq$_agent(name => i_recipient, address => null, protocol => null);
    l_message_properties.recipient_list := l_recipients;
    dbms_aq.enqueue(queue_name => 'q_core_actions',
                    enqueue_options => l_queue_options,
                    message_properties => l_message_properties,
                    payload => l_cmd,
                    msgid => l_message_id);
    commit;
  END p_enqueue;

  function f_dequeue(i_consumer varchar2 default sys_context('userenv', 'session_user'), i_waittime number default dbms_aq.forever, i_autocommit boolean default true) return o_action AS
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
    l_action := o_action.f_serialize(l_cmd);
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