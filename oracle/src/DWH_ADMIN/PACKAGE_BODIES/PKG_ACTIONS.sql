--------------------------------------------------------
--  DDL for Package Body PKG_ACTIONS
--------------------------------------------------------

  CREATE OR REPLACE EDITIONABLE PACKAGE BODY "DWH_ADMIN"."PKG_ACTIONS" AS

  procedure p_enqueue(i_recipient varchar2 default sys_context('userenv', 'session_user'), i_action o_action) AS
    l_queue_options dbms_aq.enqueue_options_t;
    l_message_properties dbms_aq.message_properties_t;
    l_message_id raw(16);
    l_recipients dbms_aq.aq$_recipient_list_t;
    l_cmd t_action;
  begin
    l_cmd := t_action(i_action.f_deserialize);
    l_recipients(1) := sys.aq$_agent(name => i_recipient, address => null, protocol => null);
    l_message_properties.recipient_list := l_recipients;
    dbms_aq.enqueue(queue_name => 'q_core_actions',
                    enqueue_options => l_queue_options,
                    message_properties => l_message_properties,
                    payload => l_cmd,
                    msgid => l_message_id);
    commit;
  END p_enqueue;

  function f_dequeue(i_consumer varchar2 default sys_context('userenv', 'session_user'), i_autocommit boolean default true) return o_action AS
    l_queue_options dbms_aq.dequeue_options_t;
    l_message_properties dbms_aq.message_properties_t;
    l_message_id raw(16);
    l_cmd t_action;
    l_action o_action;
  begin
    l_queue_options.consumer_name := i_consumer;
    l_queue_options.visibility := dbms_aq.on_commit;
    dbms_aq.dequeue(queue_name => 'q_core_actions',
                    dequeue_options => l_queue_options,
                    message_properties => l_message_properties,
                    payload => l_cmd,
                    msgid => l_message_id);
    l_action := o_action.f_serialize(l_cmd.cmd#);
    if(i_autocommit) then
      commit;
    end if;
    return l_action;
  END f_dequeue;

END PKG_ACTIONS;

/