@$ORACLE_HOME/rdbms/admin/proftab.sql
BEGIN
  DBMS_SCHEDULER.CREATE_JOB(job_name => 'j_download_files_producer',
                            job_type => 'PLSQL_BLOCK',
                            job_action => 'BEGIN pkg_download_files.p_job_producer_df_handler; END;',
                            --start_date => /* As soon as job is enabled */,
                            repeat_interval => 'FREQ=HOURLY',
                            enabled =>  TRUE,
                            comments => 'Automatic produce queue actions for fresh files');
END;
/
BEGIN
  DBMS_SCHEDULER.CREATE_JOB(job_name => 'j_download_files_consumer',
                            job_type => 'PLSQL_BLOCK',
                            job_action => 'BEGIN pkg_download_files.p_job_consumer_df_handler; END;',
                            --start_date => /* As soon as job is enabled */,
                            repeat_interval => 'FREQ=HOURLY',
                            enabled =>  TRUE,
                            comments => 'Automatic consume download actions');
END;
/
BEGIN
  DBMS_SCHEDULER.CREATE_JOB(job_name => 'j_import_files_consumer',
                            job_type => 'PLSQL_BLOCK',
                            job_action => 'BEGIN pkg_download_files.p_job_consumer_if_handler; END;',
                            --start_date => /* As soon as job is enabled */,
                            repeat_interval => 'FREQ=HOURLY',
                            enabled =>  TRUE,
                            comments => 'Automatic consume import actions');
END;
/
grant execute on o_download_files_action to dwh_admin;
grant execute on o_import_files_action to dwh_admin;
grant execute on o_nbpdata_import_action to dwh_admin;
