create or replace package body pkg_cfg_properties as
  procedure p_set_properties(p_key varchar2, p_value varchar2) as
    pragma autonomous_transaction;
  begin
    update cfg_properties
      set value# = p_value
    where key# = p_key;
    commit;
  end;

  function f_get_properties(p_key varchar2) return varchar2 deterministic is
    l_value varchar2(4000);
  begin
    select value# into l_value
    from cfg_properties
    where key# = p_key;
    return l_value;
  end;
end;
/