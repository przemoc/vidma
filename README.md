vidma - Virtual Disks Manipulator
=================================

vidma is a utility for manipulating virtual disk images. It can show basic
information about the image or resize it. Resizing is done by in-place
modification of a file holding the image or by creating modified copy of such
file.


Supported formats
-----------------

  * _VDI - Virtual Disk Image_  
    Format introduced by VirtualBox and mostly used by VirtualBox. It has a few
    variants, but only two types, fixed and dynamic, are handled by `vidma`.


Requirements
------------

* little-endian machine, e.g. x86, x86-64
* Windows or POSIX OS, e.g. BSD, Linux, Mac OS X


Links
-----

* [VirtualBox Forum topic](http://tinyurl.com/vbox-vidma)  
  (contains example of usage)


Bugs
----

If you find any bug, then please create a new issue in the project's
[GitHub page][1] and describe the problem there, unless someone already did it
before you.

  [1]: https://github.com/przemoc/vidma/issues

Remember to provide following information:

* What system do you have?  
  (`uname -a`, `lsb_release -drc`)
* What compiler do you use? (if you have built `vidma` manually)  
  (`cc -v`)
* What `vidma` version are you using?  
  (first line of `vidma` output)
* What have you done?  
  (run `history` and check the commands used to compile and run vidma)
* If problem regards corrupted image, then paste information about the original
  image and the one after failed modification.  
  (`vidma original_image_file`, `vidma modified_image_file`)


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
