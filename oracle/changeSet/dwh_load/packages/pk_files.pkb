CREATE OR REPLACE EDITIONABLE PACKAGE BODY pk_files is
  function get_file(i_filepath in varchar2) return t_file
  is language java name 'tools.File.getFile(java.lang.String) return oracle.sql.STRUCT';

  function get_file_list(i_directorypath in varchar2) return t_files
  is language java name 'tools.File.getFileList(java.lang.String) return oracle.sql.ARRAY';
end;

/
