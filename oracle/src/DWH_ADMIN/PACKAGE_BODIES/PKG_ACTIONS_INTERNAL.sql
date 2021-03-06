create or replace PACKAGE BODY PKG_ACTIONS_INTERNAL AS

  g_queue_table constant varchar2(128) := 'core_actions_queue';
  e_dbmsaq_end_of_group exception;
  pragma exception_init(e_dbmsaq_end_of_group, -25235);

  procedure p_heartbeat_queue(i_queue_name varchar2) as
    l_upp_queue_table user_queues.queue_table%type;
    l_upp_queue_name user_queues.name%type;
    l_upp_exception_queue_name user_queues.name%type;
    
    l_queue_table_exists varchar2(1);
    l_queue_exists varchar2(1);
    l_queue_is_active varchar2(1);
    l_exceptionqueue_exists varchar2(1);
    l_exceptionqueue_is_active varchar2(1);
  begin
    if(i_queue_name like '%\_E' escape '\') then
      l_upp_queue_name := rtrim(upper(i_queue_name), '_E');
    else
      l_upp_queue_name := upper(i_queue_name);
    end if;
    l_upp_queue_table := upper(g_queue_table);
    l_upp_exception_queue_name := upper(l_upp_queue_name)||'_E';
    dbms_output.put_line('l_upp_queue_name='||l_upp_queue_name||' l_upp_queue_table='||l_upp_queue_table||' l_upp_exception_queue_name='||l_upp_exception_queue_name);
    /* checking if queue and exception_queue is in correct state */
    select case when exists(select 1 from user_queue_tables
                            where queue_table = l_upp_queue_table)
                  then 'Y' else 'N' end,
           case when exists(select 1 from user_queues
                            where queue_table = l_upp_queue_table
                              and name = l_upp_queue_name
                              and queue_type = 'NORMAL_QUEUE')
                  then 'Y' else 'N' end,
           case when exists(select 1 from user_queues
                            where queue_table = l_upp_queue_table
                              and name = l_upp_queue_name
                              and queue_type = 'NORMAL_QUEUE'
                              and trim(enqueue_enabled) = 'YES'
                              and trim(dequeue_enabled) = 'YES')
                  then 'Y' else 'N' end,
    	   case when exists(select 1 from user_queues
                            where queue_table = l_upp_queue_table
                              and name = l_upp_exception_queue_name
                              and queue_type = 'EXCEPTION_QUEUE')
                  then 'Y' else 'N' end,
    	   case when exists(select 1 from user_queues
                            where queue_table = l_upp_queue_table
                              and name = l_upp_exception_queue_name
                              and queue_type = 'EXCEPTION_QUEUE'
                              and trim(enqueue_enabled) = 'YES'
                              and trim(dequeue_enabled) = 'NO')
                  then 'Y' else 'N' end
      into l_queue_table_exists,
           l_queue_exists,
           l_queue_is_active,
           l_exceptionqueue_exists,
           l_exceptionqueue_is_active
    from dual;
    /* If queue table not exists we need to create it */
    if(l_queue_table_exists = 'N') then
      dbms_aqadm.create_queue_table(queue_table => g_queue_table,
                                    comment => 'Main Queue with all actions in system',
                                    queue_payload_type => 'SYS.XMLType',
                                    multiple_consumers => true,
                                    message_grouping => dbms_aqadm.transactional);
    end if;
    /* If queue not exists we need to create it */
    if(l_queue_exists = 'N') then
      dbms_aqadm.create_queue(queue_name => l_upp_queue_name,
                              queue_type => dbms_aqadm.normal_queue,
                              queue_table => g_queue_table);
    end if;
    /* starting/restarting queue if it is needed */
    if(l_queue_is_active = 'N') then
      dbms_aqadm.stop_queue(queue_name => l_upp_queue_name);
      dbms_aqadm.start_queue(queue_name => l_upp_queue_name,
                             enqueue => true,
                             dequeue => true);
    end if;
    /* if exception table not exists then we need to create it */
    if(l_exceptionqueue_exists = 'N') then
      dbms_aqadm.create_queue(queue_name => l_upp_exception_queue_name,
                              queue_type => dbms_aqadm.exception_queue,
                              queue_table => g_queue_table);
    end if;
    /* starting/restarting exception queue if it is needed */
    if(l_exceptionqueue_is_active = 'N') then
      dbms_aqadm.stop_queue(queue_name => l_upp_exception_queue_name);
      dbms_aqadm.start_queue(queue_name => l_upp_exception_queue_name,
                             enqueue => false,
                             dequeue => true);
    end if;
  end;

  procedure p_enqueue(i_queue_name varchar2,
                      i_recipient varchar2 default sys_context('userenv', 'session_user'),
                      i_autocommit boolean default true,
                      i_action o_action) AS
    l_queue_name varchar2(4000);
    l_queue_options dbms_aq.enqueue_options_t;
    l_message_properties dbms_aq.message_properties_t;
    l_message_id raw(16);
    l_recipients dbms_aq.aq$_recipient_list_t;
    l_cmd xmltype;
  begin
    l_queue_name := upper(i_queue_name);
    p_heartbeat_queue(i_queue_name => l_queue_name);
    l_cmd := o_action.f_deserialize(i_action);
    dbms_output.put_line('l_cmd='||l_cmd.getClobVal());
    l_recipients(1) := sys.aq$_agent(name => i_recipient, address => null, protocol => null);
    l_message_properties.recipient_list := l_recipients;
    l_message_properties.expiration := dbms_aq.never;
    l_message_properties.exception_queue := l_queue_name||'_E';
    l_queue_options.visibility := dbms_aq.on_commit;
    dbms_aq.enqueue(queue_name => l_queue_name,
                    enqueue_options => l_queue_options,
                    message_properties => l_message_properties,
                    payload => l_cmd,
                    msgid => l_message_id);
    if(i_autocommit) then
      commit;
    end if;
  END p_enqueue;

  function f_dequeue(i_queue_name varchar2,
                     i_consumer varchar2 default sys_context('userenv', 'session_user'),
                     i_waittime number default dbms_aq.forever,
                     i_autocommit boolean default true) return o_action AS
    l_queue_name varchar2(4000);
    l_queue_options dbms_aq.dequeue_options_t;
    l_message_properties dbms_aq.message_properties_t;
    l_message_id raw(16);
    l_cmd xmltype;
    l_action o_action;
  begin
    l_queue_name := upper(i_queue_name);
    if(l_queue_name not like '%\_E' escape '\') then
      p_heartbeat_queue(i_queue_name => l_queue_name);
      l_queue_options.consumer_name := i_consumer;
    end if;
    l_queue_options.visibility := dbms_aq.on_commit;
    l_queue_options.wait := i_waittime;
    l_queue_options.navigation := dbms_aq.first_message;
    loop
      begin
        dbms_aq.dequeue(queue_name => l_queue_name,
                        dequeue_options => l_queue_options,
                        message_properties => l_message_properties,
                        payload => l_cmd,
                        msgid => l_message_id);
        exit;
      exception
        when e_dbmsaq_end_of_group
          then l_queue_options.navigation := dbms_aq.next_transaction;
      end;
    end loop;
    execute immediate 'declare
                         l_cmd xmltype;
                         l_action '||i_consumer||'.'||l_cmd.getrootelement()||';
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
