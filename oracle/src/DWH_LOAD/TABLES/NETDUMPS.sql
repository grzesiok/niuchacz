--------------------------------------------------------
--  DDL for Table NETDUMPS
--------------------------------------------------------

CREATE TABLE NETDUMPS(
	"ID" NUMBER GENERATED BY DEFAULT AS IDENTITY MINVALUE 1 MAXVALUE 9999999999999999999999999999 INCREMENT BY 1 START WITH 1 CACHE 10000 NOORDER  NOCYCLE  NOKEEP , 
	"FILENAME" VARCHAR2(4000), 
	"FRAMELEN" NUMBER, 
	"FRAMEPACKETFLAGS" VARCHAR2(4000), 
	"FRAMEPROTOCOLS" VARCHAR2(4000), 
	"FRAMETIME" TIMESTAMP (6), 
	"ETHDST" VARCHAR2(17), 
	"ETHSRC" VARCHAR2(17), 
	"ETHTYPE" VARCHAR2(10), 
	"IPCHECKSUM" RAW(4), 
	"IPDST" VARCHAR2(15), 
	"IPFLAGSDF" VARCHAR2(1), 
	"IPFLAGSMF" VARCHAR2(1), 
	"IPFLAGSRB" VARCHAR2(1), 
	"IPFLAGSSF" VARCHAR2(1), 
	"IPLEN" NUMBER, 
	"IPPROTO" NUMBER, 
	"IPSRC" VARCHAR2(15), 
	"IPTOS" NUMBER, 
	"IPTTL" NUMBER, 
	"IPVERSION" NUMBER, 
	"ICMPCHECKSUM" RAW(4), 
	"ICMPCODE" NUMBER, 
	"ICMPEXT" VARCHAR2(4000), 
	"ICMPEXTCHECKSUM" RAW(4), 
	"ICMPEXTCLASS" NUMBER, 
	"ICMPEXTCTYPE" NUMBER, 
	"ICMPEXTDATA" VARCHAR2(4000), 
	"ICMPEXTLENGTH" NUMBER, 
	"ICMPEXTRES" NUMBER, 
	"ICMPEXTVERSION" NUMBER, 
	"ICMPLENGTH" NUMBER, 
	"ICMPLIFETIME" NUMBER, 
	"ICMPMTU" NUMBER, 
	"ICMPNORESP" VARCHAR2(4000), 
	"ICMPNUMADDRS" NUMBER, 
	"ICMPPOINTER" NUMBER, 
	"ICMPPREFLEVEL" NUMBER, 
	"ICMPRECEIVETIMESTAMP" VARCHAR2(4000), 
	"ICMPREDIRGW" VARCHAR2(15), 
	"ICMPRESERVED" VARCHAR2(4000), 
	"ICMPRESPTIME" VARCHAR2(4000), 
	"ICMPSEQ" NUMBER, 
	"ICMPSEQLE" NUMBER, 
	"ICMPTRANSMITTIMESTAMP" VARCHAR2(4000), 
	"ICMPTYPE" NUMBER, 
	"TCPACK" NUMBER, 
	"TCPCHECKSUM" RAW(4), 
	"TCPDSTPORT" VARCHAR2(4000), 
	"TCPFLAGSACK" VARCHAR2(1), 
	"TCPFLAGSCWR" VARCHAR2(1), 
	"TCPFLAGSECN" VARCHAR2(1), 
	"TCPFLAGSFIN" VARCHAR2(1), 
	"TCPFLAGSNS" VARCHAR2(1), 
	"TCPFLAGSPUSH" VARCHAR2(1), 
	"TCPFLAGSRES" VARCHAR2(1), 
	"TCPFLAGSRESET" VARCHAR2(1), 
	"TCPFLAGSSYN" VARCHAR2(1), 
	"TCPFLAGSURG" VARCHAR2(1), 
	"TCPLEN" NUMBER, 
	"TCPOPTIONS" VARCHAR2(4000), 
	"TCPSEQ" NUMBER, 
	"TCPNXTSEQ" NUMBER, 
	"TCPSRCPORT" NUMBER, 
	"TCPWINDOWSIZE" NUMBER, 
	"UDPCHECKSUM" RAW(4), 
	"UDPDSTPORT" NUMBER, 
	"UDPLENGTH" NUMBER, 
	"UDPSRCPORT" NUMBER, 
	"DNSLENGTH" NUMBER, 
	"DNSOPENPGPKEY" VARCHAR2(4000), 
	"DNSOPT" VARCHAR2(4000), 
	"DNSQRYCLASS" VARCHAR2(4000), 
	"DNSQRYNAME" VARCHAR2(4000), 
	"DNSQRYNAMELEN" NUMBER, 
	"DNSQRYQU" VARCHAR2(4000), 
	"DNSQRYTYPE" VARCHAR2(4000)
)
PCTFREE 0
ROW STORE COMPRESS BASIC NOLOGGING
TABLESPACE TS_LOADER_LOAD
PARTITION BY RANGE (frametime) INTERVAL (NUMTODSINTERVAL(1,'DAY'))
(PARTITION netdumps_part_1 values LESS THAN (TO_DATE('01-05-2017','DD-MM-YYYY')));

exec dbms_stats.SET_TABLE_PREFS('DWH_LOAD','NETDUMPS','INCREMENTAL','TRUE');
exec dbms_stats.SET_TABLE_PREFS('DWH_LOAD','NETDUMPS','INCREMENTAL_STALENESS','USE_STALE_PERCENT');
exec dbms_stats.SET_TABLE_PREFS('DWH_LOAD','NETDUMPS','APPROXIMATE_NDV_ALGORITHM','HYPERLOGLOG');