--------------------------------------------------------
--  DDL for Type O_TEST_ACTION
--------------------------------------------------------

  CREATE OR REPLACE EDITIONABLE TYPE "DWH_ADMIN"."O_TEST_ACTION" under o_action(
  overriding member procedure p_exec,
  overriding member function f_deserialize return varchar2
)
/
CREATE OR REPLACE EDITIONABLE TYPE BODY "DWH_ADMIN"."O_TEST_ACTION" as
  overriding member procedure p_exec as
  begin
    dbms_output.put_line(self.key#);
  end;
  
  overriding member function f_deserialize return varchar2 as
  begin
    return 'o_test_action('''||self.key#||''');';
  end;
end;

/
