--------------------------------------------------------
--  DDL for Package PKG_FILES
--------------------------------------------------------

  CREATE OR REPLACE EDITIONABLE PACKAGE "DWH_ADMIN"."PKG_FILES" authid definer is
  function get_file(i_filepath in varchar2) return T_FILE;
  function get_file_list(i_directorypath in varchar2) return T_FILES;
end;

/