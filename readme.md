TODO: Maybe add udev rules to guide, not sure if needed
Note: First install update.zip for current version

# Introduction
This is a guide to building a remote control for e-book readers by Tolino (and possibly Kobo). 
The purpose of the remote control is to flip to the next(previous) page without requiring us to actually physically touch the screen of the Tolino which requires leaving the comfort of the blanket and is therefore unacceptable.
The guide consists of three parts:
1. Gaining root access to the Tolino so we can run our remote control client on it
2. Writing the remote control client and getting it to run on the Tolino
3. Building the actual remote control using an ESP32, and a 3D-printed case.


# Rooting the Tolino
Note: This guide is just a combination of the following two guides by "roms3700" and cweiske who both describe the process in detail. See [References](#references)
This has been tested on the models
1. Shine 2 HD
2. Shine 4 HD
but might very well work for other Tolino models as well.

To root the Tolino, we use a .zip file of an official Tolino firmware update and modify it according to our needs. 
The basic idea is to modify the recovery image, which is the image that is loaded into memory when we boot the Tolino in recovery mode. We will modify the recovery image in a way that gives us not only adb access to the Tolino but also gives us root rights in any adb session. Then, we will boot into the recovery image to create a backup of the current system, to make sure that none of our changes brick the Tolino. Afterwards, we use the recovery image to flash a modified boot image, which is the image that is loaded during a normal boot. The modified boot image will contain very similar changes to the recovery image which allow us to start a adb session with root rights during a normal boot of the tolino.

## Extracting the recovery image
At first, we will fix the recovery image to allow us to adb into the tolino when it is in recovery mode.
If you haven't done so already, make sure you install the latest update on the Tolino.
If the Tolino is up to date then download the latest update zip for your tolino from the official website and unzip the file via 
```
$ unzip update.zip
```
It should contain a file `recovery.img`, which we will now extract using
  
```
$ abootimg -x recovery.img
```

this gives us 
```
initrd.img
zImage
bootimg.cfg
```

Now we extract the `initrd.img` in a subfolder in the following way:
```
$ mkdir initrd
$ cd initrd
$ zcat ../initrd.img | cpio -vid
```
  
## default.prop
In the file `default.prop`, we should find the following three lines (not necessarily consecutive)
```
ro.debuggable=0
ro.secure=1
persist.sys.usb.config=mass_storage
```
we will change these to contain:
```
ro.debuggable=1 
ro.secure=0 
persist.sys.usb.config=mass_storage,adb 
```

## init.rc
In `init.rc`, we want to
1. not mount any of the file systems, so that we can create a backup of it
2. enable usb mass storage for writing the backup
3. disable automatically starting the recovery software since we will do everything over usb anyways (this is probably not needed)

For this we'll comment out the following lines:

```
service recovery /sbin/recovery
```

```
mount vfat /dev/block/mmcblk0p4 /sdcard
mount ext4 /dev/block/mmcblk0p7 /data
mount ext4 /dev/block/mmcblk0p6 /cache
```
  
and replace the line 
```
write /sys/class/android_usb/android0/enable 1
```
with the following
```
write /sys/class/android_usb/android0/enable 0
write /sys/class/android_usb/android0/idVendor 18D1
write /sys/class/android_usb/android0/idProduct D001
write /sys/class/android_usb/android0/functions mass_storage,adb
write /sys/class/android_usb/android0/iManufacturer $ro.product.manufacturer
write /sys/class/android_usb/android0/iProduct $ro.product.model
write /sys/class/android_usb/android0/iSerial $ro.serialno
write /sys/class/android_usb/android0/enable 1
```

## adbd and ALLOW_ADBD_ROOT

Now, we have to make sure that adbd does actually give us root. In newer versions of the tolino firmware, adbd was not compiled with the `ALLOW_ADBD_ROOT` flag enabled. In order to change this, we could try to find the appropriate version of adbd and recompile with this flag enabled, but we don't know which other compile flags were enabled during the compilation, so instead we are simply going to patch a few lines in the binary directly. This will effectively give the same result as having enabled ALLOW_ADBD_ROOT during compilation.

To do this we replace the file `sbin/adbd` with the adbd file in this repository.

## Re-packing the image
  
Next, we'll re-pack this into initrd_adb_enabled.img with the command
```
$ find . | cpio --create --format='newc' | gzip > ../initrd_adb_enabled.img
```

Now, we'll go the folder above via
```
$ cd ..
```
and attempt rezipping this into recovery_adb_enabled.img via
```
$ abootimg --create recovery_adb_enabled.img -f bootimg.cfg -k zImage -r initrd_adb_enabled.img
```
  
If this fails with the error message "updated is too big for the Boot Image (XXX vs YYY bytes)" then we will replace the following line in bootimg.cfg
```
bootsize = 0xAAA
```
with
```
bootsize = 0xBBB
```

where 0xBBB is the hexadecimal representation of XXX. 

Then we attempt repackaging again with
```
$ abootimg --create recovery_adb_enabled.img -f bootimg.cfg -k zImage -r initrd_adb_enabled.img
```

## Booting the recovery image
Now, we should be able to run this recovery image via fastboot on the tolino. Connect the tolino via usb to the computer. Then power off the tolino. Now press and hold the light button and then press and hold the power button as well until the LED of the home buttons turns on, off and then on again. Now we can release the buttons. Now the tolino should boot into fastboot where nothing should show on the display. 

When we run
```
$ lsusb
```
in the shell, the tolino should show up as something like
```
Bus xxx Device yyy: ID 18d1:0d02 Google Inc. Celkon A88.
```
  
The command
```
$ dmesg
```
should contain lines similar to the following:
```
[12052.793349] usb 3-5: new high-speed USB device number 15 using xhci_hcd
[12052.958318] usb 3-5: New USB device found, idVendor=18d1, idProduct=0d02
[12052.958320] usb 3-5: New USB device strings: Mfr=1, Product=2, SerialNumber=3
[12052.958322] usb 3-5: Product: i.mx6sl NTX Smart Device
[12052.958323] usb 3-5: Manufacturer: Freescale
[12052.958324] usb 3-5: SerialNumber: XXXXX.
```

Now we should be able to run fastboot commands on the tolino.
```
$ fastboot devices
```
should give the result
```
XXXXX     Android fastboot
```

Note: If this fails because of insufficient permissions or gives the result " < waiting for any device >", try running all fastboot commands as root instead.

Now, our goal is to boot the recovery image via fastboot.
```
$ fastboot boot recovery_adb_enabled.img
```
  
which should yield something like 
```
fastboot boot recovery_adb_enabled.img
downloading 'boot.img'...
OKAY [  0.217s]
booting...
OKAY [  0.003s]
finished. total time: 0.220s
```

and the tolino should the tolino logo. Again, we can find the tolino via lsusb and dmesg.

Now we should see the tolino as an adb devices via. 
```
$ adb devices
```

## Confirm we are root
Now, we should be able to enter an adb shell with
```
$ adb shell
```
which should land us on the Tolino. 
On some systems, this might not work and fail with "insuficcient permissions". Here, it might help to restart the adb server as root (see [stackoverflow post](https://stackoverflow.com/questions/28704636/insufficient-permissions-for-device-in-android-studio-workspace-running-in-opens)):
```
$ adb kill-server
$ sudo adb start-server
```

Once we succesfully entered the adb shell,
```
# busybox id
```
should give something like
```
uid=0 gid=0
```
confirming that we are root (also the `#` in the prompt gives it away).

## Creating a backup image

Now we will create a backup image of the current tolino to make sure we can always return to the current state in case we mess something up during the next steps.
To do this we can first confirm that none of the storage is mounted by running
```
# busybox mount
```
in the adb shell. Here, no entry like
```
/dev/block/mmc*
```
should show up.

If this works, we now run the command (again in the adb shell)
```
# echo "/dev/block/mmcblk0" > /sys/devices/platform/fsl-usb2-udc/gadget/lun0/file
```
which ensures that the device makes the entire memory available for us to make a backup of and not just the "tolino" partition which is intended for the user.
Now we exit the adb shell with
```
# exit
```
If your computer shows a dialog that some new device is found do NOT mount it. Otherwise we wont be able to make a backup.
Now we can obtain the device name of the tolino system with
```
$ dmesg
```
which should give a result like
```
[46502.181284] sd 7:0:0:0: [sdf] 15269888 512-byte logical blocks: (7.82 GB/7.28 GiB)
[46502.185237] sd 7:0:0:0: [sdf] Write cache: enabled, read cache: enabled, doesn't support DPO or FUA
[46502.208577]  sdX: sdX1 sdX2 sdX3 < sdX5 sdX6 sdX7 sdX8 sdX9 sdX10 > sdX4
```
  
Here `X` stands for some letter that will change depending on your system.
To confirm one last time that the tolino is not mounted, we run
```
$ mount
```
and check that no entry with our device name (`/dev/sdX` with `X` replaced with the letter in your case) shows up.

Now we can create the backup using `dd`. Be mindful of the fact that `dd` will do whatever you tell it to with no regards for anything so be careful that the following command is entered correctly. The command to create the backup is
```
$ dd if=/dev/sdX of=nameOfYourBackup.bin bs=2M
```
Make sure to replace X with the device name in your case. This command will probably take a while because we are creating a backup of the entire device.

When the backup is finished, we can reverse what we did earlier by entering an adb shell with
```
$ adb shell
```
And then running
```
# echo "" > /sys/devices/platform/fsl-usb2-udc/gadget/lun0/file
```

## Obtain root in normal boot
Now we have succesfully created a backup using the modified recovery image.
However, currently we only have root access to our tolino in recovery mode, not during a normal mode. To also gain root access here, we have to modify the boot image in the same way as the recovery image. We will also make sure that our remote control script is started automatically in the background on every boot. So we unpack the image as before and then change the same lines as before in default.prop:
```
ro.debuggable=0
ro.secure=1
persist.sys.usb.config=mass_storage
```
to these:
```
ro.debuggable=1 
ro.secure=0 
persist.sys.usb.config=mass_storage,adb 
```

In `init.rc`, we add 
```
service start_remote /system/bin/sh /system/usr/remoteControl/remoteClient.sh
    class remote

on property:service.bootanim.exit=1
    start start_remote
```

And, again, we replace the `/sbin/adbd` file with the patched adbd file from the repository.

Now, we pack the image again the same way as before (adjusting the size in bootimg.cfg if needed).
To confirm that our newly created boot image works properly, we can fastboot into it as we did with the recovery image. If it boots succesfully and we also have root access via adb, we can flash it permanently onto the tolino.
To do this, we fastboot the recovery image again
Then we copy the image to the reader via
```
$ adb push boot_adb_enabled.img /tmp
```
and then enter an
```
$ adb shell
```
where we run
```
# busybox dd if=/tmp/boot_adb_enabled.img of=/dev/block/mmcblk0p1 bs=2M
# busybox sync && busybox sleep 5 && busybox sync
```
where the last command is just needed to ensure that what we just copied is written properly to the memory.
Now we should be able to simply reboot the tolino by holding down the power button for a while. If everything worked, we should now have root access via adb in a normal boot. You are now officially allowed to call your self Martin Rooter King.

## References
1. Guide to root the Tolino by roms3700 (German): https://www.e-reader-forum.de/t/tolino-vision-2-rooten.147429/
2. Guide to patch adbd to fix the ALLOW_ADBD_ROOT problem by cweiske: http://cweiske.de/tagebuch/android-root-adb.htm

# Writing the remote control script
<!-- Arguably the simplest way to go forward/backward by one page is to simply emulate a touch screen event in the right/left side of the screen of the Tolino. Thankfully, android makes this extremely easy with the  -->
## Netcat
The remote control script consists of a TCP client that listens for messages from the remote control. In the event of an incoming message, a script which flips the page forward or backward (depending on the incoming message) is run.
In order to run the TCP server we use the [Open-BSD version of netcat](https://man.openbsd.org/nc.1), which allows running the server even when detached from standard input.
This guide contains a precompiled binary for this version of netcat that should run on the Tolino. If this doesn't work for versions of the Tolino other than the ones with which this guide was tested, it might be necessary to use the android NDK to compile netcat for the appropriate android version of your Tolino.

## Installation
In order to install the remote script, we first make sure we can write to `/system/usr` by mounting in writeable mode via
```
$ adb shell
# mount -o rw,remount /dev/block/mmcblk0p5 /system
```
Now, in a shell on our local machine, we simply push the folder `remoteControl` in this repo to `/system/usr`:

```
$ adb push remoteControl/. /system/usr/remoteControl
```

In the still open adb shell we mount the file system in readonly again:
```
# mount -o ro,remount /dev/block/mmcblk0p5 /system
```

## Testing the remote control script (optional)
Properly using the remote control script we just copied to the Tolino would require actually building the remote control which is done in part 3 of this guide :). Here, we want to test whether our script works without having the physical remote control present. First, we reboot the Tolino, to make sure that the service we defined in the `init.rc` in our boot.img is actually run. Once we have rebooted, we join a WiFi that our local machine is in with the Tolino. 
Now we need to find out the IP address of our Tolino. If you can't check the router or use nmap, one solution for this is to find it out via adb - connect the tolino via USB and run
```
$ adb shell
# busybox ifconfig
```

Once we have the IP adress we should be able to open a book on the tolino and run
```
$ netcat <IP> 5000
```
This should establish a connection with the remote control script running on the tolino. Entering anything (and pressing enter afterwards) should flip to the next page, entering "back" and pressing enter should flip to the previous page.

# Building the actual remote control
TODO
