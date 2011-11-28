The BACnet Scriptable (using Perl) Tool.

* Running this tool assumes that the library has been already built. Currently,
  the tool assumes only win32 port, but should be easily modifiable for any
  port build. The library has to be built with BBMD_DEFINE=-DBBMD_ENABLED\=1
* This tool has to be run from a path without any spaces. The presence of the
  .Inline directory is required.
* Run the tool without any arguments to see usage instructions
* To run the example ReapProperty script (which reads Analog Value 0 Present
  Value) for Device at instance 1234 run the following command

  perl bacnet.pl --script example_readprop.pl -- 1234


