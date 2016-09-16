#!/system/bin/sh

term="/dev/pts/* "

if [ "$1" = "-m" ]; then
	setprop ril.assert 1
	phone_count=`getprop persist.msms.phone_count`
	if [ "$phone_count" = "1" ]; then
		setprop ril.sim.power 0
	elif [ "$phone_count" = "2" ]; then
		setprop ril.sim.power 0
		setprop ril.sim.power1 0
	fi
	phone=`getprop sys.phone.app`
	kill $phone
fi
