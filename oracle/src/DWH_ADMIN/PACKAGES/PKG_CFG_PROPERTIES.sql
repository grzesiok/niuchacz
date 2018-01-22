create or replace package pkg_cfg_properties authid definer as
  procedure p_set_properties(p_key varchar2, p_value varchar2);
  function f_get_properties(p_key varchar2) return varchar2 deterministic;
end;
/