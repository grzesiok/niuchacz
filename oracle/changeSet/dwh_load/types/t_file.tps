CREATE OR REPLACE EDITIONABLE TYPE t_file authid current_user as object
(
  filepath varchar2(4000),
  filename varchar2(4000),
  filesize number,
  fileexists char(1),
  lastmodified  timestamp,
  isdir char(1),
  iswriteable char(1),
  isreadable char(1),
  isexecutable char(1),
  hashvalue number,
  constructor function T_FILE(i_filepath varchar2) return self as result
);
