--------------------------------------------------------
--  DDL for Table EXT_OSHA_INSPECTIONS
--------------------------------------------------------

  CREATE TABLE "EXT_OSHA_INSPECTIONS" 
   (	"ACTIVITY_NR" VARCHAR2(4000), 
	"REPORTING_ID" VARCHAR2(4000), 
	"STATE_FLAG" VARCHAR2(4000), 
	"ESTAB_NAME" VARCHAR2(4000), 
	"SITE_ADDRESS" VARCHAR2(4000), 
	"SITE_CITY" VARCHAR2(4000), 
	"SITE_STATE" VARCHAR2(4000), 
	"SITE_ZIP" VARCHAR2(4000), 
	"OWNER_TYPE" VARCHAR2(4000), 
	"OWNER_CODE" VARCHAR2(4000), 
	"ADV_NOTICE" VARCHAR2(4000), 
	"SAFETY_HLTH" VARCHAR2(4000), 
	"SIC_CODE" VARCHAR2(4000), 
	"NAICS_CODE" VARCHAR2(4000), 
	"INSP_TYPE" VARCHAR2(4000), 
	"INSP_SCOPE" VARCHAR2(4000), 
	"WHY_NO_INSP" VARCHAR2(4000), 
	"UNION_STATUS" VARCHAR2(4000), 
	"SAFETY_MANUF" VARCHAR2(4000), 
	"SAFETY_CONST" VARCHAR2(4000), 
	"SAFETY_MARIT" VARCHAR2(4000), 
	"HEALTH_MANUF" VARCHAR2(4000), 
	"HEALTH_CONST" VARCHAR2(4000), 
	"HEALTH_MARIT" VARCHAR2(4000), 
	"MIGRANT" VARCHAR2(4000), 
	"MAIL_STREET" VARCHAR2(4000), 
	"MAIL_CITY" VARCHAR2(4000), 
	"MAIL_STATE" VARCHAR2(4000), 
	"MAIL_ZIP" VARCHAR2(4000), 
	"HOST_EST_KEY" VARCHAR2(4000), 
	"NR_IN_ESTAB" VARCHAR2(4000), 
	"OPEN_DATE" VARCHAR2(4000), 
	"CASE_MOD_DATE" VARCHAR2(4000), 
	"CLOSE_CONF_DATE" VARCHAR2(4000), 
	"CLOSE_CASE_DATE" VARCHAR2(4000), 
	"LD_DT" VARCHAR2(4000)
   ) 
   ORGANIZATION EXTERNAL 
    ( TYPE ORACLE_LOADER
      DEFAULT DIRECTORY "DWH_OSHA_DIR"
      ACCESS PARAMETERS
      ( RECORDS DELIMITED BY newline
    SKIP 1
    FIELDS CSV WITH EMBEDDED TERMINATED BY ',' OPTIONALLY ENCLOSED BY '"' MISSING FIELD VALUES ARE NULL
    (
     activity_nr CHAR(4000),
     reporting_id CHAR(4000),
     state_flag CHAR(4000),
     estab_name CHAR(4000),
     site_address CHAR(4000),
     site_city CHAR(4000),
     site_state CHAR(4000),
     site_zip CHAR(4000),
     owner_type CHAR(4000),
     owner_code CHAR(4000),
     adv_notice CHAR(4000),
     safety_hlth CHAR(4000),
     sic_code CHAR(4000),
     naics_code CHAR(4000),
     insp_type CHAR(4000),
     insp_scope CHAR(4000),
     why_no_insp CHAR(4000),
     union_status CHAR(4000),
     safety_manuf CHAR(4000),
     safety_const CHAR(4000),
     safety_marit CHAR(4000),
     health_manuf CHAR(4000),
     health_const CHAR(4000),
     health_marit CHAR(4000),
     migrant CHAR(4000),
     mail_street CHAR(4000),
     mail_city CHAR(4000),
     mail_state CHAR(4000),
     mail_zip CHAR(4000),
     host_est_key CHAR(4000),
     nr_in_estab CHAR(4000),
     open_date CHAR(4000),
     case_mod_date CHAR(4000),
     close_conf_date CHAR(4000),
     close_case_date CHAR(4000),
     ld_dt CHAR(4000)
    )
              )
      LOCATION
       ( 'osha_inspection-5.csv'
       )
    )
   REJECT LIMIT 1 ;
