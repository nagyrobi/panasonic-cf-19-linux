# panasonic-hbtn
Panasonic CF-18/19 (Toughbook) Tablet Button driver for Linux
Modified to send "XF86MenuKB" keycode for the keyboard button, and "XF86DOS" for the screen rotation button.
See in usr/local/bin scripts to use with these.

## Usage
```bash
cd /usr/local/src/panasonic-hbtn/

make all
make install

depmod -a
modprobe panasonic-hbtn
```
Then try out if those buttons are in working order.

Finally,
```bash
update-initramfs -u
```

Tested with CF-19C MK2 with Ubuntu 15.10 i386 desktop.
