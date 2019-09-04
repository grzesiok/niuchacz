CREATE OR REPLACE EDITIONABLE TYPE BODY "O_IMPORT" AS

  member function f_get_directory_name return varchar2 AS
  BEGIN
    raise_application_error(-20000, 'Unimplemented feature');
    return null;
  END f_get_directory_name;
  
  member function f_get_directory_histname return varchar2 AS
  BEGIN
    raise_application_error(-20000, 'Unimplemented feature');
    return null;
  END f_get_directory_histname;
  
  member function f_get_filterpath return varchar2 AS
  BEGIN
    raise_application_error(-20000, 'Unimplemented feature');
    return null;
  END f_get_filterpath;
  
  member function f_is_active return boolean AS
  BEGIN
    return false;
  END f_is_active;
  
  member procedure p_import_file(i_filename varchar2) AS
  BEGIN
    raise_application_error(-20000, 'Unimplemented feature');
  END p_import_file;

END;
