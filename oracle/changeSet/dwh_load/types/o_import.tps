CREATE OR REPLACE EDITIONABLE TYPE "O_IMPORT" force authid definer AS OBJECT
(
  l_import_timestamp timestamp,
  member function f_get_directory_name return varchar2,
  member function f_get_directory_histname return varchar2,
  member function f_get_filterpath return varchar2,
  member function f_is_active return boolean,
  member procedure p_import_file(i_filename varchar2)
) not final not instantiable;
