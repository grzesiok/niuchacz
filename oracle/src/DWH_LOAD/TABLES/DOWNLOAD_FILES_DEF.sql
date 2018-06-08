create table download_files_def
(
  download_file_def_id number generated always as identity,
  url varchar(255),
  user_name varchar2(30),
  next_download date,
  next_download_offset interval day to second,
  import_action_owner varchar2(128),
  import_action_typename varchar2(128),
  activeflag varchar2(1)
);
