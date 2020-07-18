#!/bin/sh
rproc_class_dir="/sys/class/remoteproc/remoteproc0/"
fmw_dir="/lib/firmware"
project_name=$(basename $(pwd))
fmw_name=stm32mp157c-rpmsg-netlink.elf
echo "fw_cortex_m4.sh: fmw_name=${fmw_name}"
rproc_state=`tr -d '\0' < $rproc_class_dir/state`

error() {
	echo -e "$1"
	exit 0
}


case $1 in
	start) ;;
	stop) ;;
	*) echo "`basename ${0}`:usage: start | stop"
	   exit 1
	   ;;
esac

#################
# Start example #
#################
if [ $1 == "start" ]
then

if [ $rproc_state == "running" ]
then
echo "Stopping running fw ..."
echo stop > $rproc_class_dir/state
fi

# load and start firmware
echo $fmw_name > $rproc_class_dir/firmware
echo start > $rproc_class_dir/state

fi


################
# Stop example #
################
if [ $1 == "stop" ]
then

if [ $rproc_state == "offline" ]
then
echo "Nothing to do, no M4 fw is running"

else
echo stop > $rproc_class_dir/state

fi
fi
