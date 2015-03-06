obj-m := analyzer.o

modules clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build C=1 M=$(CURDIR) $@

insmod rmmod:
	sudo $@ analyzer.ko

stop-udev:
	sudo udevadm control --stop-exec-queue

short-timeout:
	echo 10 | sudo tee /proc/sys/kernel/hung_task_timeout_secs > /dev/null

drop-caches:
	echo 1 | sudo tee /proc/sys/vm/drop_caches > /dev/null

watch:
	sudo less -S /var/log/kern.log

clear-log:
	true | sudo tee /var/log/kern.log
