create or replace TYPE o_import_files_action force under dwh_admin.o_action(
  url# varchar2(4000),
  hash_md5# raw(16),
  content# blob,
  constructor function o_import_files_action(i_url varchar2, i_hash_md5 raw) return self as result,
  overriding member procedure p_create,
  overriding member procedure p_destroy
) not final not instantiable;
/
create or replace TYPE BODY o_import_files_action AS

  constructor function o_import_files_action(i_url varchar2, i_hash_md5 raw) return self as result AS
  BEGIN
    self.key# := 'df_oifa';
    self.url# := i_url;
    self.hash_md5# := i_hash_md5;
    RETURN;
  END o_import_files_action;

  overriding member procedure p_create as
  begin
    select content# into self.content#
    from download_files
    where url# = self.url#;
  end;
  
  overriding member procedure p_destroy as
  begin
    null;
  end;

END;
/
