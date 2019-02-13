LCLS-II REAL Heater Control
---------------------------

AT90CAN128 (AST-CAN485 board)-based project that interfaces with a Beckhoff PLC
via CANopen, controlling 32 PWM outputs using PCA9685 16-channel 12-bit PWM
chips.

Object Dictionary
-----------------

The CANopen object dictionary is provided as both `.od` and `.eds` (what
Beckhoff TwinCAT3 expects).

Building
--------

The provided `Makefile` and Atmel Studio 7 project should work directly.

Uploading
---------

A couple tweaks to the Makefile:
    1. Change the `ARDUINO_ROOT` path to point to where you have it installed
    2. Change the `COM_PORT` to the appropriate device
    
Will also allow you to upload using `avrdude` like so:

```bash
$ make all upload
```

CANopen library
---------------

This repository, in the interests of making a focused and easily compilable
example for the AST-CAN485, includes a vendored copy of `canfestival-3-asc`.

It currently corresponds with the changeset `968:1a25f5151a8d` (2018/5/11) from
its bitbucket hg repository.

Python
------

Located in `canfestival/objdictgen`:

The object dictionary editor program, based on wxPython, still runs on Python 2.7.
It requires wxPython 3, which is not available via pip. 

To get this working with conda:
```bash
$ conda create -n canfestival python=2.7 wxpython==3
$ cd canfestival/objdictgen
$ source activate canfestival
$ pythonw objdictedit.py ../../real_objdict.od
```

Links
-----
[Project template](https://github.com/klauer/can485_canopen_slave)
[canfestival-3-asc repository](https://bitbucket.org/Mongo/canfestival-3-asc)
[AST-CAN485 product page](https://www.sparkfun.com/products/14483)
[AST-CAN485 schematic](https://cdn.sparkfun.com/assets/2/8/3/4/7/SparkFun_AST-CAN485.pdf)
[AST-CAN485 hookup guide](https://learn.sparkfun.com/tutorials/ast-can485-hookup-guide/)
