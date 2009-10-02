#GO2=-O2 -DNDEBUG
GO2=-O2 -DNDEBUG -g
#GO2=-g
#CXXFLAGS=-Wall -g
CXXFLAGS=-Wall ${GO2} -m64
#CXXFLAGS+=-pg -g
CFLAGS=-Wall ${GO2} -m64
#LDFLAGS+=-pg
LDFLAGS+=-L/usr/local/lib64
EMOBJS := AcceptanceVotePickOne.o FuzzyVotePickOne.o InstantRunoffVotePickOne.o
EMOBJS += OneVotePickOne.o RankedVotePickOne.o Condorcet.o TopNRunoff.o
EMOBJS += IRNR.o RandomElection.o STV.o IRNRP.o MaybeDebugLog.o
EMOBJS += ApprovalNoInfo.o ApprovalWithPoll.o
EMOBJS += VoteForAndAgainst.o
EMOBJS += IteratedNormalizedRatings.o

OBJS := ResultFile.o DBResultFile.o VoterArray.o VoterSim.o WorkQueue.o
OBJS += ThreadSafeDBRF.o voter_main.o NameBlock.o
OBJS += ${EMOBJS}
#OBJS += voter.o

VSMALLOBJS := ResultFile.o VoterArray.o VoterSim.o WorkQueue.o voter.o
VSMALLOBJS += voter_main_sm.o gauss.o NameBlock.o
VSMALLOBJS += ${EMOBJS}

VPBOBJS := ResultFile.o VoterArray.o VoterSim.o WorkQueue.o voter.o gauss.o
VPBOBJS += ResultLog.o ProtoResultLog.o trial.pb.o NameBlock.o
VPBOBJS += ${EMOBJS}

FROBOB := ResultFile.o DBResultFile.o resultFileFrob.o NameBlock.o

TOPLOTOB := resultDBToGnuplot.o ResultFile.o DBResultFile.o WorkQueue.o NameBlock.o

NNSVOBJS := ${EMOBJS}
NNSVOBJS += ResultFile.o DBResultFile.o VoterArray.o VoterSim.o WorkQueue.o
NNSVOBJS += ThreadSafeDBRF.o voter.o
NNSVOBJS += NNSVSim.o NameBlock.o
NNSVOBJS += NNStrategicVoter.o

SGOBJS := ${EMOBJS}
SGOBJS += VoterArray.o spacegraph.o
SGOBJS += ResultFile.o voter.o gauss.o
SGOBJS += VoterSim.o WorkQueue.o NameBlock.o
#SGOBJS += DBResultFile.o ThreadSafeDBRF.o

STOBJS := VoterArray.o WorkQueue.o voter.o speed_test.o gauss.o
STOBJS += ${EMOBJS} NameBlock.o

UNAME := $(shell uname)

include ${UNAME}.make

all:	spacegraph speedtest vsmall
#all:	voter frob resultDBToGnuplot nnsv spacegraph

voter:	$(OBJS)
voter:	CC=${CXX}

voter_main_sm.o:	voter_main.cpp
	${CXX} ${CXXFLAGS} voter_main.cpp -c -o voter_main_sm.o -DNO_DB

vsmall:	${VSMALLOBJS}
	${CXX} -o vsmall ${VSMALLOBJS} ${CXXFLAGS} ${LDFLAGS}

# everything protobuf-needing, for bulk sim runs
pball:	vpb processprl

vpb:	${VPBOBJS} voter_main.cpp
	${CXX} -o vpb ${VPBOBJS} ${CXXFLAGS} ${LDFLAGS} -lprotobuf -DHAVE_PROTOBUF -DNO_DB voter_main.cpp

PPLOBJS := processProtoResultLog.o ProtoResultLog.o ResultFile.o ResultLog.o trial.pb.o NameBlock.o

processprl:	${PPLOBJS}
	${CXX} -o processprl ${PPLOBJS} ${CXXFLAGS} ${LDFLAGS} -lprotobuf

nnsv:   ${NNSVOBJS}

frob:	$(FROBOB)
	$(CXX) $(CXXFLAGS) $(FROBOB) $(LDFLAGS) -o $@

resultDBToGnuplot:	$(TOPLOTOB)
resultDBToGnuplot:	CC=${CXX}

spacegraph: LDFLAGS+=-lpng12 -lz -g
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
	rm -f $(OBJS) $(FROBOB) $(TOPLOTOB) $(SGOBJS) ${VPBOBJS} voter.o voter frob resultDBToGnuplot nnsv spacegraph vpb vsmall trial.pb.cc trail.pb.h
depend:
	makedepend -Y *.cpp

