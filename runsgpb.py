#!/usr/bin/python

import os
import subprocess
import sys

nicepathlist = []
for nicepath in ['/usr/bin/nice', '/bin/nice']:
	if os.access(nicepath, os.X_OK):
		nicepathlist = [nicepath]

candsets = [
	("threecorners","--candidates=1,1,-1,1,0,-1"),
	("fourcorners","--candidates=1,1,-1,1,-1,-1,1,-1"),
	("3a","--candidates=-0.86,-0.66,-0.02,-0.98,-0.18,-0.96"),
	("3b","--candidates=0.86,-0.02,0.58,-0.16,-0.46,-0.10"),
	("3c","--candidates=0.08,-0.06,0.54,0.28,-0.74,-0.80"),
	("4a","--candidates=-0.76,-0.44,0.70,0.40,-0.22,-0.44,0.94,-0.72"),
	("4b","--candidates=-0.52,-0.54,-0.62,0.24,-0.92,0.28,0.70,0.10"),
	("4c","--candidates=-0.20,0.14,-0.68,0.08,-0.90,0.24,0.82,0.40"),
]

# ./sgpb --candidates=-0.20,0.14,-0.68,0.08,-0.90,0.24,0.82,0.40 -minx -1 -miny -1 -maxx 1 -maxy 1 -Z 1.0 -v 1000 --threads=3 --method IRNR,IRV,Condorcet,Bucklin,Borda,OneVote,Random,Approval --mc-tests 10000 --mc-out=4c.pbz --config-file=4c_config.pb

binary = "./sgpb"
threads = 2
runcount = 5000
methods = "IRNR,IRV,Condorcet,Bucklin,Borda,OneVote,Random,Approval,Max"


def runall():
	basecmd = nicepathlist + [binary,
			"--minx=-1.0", "--miny=-1.0", "--maxx=1.0", "--maxy=1.0",
			"-Z=1.0", "-v=1000",
			"--method=%s" % methods,
			"--threads=%d" % threads,
			"--mc-tests=%d" % runcount]
	for name, candarg in candsets:
		cmd = basecmd + [ candarg,
			"--mc-out=%s.pbz" % name,
			"--config-file=%s_config.pb" % name]
		retcode = subprocess.call(cmd)
		if retcode != 0:
			print "failure!"
			print " ".join(cmd)
			sys.exit(1)

if __name__ == '__main__':
	while not os.path.exists('stop'):
		runall()
	os.unlink('stop')
