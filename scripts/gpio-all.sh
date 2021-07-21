#!/bin/bash

gpiochips=(906)
idx=0
gpio_list=()

gpio_list=(906 907 908)

ngpio=`cat /sys/class/gpio/gpiochip906/ngpio`
label=`cat /sys/class/gpio/gpiochip906/label`
beg=0
end=$((beg + ngpio))
linux_numbers=()

echo GPIO_$beg -- GPIO_$end "	" ${gpiochips[idx]} -- $((gpiochips[idx]+ngpio-1)) "	" $ngpio

for ((i=beg; i < end; ++i)); do
	if ((beg <= $i && $i < end)); then
	    linux_numbers[i]=$((i - beg + gpiochips[idx]))
	    echo "	[$i] GPIO_$i" " ==> " gpio${linux_numbers[i]} " " $i
	fi
done

for i in "${linux_numbers[@]}"; do
	echo $i | sudo tee -a "/sys/class/gpio/export"
	if [ -e "/sys/class/gpio/gpio${i}" ]; then
		echo "out" | sudo tee -a "/sys/class/gpio/gpio${i}/direction"
		echo 1 | sudo tee -a "/sys/class/gpio/gpio${i}/value"
	fi
	read -p "ok ?" yn
#	case "$yn" in [yY]*) ;; *) echo "abort." ; exit ;; esac
done
