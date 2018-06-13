create or replace TYPE o_download_files_action under dwh_admin.o_action(
  url# varchar2(4000),
  hash_md5# raw(16),
  l_download_file_def_id number,
  constructor function o_download_files_action(i_download_file_def_id varchar2, i_url varchar2, i_hash_md5 raw) return self as result,
  overriding member procedure p_exec
);
/
create or replace TYPE BODY o_download_files_action AS

  constructor function o_download_files_action(i_download_file_def_id varchar2, i_url varchar2, i_hash_md5 raw) return self as result AS
  BEGIN
    self.key# := 'df_odfa';
    self.url# := i_url;
    self.hash_md5# := i_hash_md5;
    self.l_download_file_def_id := i_download_file_def_id;
    RETURN;
  END o_download_files_action;

  overriding member procedure p_exec as
    l_url_already_downloaded number;
    l_blob blob;
    l_max_retries number;
    l_wait_time number;
    l_current number := 1;
    l_hash raw(16);
    l_start_time timestamp;
    l_stop_time timestamp;

    function f_download(p_url varchar2) return blob is
      l_http_request utl_http.req;
      l_http_response utl_http.resp;
      l_raw raw(2000);
      l_blob blob;
    begin
      -- Initialize the BLOB.
      DBMS_LOB.createtemporary(l_blob, FALSE, DBMS_LOB.CALL);
      -- Make a HTTP request and get the response.
      l_http_request := UTL_HTTP.begin_request(p_url);
      BEGIN
        l_http_response := utl_http.get_response(l_http_request);
        if(l_http_response.status_code != utl_http.http_ok) then
          utl_http.end_response(l_http_response);
          raise_application_error(-20000, 'HTTP_VERSION='||l_http_response.http_version||' '||
                                          'STATUS_CODE='||l_http_response.status_code||' '||
                                          'REASON_PHRASE'||l_http_response.reason_phrase);
        end if;
        -- Copy the response into the BLOB.
        begin
          LOOP
            utl_http.read_raw(l_http_response, l_raw, 2000);
            DBMS_LOB.append(l_blob, to_blob(l_raw));
          END LOOP;
        exception
          when utl_http.end_of_body then
            utl_http.end_response(l_http_response);
        end;
        return l_blob;
      EXCEPTION
        when others
          then utl_http.end_response(l_http_response);
               dbms_lob.freetemporary(l_blob);
               raise_application_error(-20000, DBMS_UTILITY.format_error_stack, true);
      end;
    end;
    
    procedure p_enqueue_importaction(i_url varchar2, i_hash_md5 raw) as
      l_importaction_owner download_files_def.import_action_owner%type;
      l_import_action_typename download_files_def.import_action_typename%type;
      l_plsql_block varchar2(4000);
    begin
      select dfd.import_action_owner, dfd.import_action_typename
        into l_importaction_owner, l_import_action_typename
      from download_files_def dfd
      where dfd.download_file_def_id = l_download_file_def_id;
      l_plsql_block := 'declare
                          l_action  '||l_importaction_owner||'.'||l_import_action_typename||' := '||l_importaction_owner||'.'||l_import_action_typename||'(:1, :2);
                        begin
                          dwh_admin.pkg_actions.p_enqueue(i_queue_name => pkg_download_files.g_queue_importfiles,
                                                          i_recipient => :3,
                                                          i_autocommit => false,
                                                          i_action => l_action);
                        end;';
      dbms_output.put_line(l_plsql_block);
      execute immediate l_plsql_block using in i_url, in i_hash_md5, in l_importaction_owner;
    end;
  begin
    select count(*) into l_url_already_downloaded
    from download_files
    where url# = self.url#;
    if(l_url_already_downloaded > 0) then
      return;
    end if;
    l_max_retries := dwh_admin.pkg_cfg_properties.f_get_properties('download_retries');
    l_wait_time := dwh_admin.pkg_cfg_properties.f_get_properties('download_retry_wait');
    <<try_again>>
    begin
      l_start_time := systimestamp;
      l_blob := f_download(self.url#);
      l_stop_time := systimestamp;
    exception
      when utl_http.transfer_timeout then
        if(l_current < l_max_retries) then
          l_current := l_current + 1;
          dbms_lock.sleep(l_wait_time);
          goto try_again;
        end if;
    end;
    if(self.hash_md5# is not null) then
      l_hash := dbms_crypto.hash(src => l_blob, typ => dbms_crypto.hash_md5);
      if(utl_raw.compare(self.hash_md5#, l_hash) != 0) then
        raise_application_error(-20001, 'Expected hash='||rawtohex(self.hash_md5#)||' current hash='||rawtohex(l_hash));
      end if;
    end if;
    insert into download_files(start_time#, stop_time#, url#, content#)
      values (l_start_time, l_stop_time, self.url#, l_blob);
    p_enqueue_importaction(self.url#, self.hash_md5#);
  end;

END;
/
