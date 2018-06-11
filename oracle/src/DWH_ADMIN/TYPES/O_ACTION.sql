create or replace TYPE O_ACTION force authid current_user AS OBJECT(
  key# varchar2(30),
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
    dbms_profiler.start_profiler(run_comment => self.key#||'_'||to_char(systimestamp, 'yyyymmddhh24missff'));
  END p_execbefore;
  
  member procedure p_exec AS
  BEGIN
    raise_application_error(-20000, 'Unimplemented feature');
  END p_exec;

  final member procedure p_execafter AS
  BEGIN
    dbms_profiler.stop_profiler;
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
