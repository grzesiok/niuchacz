create or replace
package pk_logging authid definer as
  subtype logging_status is pls_integer;
  subtype logging_errtype is pls_integer;

  c_logstatus_success logging_status := 1;
  c_logstatus_error logging_status := 2;
  
  c_logerrtype_warning logging_status := 1;
  c_logerrtype_normal logging_status := 2;
  c_logerrtype_critical logging_status := 3;

  procedure log_job_start(p_in_job_name varchar2);
  procedure log_file_start(p_in_job_name varchar2, p_in_file_name varchar2);
  procedure log_file_end(p_in_status logging_status default c_logstatus_success);
  procedure log_job_end(p_in_status logging_status default c_logstatus_success);
  procedure log_error(p_in_errortype logging_errtype default c_logerrtype_normal,
                      p_in_message varchar2);
end;
/
