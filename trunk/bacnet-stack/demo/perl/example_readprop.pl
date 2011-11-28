use warnings;
use strict;

if (scalar(@ARGV) == 1)
{
    my $device = $ARGV[0];
    my ($resp, $failed) = ReadProperty($device, 'OBJECT_ANALOG_VALUE', 0, 'PROP_PRESENT_VALUE');

    print "status was '$failed' and the response was '$resp'\n";
}
else
{
    print "Must specify device instance number as an argument to this script\n";
}

1;
