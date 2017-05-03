--------------------------------------------------------
--  DDL for Package PKG_ACTIONS
--------------------------------------------------------

  CREATE OR REPLACE EDITIONABLE PACKAGE "DWH_ADMIN"."PKG_ACTIONS" authid definer AS 

  procedure p_enqueue(i_recipient varchar2 default sys_context('userenv', 'session_user'), i_action o_action);
  function f_dequeue(i_consumer varchar2 default sys_context('userenv', 'session_user'), i_autocommit boolean default true) return o_action;

END PKG_ACTIONS;

/
