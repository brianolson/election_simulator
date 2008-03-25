Mon Dec 18 13:46:58 PST 2006

On Mac (Darwin) or Linux, the following should just work:

make spacegraph

It's called spacegraph because it works in a 2D space of political opinion and graphs the results of elections therein.

The `runsg.pl` perl script is used for running a set of image generating tests on various candidate positions and election methods. One handy thing to do is to copy this and the `spacegraph` executable to a new directory, edit the script and run that configuration there.

There are other simulation utilities which can be built from these sources but I haven't worked on them in a while, they might not work and they generally aren't as simple and easy to use as spacegraph. The original simulator finds social utility of election methods for various numbers of choices, numbers of voters and conditions of error. If you're interested in running this it needs the BSD licensed BerkleyDB 4.x from Sleepycat software. Contact me at http://bolson.org/email.html for details.

-----

Brian's voting stuff in general is at
http://bolson.org/voting/

This simulator distribution should be at
http://bolson.org/voting/sim_one_seat/dist/

Make internet polls counted by better election methods at
http://betterpolls.com/
