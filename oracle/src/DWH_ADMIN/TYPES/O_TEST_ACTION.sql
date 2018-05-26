create or replace TYPE O_TEST_ACTION force under o_action(
  constructor function O_TEST_ACTION(i_key varchar2) return self as result,
  overriding member procedure p_exec
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
end;
/
