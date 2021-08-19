#!/bin/sh
#
#	This is a simple hack to link 860 objects for our standalone environment
#	and bind them with a host launcher.
#
BIN=/Net/k9/dps/nd_proj/bin
LIB=/Net/k9/dps/nd_proj/i860/lib
CC=$BIN/cc860
LD=$BIN/ld860
MUNGE=$BIN/unixsyms
BINDOBJ=$LIB/bind860.o
CRT0=$LIB/crt0.o
LIB1=$LIB/libsa.a
LIB2=$LIB/gcc-runtime860

outfile="a.out"
args=""

while [ $# != 0 ]; do
	case $1 in
	-o)
		shift
		outfile=$1 ;;
	-debug)
		LIB1=$LIB/libsa_debug.a ;;
	*)
		args="$args $1" ;;
	esac
	shift
done

$LD -p -T f8000000 -o /tmp/bind860.$$ -e pstart $CRT0 $args $LIB1 $LIB2
$MUNGE /tmp/bind860.$$
strip /tmp/bind860.$$
cc -o $outfile $BINDOBJ -segcreate __I860 __i860 /tmp/bind860.$$  -lNeXT_s -lsys_s
/bin/rm -f /tmp/bind860.$$


