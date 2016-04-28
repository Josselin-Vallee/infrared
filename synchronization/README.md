<code>
    
    Here's a guide on how to configure the RPi master/slave unit:

    Username      : alarm
    Password      : alarm
    Root password : root
    Master IP     : 192.168.1.13
    Slave IP      : 192.168.1.14
    
    Decompress and write the sdcard image to the scdard of the RPi:
    Download link is in the google docs document. md5sum: ef555083f4aab9b2d8f0774b758946d3
    Copy image to sdcard. Here sdX stands for the device name of the sdcard. You can find it by typing:
    
    $ lsblk
    $ sudo gunzip --stdout sdimage-20160424.img.gz | sudo dd bs=4M of=/dev/sdX

    Changes necessary only on slave unit:
    - become root:
      $ su

    - change hostname in /etc/hostname to rpislave
      # echo rpislave > /etc/hostname
    
    - setup slave static IP address
      # netctl disable eth0master
      # netctl enable eth0slave

    - setup time synchronization slave daemon:
      # systemctl disable ptpd-master.service
      # systemctl enable ptpd-slave.service

    - reboot:
      # reboot
    
</code>
