#!/bin/bash

FILENAME=/root/netstatdumps/netstatdump$(date +"%Y%m%d%k%M%S").csv

netstat -anput | awk -v date="$(date +"%Y-%m-%d %r")" '{if($1=="tcp"){print date","$1","$4","$5","$7} else if($1=="udp"){print date","$1","$4","$5","$6}}' >> $FILENAME

