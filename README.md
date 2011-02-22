vidma - Virtual Disks Manipulator
=================================

Utility for manipulating virtual disk images.


Features
--------

* resizing fixed-size images
* operations can be done in-place


Supported formats
-----------------

* VDI - Virtual Disk Image, used mostly by VirtualBox


Requirements
------------

* little-endian machine, e.g. x86, x86-64
* Windows or POSIX OS, e.g. Linux


Links
-----

* [VirtualBox Forum topic][1] (contains example of usage)

  [1]: http://tinyurl.com/vbox-vidma


Bugs
----

If you find any bug, then please create new issue in [GitHub][2] and describe
the problem there, unless someone did it before you.

  [2]: https://github.com/przemoc/vidma/issues

Remember to provide following information:

* What system do you have?  
  (`uname -a`, `lsb_release -drc`)
* What gcc version are you using?  
  (`gcc -v`)
* What have you done?  
  (run `history` and check the command-line for compiling and running vidma)
* If problem regards corrupted image, then paste information from vidma about
  the original image and the one after failed modification.  
  (`vidma original_image`, `vidma modified_image`)


Development
-----------

### Roadmap

Stages:

* alpha (works for me?)
  * 0.0.x
  * 0.1.x
  * 0.2.x
* beta (works for you?)
  * 0.3.x
  * 0.4.x
* release candidate (works for everyone?)
  * 0.5.x
* ready (just works!)

### Hacking

Don't waste your time until vidma will be close to beta stage. I mean it.  
I am aware of many shortcomings (some are listed in [issues][2] w/ todo label)
and they will be eventually addressed (in no particular order).

I won't accept any pull request before releasing version 0.2.0. ;-)


CAUTION!
--------

Program is in **ALPHA** stage, therefore may be **HARMFUL** and **UNSAFE**!  
You have been warned! **USE AT YOUR OWN RISK! NO WARRANTY!**

To reduce possible damages of in-place operation **ALWAYS BACKUP YOUR IMAGE**
or just do not use them at all.
