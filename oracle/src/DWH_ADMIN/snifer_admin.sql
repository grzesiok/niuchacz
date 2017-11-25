begin
  dbms_aqadm.create_queue_table(queue_table => 'core_actions_queue',
                                comment => 'Main Queue with all actions in system',
                                queue_payload_type => 'SYS.XMLType',
                                multiple_consumers => true);
  dbms_aqadm.create_queue(queue_name => 'q_core_actions',
                          queue_table => 'core_actions_queue');
  insert into cfg_properties(key#, value#)
    values ('download_retries', '3');
  insert into cfg_properties(key#, value#)
    values ('download_retry_wait', '1');
  commit;
end;
/