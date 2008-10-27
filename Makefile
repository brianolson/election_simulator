#CXXFLAGS=-Wall -g
CXXFLAGS=-Wall -O2 -m64
#CXXFLAGS+=-pg -g
CFLAGS=-Wall -O2 -m64
#LDFLAGS+=-pg
LDFLAGS+=-L/usr/local/lib64
EMOBJS := AcceptanceVotePickOne.o FuzzyVotePickOne.o InstantRunoffVotePickOne.o
EMOBJS += OneVotePickOne.o RankedVotePickOne.o Condorcet.o TopNRunoff.o
EMOBJS += IRNR.o RandomElection.o
EMOBJS += ApprovalNoInfo.o ApprovalWithPoll.o
EMOBJS += VoteForAndAgainst.o
EMOBJS += IteratedNormalizedRatings.o

OBJS := ResultFile.o DBResultFile.o VoterArray.o VoterSim.o WorkQueue.o
OBJS += ThreadSafeDBRF.o voter_main.o
OBJS += ${EMOBJS}
#OBJS += voter.o

FROBOB := ResultFile.o DBResultFile.o resultFileFrob.o

TOPLOTOB := resultDBToGnuplot.o ResultFile.o DBResultFile.o WorkQueue.o

NNSVOBJS := ${EMOBJS}
NNSVOBJS += ResultFile.o DBResultFile.o VoterArray.o VoterSim.o WorkQueue.o
NNSVOBJS += ThreadSafeDBRF.o voter.o
NNSVOBJS += NNSVSim.o
NNSVOBJS += NNStrategicVoter.o

SGOBJS := ${EMOBJS}
SGOBJS += VoterArray.o spacegraph.o
SGOBJS += ResultFile.o voter.o gauss.o
SGOBJS += VoterSim.o WorkQueue.o
#SGOBJS += DBResultFile.o ThreadSafeDBRF.o

STOBJS := ResultFile.o DBResultFile.o VoterArray.o VoterSim.o WorkQueue.o
STOBJS += ThreadSafeDBRF.o voter.o speed_test.o
STOBJS += ${EMOBJS}

UNAME := $(shell uname)

include ${UNAME}.make

all:	spacegraph speedtest
#all:	voter frob resultDBToGnuplot nnsv spacegraph

voter:	$(OBJS)
voter:	CC=${CXX}

nnsv:   ${NNSVOBJS}

frob:	$(FROBOB)
	$(CXX) $(CXXFLAGS) $(FROBOB) $(LDFLAGS) -o $@

resultDBToGnuplot:	$(TOPLOTOB)
resultDBToGnuplot:	CC=${CXX}

spacegraph: LDFLAGS+=-lpng -lz -g
spacegraph:	${SGOBJS}
	${CXX} ${CXXFLAGS} ${SGOBJS} ${LDFLAGS} -o spacegraph
#spacegraph: CC=${CXX}

speedtest:	${STOBJS}
	${CXX} ${CXXFLAGS} ${STOBJS} ${LDFLAGS} -o speedtest

METHODS := Max OneVote IRV IRNR Condorcet Rated
FOURCORNERS := $(METHODS:%=fourcorners_%.png)
SGWORLD := -px 400 -py 400 -n 4 -minx -1 -miny -1 -maxx 1 -maxy 1 -Z 1.0 -v 10000
FOURCORNERS_CANDS := -c 1,1 -c -1,1 -c -1,-1 -c 1,-1

fourcorners_%.png:	spacegraph
	./spacegraph --method $* ${SGWORLD} ${FOURCORNERS_CANDS} -o $@

fourcorners_all:	${FOURCORNERS}

THREE_A_CANDS := -c -0.86,-0.66 -c -0.02,-0.98 -c -0.18,-0.96
THREE_A := $(METHODS:%=3a_%.png)
3a_%.png:	spacegraph
	./spacegraph --method $* ${SGWORLD} ${THREE_A_CANDS} -o $@

THREE_B_CANDS := -c 0.86,-0.02 -c 0.58,-0.16 -c -0.46,-0.10
THREE_B := $(METHODS:%=3b_%.png)
3b_%.png:	spacegraph
	./spacegraph --method $* ${SGWORLD} ${THREE_B_CANDS} -o $@

