--------------------------------------------------------
--  DDL for Type O_ACTION
--------------------------------------------------------

  CREATE OR REPLACE EDITIONABLE TYPE "DWH_ADMIN"."O_ACTION" authid current_user AS OBJECT(
  key# varchar2(30),
  member procedure p_exec,
  final static function f_serialize(i_bytecode varchar2) return o_action,
  member function f_deserialize return varchar2
) not final not instantiable;
/
CREATE OR REPLACE EDITIONABLE TYPE BODY "DWH_ADMIN"."O_ACTION" AS

  member procedure p_exec AS
  BEGIN
    raise_application_error(-20000, 'Unimplemented feature');
  END p_exec;

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
