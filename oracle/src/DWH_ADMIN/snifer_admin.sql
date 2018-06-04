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
BEGIN
  DBMS_SCHEDULER.CREATE_JOB(job_name => 'j_download_files_producer',
                            job_type => 'PLSQL_BLOCK',
                            job_action => 'BEGIN pkg_download_files.p_job_producer_handler; END;',
                            --start_date => /* As soon as job is enabled */,
                            repeat_interval => 'FREQ=HOURLY',
                            enabled =>  TRUE,
                            comments => 'Automatic produce queue actions for fresh files');
END;
/
BEGIN
  DBMS_SCHEDULER.CREATE_JOB(job_name => 'j_download_files_consumer',
                            job_type => 'PLSQL_BLOCK',
                            job_action => 'BEGIN pkg_download_files.p_job_consumer_handler; END;',
                            --start_date => /* As soon as job is enabled */,
                            repeat_interval => 'FREQ=HOURLY',
                            enabled =>  TRUE,
                            comments => 'Automatic consume download actions');
END;
/