dist:	. CVS/* Makefile
	mkdir -p dist; cd dist; rm -rf voting; cvs export -r HEAD voting; tar cf voting.tar voting; rm -rf voting; gzip < voting.tar > voting.tar.gz; bzip2 < voting.tar > voting.tar.bz2; rm -f voting.tar
	@echo dist left in dist

ballot.html:	formCandidates makeForm.pl
	echo "<html><head><title>voting form</title></head><body>" > ballot.html
	./makeForm.pl formCandidates >> ballot.html
	echo "</body></html>" >> ballot.html

%.pb.cc %.pb.h : %.proto
	protoc $< --cpp_out=$(@D)

ProtoResultLog.o:	trial.pb.h
# DO NOT DELETE

AcceptanceVotePickOne.o: Voter.h AcceptanceVotePickOne.h VotingSystem.h
ApprovalNoInfo.o: Voter.h ApprovalNoInfo.h VotingSystem.h
ApprovalWithPoll.o: Voter.h ApprovalWithPoll.h VotingSystem.h
Condorcet.o: Voter.h Condorcet.h RankedVotePickOne.h VotingSystem.h
DBResultFile.o: DBResultFile.h ResultFile.h
FuzzyVotePickOne.o: FuzzyVotePickOne.h VotingSystem.h Voter.h
IRNR.o: IRNR.h VotingSystem.h Voter.h
InstantRunoffVotePickOne.o: InstantRunoffVotePickOne.h VotingSystem.h Voter.h
IteratedNormalizedRatings.o: IteratedNormalizedRatings.h VotingSystem.h
IteratedNormalizedRatings.o: Voter.h
NNSVSim.o: NNStrategicVoter.h Voter.h VoterSim.h ResultFile.h DBResultFile.h
NNSVSim.o: VotingSystem.h WorkQueue.h NNSVSim.h
NNStrategicVoter.o: NNStrategicVoter.h Voter.h
OneVotePickOne.o: Voter.h OneVotePickOne.h VotingSystem.h
ProtoResultLog.o: ProtoResultLog.h ResultLog.h VoterSim.h Voter.h
ProtoResultLog.o: ResultFile.h trial.pb.h
RandomElection.o: RandomElection.h VotingSystem.h Voter.h
RankedVotePickOne.o: Voter.h RankedVotePickOne.h VotingSystem.h
ResultFile.o: ResultFile.h VotingSystem.h
ResultLog.o: ResultLog.h VoterSim.h Voter.h ResultFile.h
ThreadSafeDBRF.o: ThreadSafeDBRF.h DBResultFile.h ResultFile.h
TopNRunoff.o: TopNRunoff.h VotingSystem.h Voter.h
VoteForAndAgainst.o: Voter.h VoteForAndAgainst.h VotingSystem.h
VoterArray.o: Voter.h gauss.h
VoterSim.o: VoterSim.h Voter.h ResultFile.h ResultLog.h DBResultFile.h
VoterSim.o: VotingSystem.h WorkQueue.h VoterSim_run.h
WorkQueue.o: WorkQueue.h
nnsv.o: Voter.h VoterSim.h ResultFile.h VotingSystem.h OneVotePickOne.h
nnsv.o: RankedVotePickOne.h AcceptanceVotePickOne.h FuzzyVotePickOne.h
nnsv.o: InstantRunoffVotePickOne.h Condorcet.h IRNR.h RandomElection.h
nnsv.o: DBResultFile.h ThreadSafeDBRF.h WorkQueue.h NNSVSim.h
nnsv.o: NNStrategicVoter.h
resultDBToGnuplot.o: ResultFile.h WorkQueue.h DBResultFile.h
resultFileFrob.o: ResultFile.h DBResultFile.h
spacegraph.o: Voter.h VoterSim.h ResultFile.h VotingSystem.h OneVotePickOne.h
spacegraph.o: RankedVotePickOne.h AcceptanceVotePickOne.h FuzzyVotePickOne.h
spacegraph.o: InstantRunoffVotePickOne.h Condorcet.h IRNR.h
spacegraph.o: IteratedNormalizedRatings.h RandomElection.h gauss.h
speed_test.o: Voter.h VoterSim.h ResultFile.h VotingSystem.h OneVotePickOne.h
speed_test.o: RankedVotePickOne.h AcceptanceVotePickOne.h FuzzyVotePickOne.h
speed_test.o: InstantRunoffVotePickOne.h Condorcet.h IRNR.h RandomElection.h
voter.o: Voter.h VoterSim.h ResultFile.h VotingSystem.h
voter.o: AcceptanceVotePickOne.h Condorcet.h RankedVotePickOne.h
voter.o: FuzzyVotePickOne.h gauss.h InstantRunoffVotePickOne.h IRNR.h
voter.o: IteratedNormalizedRatings.h OneVotePickOne.h RandomElection.h
voter.o: WorkQueue.h
voter_main.o: Voter.h VoterSim.h ResultFile.h VotingSystem.h OneVotePickOne.h
voter_main.o: RankedVotePickOne.h AcceptanceVotePickOne.h FuzzyVotePickOne.h
voter_main.o: InstantRunoffVotePickOne.h Condorcet.h IRNR.h RandomElection.h
voter_main.o: DBResultFile.h ThreadSafeDBRF.h WorkQueue.h workQThread.h
