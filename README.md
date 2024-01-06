## Why?? Why would anyone put time and effort into this?

I learned about coreboot around when I started a university course in electrical engineering. I was interested, fascinated by computer architecture in general, and wanted to put coreboot onto something to learn and play or so. So I got an ASUS A8V-E Deluxe for 10 Euros and gave it a try.

## 2013: 

There was a short mailing list thread about my attempts to use registered ECC RAM
https://mail.coreboot.org/pipermail/coreboot/2013-October/076517.html
I had no time to dig deeper back then, and I probably lacked some skills for tackling this kind of issue.

## 10 years, December 2023 - January 2024

I wanted to finish this.

Rudolf was right, DDR1 setup is manageable.

The issues were:
* NOT the SPD I2C address to DIMM map, it was OK to use
* a setting for chip selects in dual channel mode (128 bit mode) was not made for the case of registered RAM on socket 939, resulting garbage data on one half of the 128 bit interface. Registered DIMMs are not officially supported on this socket, but they work.
* there is a bug when coreboot enables ECC; "fixed" by not enabling it in coreboot, but later in Linux

Result:

* dual core working with the VIA K8T890 (vendor BIOS doesn't enable both cores)
* 4x1GB reg ECC working at DDR400 CL3 (vendor BIOS might even work with this, untested)

Remaining issues:

* CMOS configuration is broken
* on rare occasions, coreboot fails somewhere in ramstage with PCI bus 5 or so
* on rare occasions, Linux fails to boot with random broken behavior
* system is not 100% stable
* suspend to RAM (S3) untested and probably broken; between 4.6 and 4.8 the S3 state was added to the ACPI tables so maybe it can work

## coreboot build setup in 2024

I decided to try setting up a chroot with an older Ubuntu release. Using schroot makes the usage of the chroot a bit nicer.
https://help.ubuntu.com/community/DebootstrapChroot

The /home folder is bind-mounted into the chroot.
The schroot config must have 'type=directory' or the mounts won't be done.

This is in my schroot.conf:
```
[ubuntu_focal]                       
description=Ubuntu Focal    
directory=/srv/chroot/ubuntu_focal
type=directory    
users=michael
groups=default
root-groups=root                     
aliases=focal,default
```

as root, here on Gentoo:
```
emerge -av debootstrap schroot
vim /etc/schroot/schroot.conf
mkdir -p /srv/chroot/ubuntu_focal
debootstrap --arch=i386 --variant=buildd focal /srv/chroot/ubuntu_focal http://archive.ubuntu.com/ubuntu/
schroot -c focal
# from coreboot doc
apt-get install -y bison build-essential curl flex git gnat libncurses5-dev m4 zlib1g-dev
# make it work
apt install python3
apt install python-is-python3
```

Now in the chroot, in ~/src/coreboot-4.8 or whereever you keep the sources:
```
make crossgcc-i386 CPUS=12
make menuconfig
```

On the host OS, do `vim payloads/external/SeaBIOS/Makefile` to fix the SeaBIOS
git remote URL.

Old toolchain tarballs for 'util/crossgcc/tarballs/' are here:
https://www.coreboot.org/releases/crossgcc-sources/
