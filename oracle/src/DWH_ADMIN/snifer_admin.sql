exec dbms_aqadm.stop_queue(queue_name => 'q_core_actions');
exec dbms_aqadm.drop_queue(queue_name => 'q_core_actions');
exec dbms_aqadm.drop_queue_table(queue_table => 'core_actions_queue');
begin
  dbms_aqadm.create_queue_table(queue_table => 'core_actions_queue',
                                comment => 'Main Queue with all actions in system',
                                queue_payload_type => 'SYS.XMLType',
                                multiple_consumers => true);
  dbms_aqadm.create_queue(queue_name => 'q_core_actions',
                          queue_table => 'core_actions_queue');
  dbms_aqadm.start_queue(queue_name => 'q_core_actions');
  dbms_aqadm.start_queue(queue_name => 'aq$_core_actions_queue_e',
                         enqueue => false,
                         dequeue => true);
  insert into cfg_properties(key#, value#)
    values ('download_retries', '3');
  insert into cfg_properties(key#, value#)
    values ('download_retry_wait', '1');
  commit;
end;
/