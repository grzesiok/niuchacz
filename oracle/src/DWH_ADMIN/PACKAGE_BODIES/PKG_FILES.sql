--------------------------------------------------------
--  DDL for Package Body PKG_FILES
--------------------------------------------------------

  CREATE OR REPLACE EDITIONABLE PACKAGE BODY "DWH_ADMIN"."PKG_FILES" is
  function get_file(i_filepath in varchar2) return T_FILE
  is language java name 'tools.File.getFile(java.lang.String) return oracle.sql.STRUCT';

  function get_file_list(i_directorypath in varchar2) return T_FILES
  is language java name 'tools.File.getFileList(java.lang.String) return oracle.sql.ARRAY';
end;

/

  GRANT EXECUTE ON "DWH_ADMIN"."PKG_FILES" TO "DWH_LOAD";
