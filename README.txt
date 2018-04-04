On Mac (Darwin) or Linux, the following should just work:

make

The coolest tool is the `spacegraph` family of tools. It's called spacegraph because it works in a 2D space of political opinion and graphs the results of elections therein.

A simple way to run things is:
python runsgpb.py

THIS WILL RUN FOREVER, or until killed or `touch stop` in the simulator directory. It accumulates simulation data into files which can be turned into images by:
python runsgpb.py --render

---

The other major tool here is `vpb` which runs election simulations and accumulates results into data files which can be processed into charts and graphs of how elections do with different numbers of candidates, numbers of voters, or amount of error. TODO: document vpb better

---

Brian's voting stuff in general is at
http://bolson.org/voting/

This simulator distribution should be at
http://bolson.org/voting/sim_one_seat/dist/

Make internet polls counted by better election methods at
http://betterpolls.com/
