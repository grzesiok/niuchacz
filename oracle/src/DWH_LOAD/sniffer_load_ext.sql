insert into download_files_def(url, user_name, next_download, next_download_offset, import_action_owner, import_action_typename, activeflag)
values ('"http://api.nbp.pl/api/exchangerates/tables/a/"yyyy-mm-dd"/?format=json"', 'DWH_LOAD', to_date('20020102', 'yyyymmdd'), interval '1' DAY, 'DWH_LOAD', 'O_NBPDATA_IMPORT_ACTION', 'Y');
insert into download_files_def(url, user_name, next_download, next_download_offset, import_action_owner, import_action_typename, activeflag)
values ('"http://api.nbp.pl/api/exchangerates/tables/b/"yyyy-mm-dd"/?format=json"', 'DWH_LOAD', to_date('20020102', 'yyyymmdd'), interval '1' DAY, 'DWH_LOAD', 'O_NBPDATA_IMPORT_ACTION', 'Y');
insert into download_files_def(url, user_name, next_download, next_download_offset, import_action_owner, import_action_typename, activeflag)
values ('"http://api.nbp.pl/api/exchangerates/tables/c/"yyyy-mm-dd"/?format=json"', 'DWH_LOAD', to_date('20020102', 'yyyymmdd'), interval '1' DAY, 'DWH_LOAD', 'O_NBPDATA_IMPORT_ACTION', 'N');
commit;
