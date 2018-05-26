create or replace TYPE o_download_action under o_action(
  url# varchar2(4000),
  hash_md5# raw(16),
  constructor function o_download_action(i_url varchar2, i_hash_md5 raw) return self as result,
  overriding member procedure p_exec
);
/
create or replace TYPE BODY o_download_action AS

  constructor function o_download_action(i_url varchar2, i_hash_md5 raw) return self as result AS
  BEGIN
    self.key# := 'oda';
    self.url# := i_url;
    self.hash_md5# := i_hash_md5;
    RETURN;
  END O_DOWNLOAD_ACTION;

  overriding member procedure p_exec as
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
               raise_application_error(-20000, '??', true);
      end;
    end;
  begin
    l_max_retries := pkg_cfg_properties.f_get_properties('download_retries');
    l_wait_time := pkg_cfg_properties.f_get_properties('download_retry_wait');
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
    pkg_download_internal.p_hist_insert(i_starttime =>l_start_time, i_stoptime => l_stop_time, i_url => self.url#, i_content => l_blob);
  end;

END;
/
