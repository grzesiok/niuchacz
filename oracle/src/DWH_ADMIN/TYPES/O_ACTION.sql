create or replace TYPE O_ACTION authid current_user AS OBJECT(
  key# varchar2(30),
  dbop_eid number,
  dbop_name varchar2(4000),
  final member procedure p_execbefore,
  member procedure p_exec,
  final member procedure p_execafter,
  final static function f_serialize(i_bytecode varchar2) return o_action,
  member function f_deserialize return varchar2
) not final not instantiable;
/
create or replace TYPE BODY O_ACTION AS

  final member procedure p_execbefore AS
  BEGIN
    dbop_name := self.key#||to_char(systimestamp, 'yyyymmddhh24missff');
    self.dbop_eid := dbms_sql_monitor.begin_operation(dbop_name => self.dbop_name, forced_tracking => dbms_sql_monitor.force_tracking);
  END p_execbefore;

  member procedure p_exec AS
  BEGIN
    raise_application_error(-20000, 'Unimplemented feature');
  END p_exec;

  final member procedure p_execafter AS
    pragma autonomous_transaction;
    l_dbop_result clob;
  BEGIN
    dbms_sql_monitor.end_operation(dbop_name => self.dbop_name, dbop_eid => self.dbop_eid);
    l_dbop_result := dbms_sql_monitor.report_sql_monitor(dbop_name => self.dbop_name, type => 'XML', report_level => 'ALL');
    insert into core_actions_hist(log_time, action, dbop_result)
      values (systimestamp, self.f_deserialize, l_dbop_result);
    commit;
  END p_execafter;

  final static function f_serialize(i_bytecode varchar2) return o_action AS
    l_cmd o_action;
  BEGIN
    execute immediate 'begin :1 := '||i_bytecode||' end;' using in out l_cmd;
    RETURN l_cmd;
  END f_serialize;

  member function f_deserialize return varchar2 AS
  BEGIN
    raise_application_error(-20000, 'Unimplemented feature');
    RETURN NULL;
  END f_deserialize;

END;
/