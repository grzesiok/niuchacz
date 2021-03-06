begin
  dbms_aqadm.create_queue_table(queue_table => 'core_actions_queue',
                                comment => 'Main Queue with all actions in system',
                                queue_payload_type => 'SYS.XMLType',
                                multiple_consumers => true);
  dbms_aqadm.create_queue(queue_name => 'q_core_actions',
                          queue_table => 'core_actions_queue');
  dbms_aqadm.start_queue(queue_name => 'q_core_actions');
  dbms_aqadm.start_queue(queue_name => 'aq$_core_actions_queue_e', enqueue => false, dequeue => true);
end;
/
grant execute, under on  o_action to dwh_load with grant option;
grant execute on pkg_cfg_properties to dwh_load;
grant execute on pkg_actions to dwh_load;
@$ORACLE_HOME/rdbms/admin/proftab.sql