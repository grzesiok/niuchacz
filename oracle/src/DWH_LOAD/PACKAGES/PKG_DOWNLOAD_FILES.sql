CREATE OR REPLACE 
PACKAGE PKG_DOWNLOAD_FILES authid definer AS 

  g_queue_downloadfiles constant varchar2(128) := 'Q_DOWNLOAD_FILES';
  g_queue_importfiles constant varchar2(128) := 'Q_IMPORT_FILES';

  procedure p_job_producer_df_handler;
  procedure p_job_consumer_df_handler;
  procedure p_job_consumer_if_handler;

END PKG_DOWNLOAD_FILES;
/
