create or replace TYPE O_ACTION force authid current_user AS OBJECT(
  key# varchar2(30),
  dbop_eid number,
  dbop_name varchar2(4000),
  --initialization process
  member procedure p_create,
  member procedure p_destroy,
  --execution process
  final member procedure p_execbefore,
  member procedure p_exec,
  final member procedure p_execafter,
  --object serialization
  final static function f_serialize(i_bytecode xmltype) return o_action,
  final static function f_deserialize(i_object o_action) return xmltype
) not final not instantiable;
/
create or replace TYPE BODY O_ACTION AS

  member procedure p_create as
  begin
    null;
  end;

  member procedure p_destroy as
  begin
    null;
  end;

  final member procedure p_execbefore AS
  BEGIN
    --dbms_output.put_line('self.dbop_name='||self.dbop_name||' self.dbop_eid='||self.dbop_eid);
    self.dbop_name := self.key#||to_char(systimestamp, 'yyyymmddhh24missff');
    --self.dbop_eid := dbms_sql_monitor.begin_operation(dbop_name => self.dbop_name, forced_tracking => dbms_sql_monitor.force_tracking);
  END p_execbefore;
  
  member procedure p_exec AS
  BEGIN
    raise_application_error(-20000, 'Unimplemented feature');
  END p_exec;

  final member procedure p_execafter AS
    l_dbop_result clob;
  BEGIN
    --dbms_sql_monitor.end_operation(dbop_name => self.dbop_name, dbop_eid => self.dbop_eid);
    --l_dbop_result := dbms_sql_monitor.report_sql_monitor(dbop_name => self.dbop_name, type => 'XML', report_level => 'ALL');
    pkg_actions_internal.p_hist_insert(i_action => self, i_dbop_result => l_dbop_result);
  END p_execafter;

  final static function f_serialize(i_bytecode xmltype) return o_action AS
    l_cmd o_action;
  BEGIN
    i_bytecode.toobject(l_cmd);
    RETURN l_cmd;
  END f_serialize;

  final static function f_deserialize(i_object o_action) return xmltype AS
  BEGIN
    return xmltype(i_object);
  END f_deserialize;

END;
/
