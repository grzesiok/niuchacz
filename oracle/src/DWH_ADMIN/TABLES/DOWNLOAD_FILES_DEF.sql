create table download_files_def
(
  download_file_def_id number generated always as identity,
  url varchar(255),
  user_name varchar2(30),
  last_download date,
  next_download_offset interval day to second
);
