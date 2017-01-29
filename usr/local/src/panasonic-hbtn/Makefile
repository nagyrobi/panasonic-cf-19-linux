obj-m = panasonic-hbtn.o

KVERSION = $(shell uname -r)

all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
install:
	mkdir -p /lib/modules/$(KVERSION)/kernel/drivers/panasonic-hbtn/
	cp $(PWD)panasonic-hbtn.ko /lib/modules/$(KVERSION)/kernel/drivers/panasonic-hbtn/
	echo "panasonic-hbtn" > /etc/modules-load.d/panasonic-hbtn.conf