THREE_C_CANDS := -c 0.08,-0.06 -c 0.54,0.28 -c -0.74,-0.80
THREE_C := $(METHODS:%=3c_%.png)
3c_%.png:	spacegraph
	./spacegraph --method $* ${SGWORLD} ${THREE_C_CANDS} -o $@

FOUR_A_CANDS := -c -0.76,-0.44 -c 0.70,0.40 -c -0.22,-0.44 -c 0.94,-0.72
FOUR_A := $(METHODS:%=4a_%.png)
4a_%.png:	spacegraph
	./spacegraph --method $* ${SGWORLD} ${FOUR_A_CANDS} -o $@

FOUR_B_CANDS := -c -0.52,-0.54 -c -0.62,0.24 -c -0.92,0.28 -c 0.70,0.10
FOUR_B := $(METHODS:%=4b_%.png)
4b_%.png:	spacegraph
	./spacegraph --method $* ${SGWORLD} ${FOUR_B_CANDS} -o $@

FOUR_C_CANDS := -c -0.20,0.14 -c -0.68,0.08 -c -0.90,0.24 -c 0.82,0.40
FOUR_C := $(METHODS:%=4c_%.png)
4c_%.png:	spacegraph
	./spacegraph --method $* ${SGWORLD} ${FOUR_C_CANDS} -o $@

sg_runall:	${THREE_A} ${THREE_B} ${THREE_C} ${FOUR_A} ${FOUR_B} ${FOUR_C} ${FOURCORNERS}

headerdoc:
	mkdir -p doc
	headerdoc2html -o doc *.h
	gatherheaderdoc doc

clean:
	rm -f $(OBJS) $(FROBOB) $(TOPLOTOB) $(SGOBJS) voter.o voter frob resultDBToGnuplot nnsv spacegraph
depend:
	makedepend -Y *.cpp

dist:	. CVS/* Makefile
	mkdir -p dist; cd dist; rm -rf voting; cvs export -r HEAD voting; tar cf voting.tar voting; rm -rf voting; gzip < voting.tar > voting.tar.gz; bzip2 < voting.tar > voting.tar.bz2; rm -f voting.tar
	@echo dist left in dist

ballot.html:	formCandidates makeForm.pl
	echo "<html><head><title>voting form</title></head><body>" > ballot.html
	./makeForm.pl formCandidates >> ballot.html
	echo "</body></html>" >> ballot.html

# DO NOT DELETE

AcceptanceVotePickOne.o: Voter.h AcceptanceVotePickOne.h VotingSystem.h
Condorcet.o: Voter.h Condorcet.h RankedVotePickOne.h VotingSystem.h
DBResultFile.o: DBResultFile.h ResultFile.h
FuzzyVotePickOne.o: FuzzyVotePickOne.h VotingSystem.h Voter.h
InstantRunoffVotePickOne.o: InstantRunoffVotePickOne.h VotingSystem.h Voter.h
OneVotePickOne.o: Voter.h OneVotePickOne.h VotingSystem.h
RankedVotePickOne.o: Voter.h RankedVotePickOne.h VotingSystem.h
ResultFile.o: ResultFile.h VotingSystem.h
ThreadSafeDBRF.o: ThreadSafeDBRF.h DBResultFile.h ResultFile.h
VoterArray.o: Voter.h
VoterSim.o: VoterSim.h Voter.h ResultFile.h DBResultFile.h VotingSystem.h
VoterSim.o: WorkQueue.h VoterSim_run.h
WorkQueue.o: WorkQueue.h
resultDBToGnuplot.o: ResultFile.h WorkQueue.h DBResultFile.h
resultFileFrob.o: ResultFile.h DBResultFile.h
voter.o: Voter.h VoterSim.h VotingSystem.h OneVotePickOne.h
voter.o: RankedVotePickOne.h AcceptanceVotePickOne.h FuzzyVotePickOne.h
voter.o: InstantRunoffVotePickOne.h Condorcet.h ResultFile.h DBResultFile.h
voter.o: ThreadSafeDBRF.h WorkQueue.h
