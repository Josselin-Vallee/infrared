<code>
    
    Here's a guide on how to configure the RPi master/slave unit:

    Username      : alarm
    Password      : alarm
    Root password : root
    Master IP     : 192.168.1.13
    Slave IP      : 192.168.1.14
    
    0. Decompress and write the sdcard image to the scdard of the RPi:
    Download link is in the google docs document. md5sum: ef555083f4aab9b2d8f0774b758946d3
    Copy image to sdcard. Here sdX stands for the device name of the sdcard. You can find it by typing:
    
    $ lsblk
    $ sudo gunzip --stdout rpimaster-20160423.img.gz | sudo dd bs=4M of=/dev/sdX

    1. On both master and slave:

    - copy setup folder to user's home folder on pi (replace by pi's IP)
    (on pc) $ scp -r ./setup alarm@192.168.1.50:/home/alarm/
    (on pi) $ su
    (on pi) # cp -r /home/alarm/setup/* /
    
    2a. On master:
    - become root:
    $ su

    - change hostname in /etc/hostname to rpimaster
    # echo rpimaster > /etc/hostname
    
    - setup master static IP address
    # systemctl disable systemd-networkd.service
    # netctl enable eth0master

    - setup time synchronization master daemon:
    # systemctl disable systemd-timesyncd.service
    # systemctl daemon-reload
    # systemctl enable ptpd-master.service

    - reboot:
    # reboot

    2b. On slave:
    - become root:
    $ su

    - change hostname in /etc/hostname to rpislave
    # echo rpislave > /etc/hostname
    
    - setup slave static IP address
    # systemctl disable systemd-networkd.service
    # netctl enable eth0slave

    - setup time synchronization slave daemon:
    # systemctl disable systemd-timesyncd.service
    # systemctl daemon-reload
    # systemctl enable ptpd-slave.service

    - reboot:
    # reboot

    3. On both:

    - start virtual python environment (type deactivate to exit environment):
    $ source ~/python2/bin/activate (for python 2)
    $ source ~/python3/bin/activate (for python 3)
    
</code>
