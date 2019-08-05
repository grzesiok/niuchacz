CREATE OR REPLACE EDITIONABLE PACKAGE pk_files authid definer is
  function get_file(i_filepath in varchar2) return T_FILE;
  function get_file_list(i_directorypath in varchar2) return T_FILES;
end;
/
