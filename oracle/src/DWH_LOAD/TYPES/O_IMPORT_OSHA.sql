--------------------------------------------------------
--  DDL for Type O_IMPORT_OSHA
--------------------------------------------------------

  CREATE OR REPLACE EDITIONABLE TYPE "O_IMPORT_OSHA" force under o_import
(
  overriding member function f_get_directory_name return varchar2,
  overriding member function f_get_directory_histname return varchar2
) not final
/
CREATE OR REPLACE EDITIONABLE TYPE BODY "O_IMPORT_OSHA" AS

  overriding member function f_get_directory_name return varchar2 AS
  BEGIN
    return 'DWH_OSHA_DIR';
  END f_get_directory_name;
  
  overriding member function f_get_directory_histname return varchar2 AS
  BEGIN
    return 'DWH_OSHAHIST_DIR';
  END f_get_directory_histname;

END;

/
