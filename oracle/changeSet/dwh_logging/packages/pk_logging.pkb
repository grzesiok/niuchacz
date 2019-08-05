create or replace
package body pk_logging as

  procedure log_job_start(p_in_job_name varchar2) as
  begin
    null;
  end;

  procedure log_file_start(p_in_job_name varchar2, p_in_file_name varchar2) as
  begin
    null;
  end;

  procedure log_file_end(p_in_status logging_status default c_logstatus_success) as
  begin
    null;
  end;

  procedure log_job_end(p_in_status logging_status default c_logstatus_success) as
  begin
    null;
  end;

  procedure log_error(p_in_errortype logging_errtype default c_logerrtype_normal,
                      p_in_message varchar2) as
  begin
    null;
  end;

end;
