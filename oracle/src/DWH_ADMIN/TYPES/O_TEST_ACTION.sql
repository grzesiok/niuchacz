create or replace TYPE O_TEST_ACTION under o_action(
  constructor function O_TEST_ACTION(i_key varchar2) return self as result,
  overriding member procedure p_exec,
  overriding member function f_deserialize return varchar2
);
/
create or replace TYPE body O_TEST_ACTION as

  constructor function O_TEST_ACTION(i_key varchar2) return self as result as
  begin
    self.key# := i_key;
    return;
  end;

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