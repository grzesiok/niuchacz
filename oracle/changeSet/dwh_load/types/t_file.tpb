CREATE OR REPLACE EDITIONABLE TYPE BODY t_file as
  constructor function T_FILE(i_filepath varchar2) return self as result as
  begin
    self.filepath := i_filepath;
    return;
  end;
end;
/
