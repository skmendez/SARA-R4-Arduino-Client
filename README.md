# SARA-R4-Arduino-Client

[![Build Status](https://travis-ci.org/skmendez/SARA-R4-Arduino-Client.svg?branch=master)](https://travis-ci.org/skmendez/SARA-R4-Arduino-Client)

A modification of the [MKRNB library](https://github.com/arduino-libraries/MKRNB) which provides an [Arduino Client](https://www.arduino.cc/en/Reference/ClientConstructor) interface for a SARA R4 connected through any Serial interface.

This library mostly follows the MKRNB reference: http://www.arduino.cc/en/Reference/MKRNB

However, all classes must now be initialized with a `Modem`.

## Example

```c++
SoftwareSerial r4(8,9);

Modem modem(r4, 115200, 5, 6);
NBClient client(modem);
``` 