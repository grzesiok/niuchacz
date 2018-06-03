insert into download_files_def(url, user_name, last_download, next_download_offset) values ('"http://api.nbp.pl/api/exchangerates/tables/a/"yyyy-mm-dd"/?format=json"', 'DWH_LOAD', to_date('20020102', 'yyyymmdd'), interval '1' DAY);
insert into download_files_def(url, user_name, last_download, next_download_offset) values ('"http://api.nbp.pl/api/exchangerates/tables/b/"yyyy-mm-dd"/?format=json"', 'DWH_LOAD', to_date('20020102', 'yyyymmdd'), interval '1' DAY);
insert into download_files_def(url, user_name, last_download, next_download_offset) values ('"http://api.nbp.pl/api/exchangerates/tables/c/"yyyy-mm-dd"/?format=json"', 'DWH_LOAD', to_date('20020102', 'yyyymmdd'), interval '1' DAY);
commit;
