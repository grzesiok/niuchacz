CREATE OR REPLACE 
PACKAGE PKG_DOWNLOAD_FILES authid definer AS 

  procedure p_job_producer_handler;
  procedure p_job_consumer_handler;

END PKG_DOWNLOAD_FILES;
/
