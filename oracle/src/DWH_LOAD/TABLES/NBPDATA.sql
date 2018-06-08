create table nbpdata(
  importtable varchar2(1),
  importno varchar2(20),
  importdate date,
  currency_from_code varchar2(3),
  currency_to_code varchar2(3),
  mid_rate number
)
PCTFREE 0
ROW STORE COMPRESS BASIC NOLOGGING
TABLESPACE TS_LOADER_LOAD;
