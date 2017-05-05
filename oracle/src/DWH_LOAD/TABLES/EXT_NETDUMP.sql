--------------------------------------------------------
--  DDL for Table EXT_NETDUMP
--------------------------------------------------------

  CREATE TABLE "DWH_LOAD"."EXT_NETDUMP" 
   (	"FRAMELEN" VARCHAR2(4000), 
	"FRAMEPACKETFLAGS" VARCHAR2(4000), 
	"FRAMEPROTOCOLS" VARCHAR2(4000), 
	"FRAMETIME" VARCHAR2(4000), 
	"ETHDST" VARCHAR2(4000), 
	"ETHSRC" VARCHAR2(4000), 
	"ETHTYPE" VARCHAR2(4000), 
	"IPCHECKSUM" VARCHAR2(4000), 
	"IPDST" VARCHAR2(4000), 
	"IPFLAGSDF" VARCHAR2(4000), 
	"IPFLAGSMF" VARCHAR2(4000), 
	"IPFLAGSRB" VARCHAR2(4000), 
	"IPFLAGSSF" VARCHAR2(4000), 
	"IPLEN" VARCHAR2(4000), 
	"IPPROTO" VARCHAR2(4000), 
	"IPSRC" VARCHAR2(4000), 
	"IPTOS" VARCHAR2(4000), 
	"IPTTL" VARCHAR2(4000), 
	"IPVERSION" VARCHAR2(4000), 
	"ICMPCHECKSUM" VARCHAR2(4000), 
	"ICMPCODE" VARCHAR2(4000), 
	"ICMPEXT" VARCHAR2(4000), 
	"ICMPEXTCHECKSUM" VARCHAR2(4000), 
	"ICMPEXTCLASS" VARCHAR2(4000), 
	"ICMPEXTCTYPE" VARCHAR2(4000), 
	"ICMPEXTDATA" VARCHAR2(4000), 
	"ICMPEXTLENGTH" VARCHAR2(4000), 
	"ICMPEXTRES" VARCHAR2(4000), 
	"ICMPEXTVERSION" VARCHAR2(4000), 
	"ICMPLENGTH" VARCHAR2(4000), 
	"ICMPLIFETIME" VARCHAR2(4000), 
	"ICMPMTU" VARCHAR2(4000), 
	"ICMPNORESP" VARCHAR2(4000), 
	"ICMPNUMADDRS" VARCHAR2(4000), 
	"ICMPPOINTER" VARCHAR2(4000), 
	"ICMPPREFLEVEL" VARCHAR2(4000), 
	"ICMPRECEIVETIMESTAMP" VARCHAR2(4000), 
	"ICMPREDIRGW" VARCHAR2(4000), 
	"ICMPRESERVED" VARCHAR2(4000), 
	"ICMPRESPTIME" VARCHAR2(4000), 
	"ICMPSEQ" VARCHAR2(4000), 
	"ICMPSEQLE" VARCHAR2(4000), 
	"ICMPTRANSMITTIMESTAMP" VARCHAR2(4000), 
	"ICMPTYPE" VARCHAR2(4000), 
	"TCPACK" VARCHAR2(4000), 
	"TCPCHECKSUM" VARCHAR2(4000), 
	"TCPDSTPORT" VARCHAR2(4000), 
	"TCPFLAGSACK" VARCHAR2(4000), 
	"TCPFLAGSCWR" VARCHAR2(4000), 
	"TCPFLAGSECN" VARCHAR2(4000), 
	"TCPFLAGSFIN" VARCHAR2(4000), 
	"TCPFLAGSNS" VARCHAR2(4000), 
	"TCPFLAGSPUSH" VARCHAR2(4000), 
	"TCPFLAGSRES" VARCHAR2(4000), 
	"TCPFLAGSRESET" VARCHAR2(4000), 
	"TCPFLAGSSYN" VARCHAR2(4000), 
	"TCPFLAGSURG" VARCHAR2(4000), 
	"TCPLEN" VARCHAR2(4000), 
	"TCPOPTIONS" VARCHAR2(4000), 
	"TCPSEQ" VARCHAR2(4000), 
	"TCPNXTSEQ" VARCHAR2(4000), 
	"TCPSRCPORT" VARCHAR2(4000), 
	"TCPWINDOWSIZE" VARCHAR2(4000), 
	"UDPCHECKSUM" VARCHAR2(4000), 
	"UDPDSTPORT" VARCHAR2(4000), 
	"UDPLENGTH" VARCHAR2(4000), 
	"UDPSRCPORT" VARCHAR2(4000), 
	"DNSLENGTH" VARCHAR2(4000), 
	"DNSOPENPGPKEY" VARCHAR2(4000), 
	"DNSOPT" VARCHAR2(4000), 
	"DNSQRYCLASS" VARCHAR2(4000), 
	"DNSQRYNAME" VARCHAR2(4000), 
	"DNSQRYNAMELEN" VARCHAR2(4000), 
	"DNSQRYQU" VARCHAR2(4000), 
	"DNSQRYTYPE" VARCHAR2(4000)
   ) 
   ORGANIZATION EXTERNAL 
    ( TYPE ORACLE_LOADER
      DEFAULT DIRECTORY "DWH_NETDUMPS_DIR"
      ACCESS PARAMETERS
      ( RECORDS DELIMITED BY newline
    SKIP 1
    FIELDS CSV WITH EMBEDDED TERMINATED BY ',' OPTIONALLY ENCLOSED BY '"' MISSING FIELD VALUES ARE NULL
    (
     framelen CHAR(4000),
     framepacketflags CHAR(4000),
     frameprotocols CHAR(4000),
     frametime CHAR(4000) TERMINATED BY "," OPTIONALLY ENCLOSED BY '""',
     ethdst CHAR(4000),
     ethsrc CHAR(4000),
     ethtype CHAR(4000),
     ipchecksum CHAR(4000),
     ipdst CHAR(4000),
     ipflagsdf CHAR(4000),
     ipflagsmf CHAR(4000),
     ipflagsrb CHAR(4000),
     ipflagssf CHAR(4000),
     iplen CHAR(4000),
     ipproto CHAR(4000),
     ipsrc CHAR(4000),
     iptos CHAR(4000),
     ipttl CHAR(4000),
     ipversion CHAR(4000),
     icmpchecksum CHAR(4000),
     icmpcode CHAR(4000),
     icmpext CHAR(4000),
     icmpextchecksum CHAR(4000),
     icmpextclass CHAR(4000),
     icmpextctype CHAR(4000),
     icmpextdata CHAR(4000),
     icmpextlength CHAR(4000),
     icmpextres CHAR(4000),
     icmpextversion CHAR(4000),
     icmplength CHAR(4000),
     icmplifetime CHAR(4000),
     icmpmtu CHAR(4000),
     icmpnoresp CHAR(4000),
     icmpnumaddrs CHAR(4000),
     icmppointer CHAR(4000),
     icmppreflevel CHAR(4000),
     icmpreceivetimestamp CHAR(4000),
     icmpredirgw CHAR(4000),
     icmpreserved CHAR(4000),
     icmpresptime CHAR(4000),
     icmpseq CHAR(4000),
     icmpseqle CHAR(4000),
     ICMPTRANSMITTIMESTAMP char(4000), 
     icmptype CHAR(4000),
     tcpack CHAR(4000),
     tcpchecksum CHAR(4000),
     tcpdstport CHAR(4000),
     tcpflagsack CHAR(4000),
     tcpflagscwr CHAR(4000),
     tcpflagsecn CHAR(4000),
     tcpflagsfin CHAR(4000),
     tcpflagsns CHAR(4000),
     tcpflagspush CHAR(4000),
     tcpflagsres CHAR(4000),
     tcpflagsreset CHAR(4000),
     tcpflagssyn CHAR(4000),
     tcpflagsurg CHAR(4000),
     tcplen CHAR(4000),
     tcpoptions CHAR(4000),
     tcpseq CHAR(4000),
     tcpnxtseq CHAR(4000),
     tcpsrcport CHAR(4000),
     tcpwindowsize CHAR(4000),
     udpchecksum CHAR(4000),
     udpdstport CHAR(4000),
     udplength CHAR(4000),
     udpsrcport CHAR(4000),
     dnslength CHAR(4000),
     dnsopenpgpkey CHAR(4000),
     dnsopt CHAR(4000),
     dnsqryclass CHAR(4000),
     dnsqryname CHAR(4000),
     dnsqrynamelen CHAR(4000),
     dnsqryqu CHAR(4000),
     dnsqrytype CHAR(4000)
    )
          )
      LOCATION
       ( 'netdump20170505073655.csv'
       )
    )
   REJECT LIMIT 1 ;
