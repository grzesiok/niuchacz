create or replace TYPE o_nbpdata_import_action under o_import_files_action
(
  constructor function o_nbpdata_import_action(i_url varchar2, i_hash_md5 raw) return self as result,
  overriding member procedure p_exec
);
/
create or replace TYPE BODY o_nbpdata_import_action AS

  constructor function o_nbpdata_import_action(i_url varchar2, i_hash_md5 raw) return self as result AS
  BEGIN
    self.key# := 'df_oifa_nbpdata';
    self.url# := i_url;
    self.hash_md5# := i_hash_md5;
    RETURN;
  END;

  overriding member procedure p_exec as
  begin
    insert into nbpdata(importtable, importno, importdate, currency_from_code, currency_to_code, mid_rate)
      select json_value(self.content#, '$.table') as importtable,
             json_value(self.content#, '$.no') as importno,
             json_value(self.content#, '$.effectiveDate') importdate,
             'PLN',
             jt.code,
             to_number(replace(jt.mid, '.', ',') default null on conversion error) as mid
      from json_table(self.content#, '$.rates[*]'
           columns (country varchar2(50) path '$.country',
                    currency varchar2(50) path '$.currency',
                    code varchar2(3) path '$.code',
                    bid varchar2(20) path '$.bid',
                    ask varchar2(20) path '$.ask',
                    mid varchar2(20) path '$.mid')) jt;
  end;

END;
/
