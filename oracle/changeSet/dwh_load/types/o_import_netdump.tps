CREATE OR REPLACE EDITIONABLE TYPE "O_IMPORT_NETDUMP" under O_IMPORT
(
  overriding member function f_get_directory_name return varchar2,
  overriding member function f_get_directory_histname return varchar2,
  overriding member function f_get_filterpath return varchar2,
  overriding member function f_is_active return boolean,
  overriding member procedure p_import_file(i_filename varchar2)
);
