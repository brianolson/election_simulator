#GO2=-O2 -DNDEBUG
#GO2=-O2 -DNDEBUG -g
#GO2=-g
GO2=-O2
#CXXFLAGS=-Wall -g
CXXFLAGS=-Wall ${GO2} -m64
#CXXFLAGS+=-pg -g
CFLAGS=-Wall ${GO2} -m64
#LDFLAGS+=-pg
#LDFLAGS+=-L/usr/local/lib64
EMOBJS := AcceptanceVotePickOne.o FuzzyVotePickOne.o InstantRunoffVotePickOne.o
EMOBJS += OneVotePickOne.o RankedVotePickOne.o Condorcet.o TopNRunoff.o
EMOBJS += IRNR.o RandomElection.o STV.o IRNRP.o MaybeDebugLog.o
EMOBJS += ApprovalNoInfo.o ApprovalWithPoll.o
EMOBJS += VoteForAndAgainst.o Bucklin.o
EMOBJS += IteratedNormalizedRatings.o
EMOBJS += CIVSP.o

VPBOBJS := ResultFile.o VoterArray.o VoterSim.o VoterSim_run.o WorkQueue.o voter.o gauss.o
VPBOBJS += ResultLog.o NopResultLog.o ProtoResultLog.o trial.pb.o NameBlock.o GaussianRandom.o
VPBOBJS += ${EMOBJS}

SGOBJS := ${EMOBJS}
SGOBJS += VoterArray.o spacegraph.o GaussianRandom.o
SGOBJS += ResultFile.o voter.o gauss.o trial.pb.o
SGOBJS += VoterSim.o VoterSim_run.o WorkQueue.o NameBlock.o
SGOBJS += PlaneSim.o PlaneSimDraw.o XYSource.o ResultAccumulation.o
SGOBJS += spacegraph_util.o file_template.o

SGSRCS := VoterArray.cpp spacegraph.cpp GaussianRandom.cpp
SGSRCS += ResultFile.cpp voter.cpp gauss.cpp
SGSRCS += VoterSim.cpp VoterSim_run.o WorkQueue.cpp NameBlock.cpp
SGSRCS += PlaneSim.cpp PlaneSimDraw.cpp XYSource.cpp ResultAccumulation.cpp
SGSRCS += spacegraph_util.cpp

STOBJS := VoterArray.o WorkQueue.o voter.o speed_test.o gauss.o
STOBJS += ${EMOBJS} NameBlock.o GaussianRandom.o

UNAME := $(shell uname)

PROTOC := protoc

LINKPNG := -lpng16

include ${UNAME}.make
-include local.make

all:	spacegraph speedtest vpb processprl render_mcpb sgpb

# everything protobuf-needing, for bulk sim runs
pball:	vpb processprl render_mcpb sgpb

voter:	$(OBJS)
voter:	CC=${CXX}

voter_main_sm.o:	voter_main.cpp
	${CXX} ${CXXFLAGS} voter_main.cpp -c -o voter_main_sm.o

vpb:	${VPBOBJS} voter_main.cpp
	${CXX} -o vpb ${VPBOBJS} ${CXXFLAGS} ${LDFLAGS} -lprotobuf -DHAVE_PROTOBUF voter_main.cpp

PPLOBJS := processProtoResultLog.o ProtoResultLog.o ResultFile.o ResultLog.o NopResultLog.o trial.pb.o NameBlock.o

processprl:	${PPLOBJS}
	${CXX} -o processprl ${PPLOBJS} ${CXXFLAGS} ${LDFLAGS} -lprotobuf

RMCPBOBJS := PlaneSimDraw.o MessageLiteWriter.o ResultAccumulation.o
RMCPBOBJS += GaussianRandom.o trial.pb.o render_mcpb.o file_template.o

render_mcpb:	${RMCPBOBJS}
	${CXX} -o render_mcpb ${RMCPBOBJS} ${CXXFLAGS} ${LDFLAGS} -lprotobuf ${LINKPNG} -lz


sgpb:	CXXFLAGS+=-DHAVE_PROTOBUF
sgpb:	LDFLAGS+=-lprotobuf ${LINKPNG} -lz
sgpb:	${EMOBJS} ${SGSRCS} trial.pb.o MessageLiteWriter.o file_template.o
	${CXX} ${CXXFLAGS} ${EMOBJS} trial.pb.o MessageLiteWriter.o file_template.o ${SGSRCS} ${LDFLAGS} -o sgpb 

nnsv:   ${NNSVOBJS}

frob:	$(FROBOB)
	$(CXX) $(CXXFLAGS) $(FROBOB) $(LDFLAGS) -o $@

spacegraph: LDFLAGS+=-lprotobuf ${LINKPNG} -lz
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
	rm -f $(OBJS) $(FROBOB) $(TOPLOTOB) $(SGOBJS) ${VPBOBJS} voter.o voter frob nnsv spacegraph vpb trial.pb.cc trial.pb.h

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
	$(PROTOC) $< --cpp_out=$(@D)

ProtoResultLog.o:	trial.pb.h
PlaneSim.o:	trial.pb.h
processProtoResultLog.o:	trial.pb.h
render_mcpb.o:	trial.pb.h
spacegraph.o:	trial.pb.h
spacegraph_util.o:	trial.pb.h
VoterSim.o:	trial.pb.h
VoterSim_run.o:	trial.pb.h
# DO NOT DELETE

AcceptanceVotePickOne.o: Voter.h AcceptanceVotePickOne.h VotingSystem.h
ApprovalNoInfo.o: Voter.h ApprovalNoInfo.h VotingSystem.h
ApprovalWithPoll.o: Voter.h ApprovalWithPoll.h VotingSystem.h
Bucklin.o: Voter.h Bucklin.h VotingSystem.h
CIVSP.o: Voter.h CIVSP.h VotingSystem.h Condorcet.h PermutationIterator.h
Condorcet.o: Voter.h Condorcet.h VotingSystem.h
FuzzyVotePickOne.o: FuzzyVotePickOne.h VotingSystem.h Voter.h
GaussianRandom.o: GaussianRandom.h
IRNR.o: IRNR.h VotingSystem.h Voter.h
IRNRP.o: Voter.h IRNRP.h VotingSystem.h MaybeDebugLog.h
InstantRunoffVotePickOne.o: InstantRunoffVotePickOne.h VotingSystem.h Voter.h
IteratedNormalizedRatings.o: IteratedNormalizedRatings.h VotingSystem.h
IteratedNormalizedRatings.o: Voter.h
MaybeDebugLog.o: MaybeDebugLog.h
MessageLiteWriter.o: MessageLiteWriter.h
NNSVSim.o: NNStrategicVoter.h Voter.h VoterSim.h ResultFile.h NameBlock.h
NNSVSim.o: VotingSystem.h WorkQueue.h NNSVSim.h
NNStrategicVoter.o: NNStrategicVoter.h Voter.h
NameBlock.o: NameBlock.h
OneVotePickOne.o: Voter.h OneVotePickOne.h VotingSystem.h
PlaneSim.o: PlaneSim.h Voter.h VotingSystem.h PlaneSimDraw.h XYSource.h
PlaneSim.o: GaussianRandom.h ResultAccumulation.h gauss.h trial.pb.h
PlaneSimDraw.o: PlaneSimDraw.h GaussianRandom.h PlaneSim.h Voter.h
PlaneSimDraw.o: VotingSystem.h ResultAccumulation.h
ProtoResultLog.o: ProtoResultLog.h ResultLog.h VoterSim.h Voter.h
ProtoResultLog.o: ResultFile.h NameBlock.h
RandomElection.o: RandomElection.h VotingSystem.h Voter.h
RankedVotePickOne.o: Voter.h RankedVotePickOne.h VotingSystem.h
ResultAccumulation.o: ResultAccumulation.h
ResultFile.o: ResultFile.h NameBlock.h VotingSystem.h
ResultLog.o: ResultLog.h VoterSim.h Voter.h ResultFile.h NameBlock.h
STV.o: Voter.h STV.h VotingSystem.h
TopNRunoff.o: TopNRunoff.h VotingSystem.h Voter.h
VoteForAndAgainst.o: Voter.h VoteForAndAgainst.h VotingSystem.h
VoterArray.o: Voter.h gauss.h GaussianRandom.h
VoterSim.o: VoterSim.h Voter.h ResultFile.h NameBlock.h ResultLog.h
VoterSim.o: VotingSystem.h WorkQueue.h
VoterSim_run.o: VoterSim.h Voter.h ResultFile.h NameBlock.h ResultLog.h
VoterSim_run.o: VotingSystem.h WorkQueue.h
WorkQueue.o: WorkQueue.h
XYSource.o: XYSource.h
nnsv.o: Voter.h VoterSim.h ResultFile.h NameBlock.h VotingSystem.h
nnsv.o: OneVotePickOne.h RankedVotePickOne.h AcceptanceVotePickOne.h
nnsv.o: FuzzyVotePickOne.h InstantRunoffVotePickOne.h Condorcet.h IRNR.h
nnsv.o: RandomElection.h WorkQueue.h NNSVSim.h NNStrategicVoter.h
processProtoResultLog.o: ProtoResultLog.h ResultLog.h VoterSim.h Voter.h
processProtoResultLog.o: ResultFile.h NameBlock.h trial.pb.h
render_mcpb.o: MessageLiteWriter.h PlaneSimDraw.h ResultAccumulation.h
render_mcpb.o: arghandler.h file_template.h
resultDBToGnuplot.o: ResultFile.h NameBlock.h WorkQueue.h
resultFileFrob.o: ResultFile.h NameBlock.h
spacegraph.o: GaussianRandom.h PlaneSim.h Voter.h VotingSystem.h
spacegraph.o: PlaneSimDraw.h XYSource.h Condorcet.h IRNRP.h STV.h
spacegraph.o: arghandler.h file_template.h spacegraph_util.h
spacegraph_util.o: GaussianRandom.h VotingSystem.h PlaneSim.h Voter.h
spacegraph_util.o: PlaneSimDraw.h spacegraph_util.h
speed_test.o: Voter.h VoterSim.h ResultFile.h NameBlock.h VotingSystem.h
speed_test.o: OneVotePickOne.h RankedVotePickOne.h AcceptanceVotePickOne.h
speed_test.o: FuzzyVotePickOne.h InstantRunoffVotePickOne.h Condorcet.h
speed_test.o: IRNR.h RandomElection.h
test_PermutationIterator.o: PermutationIterator.h
voter.o: Voter.h VoterSim.h ResultFile.h NameBlock.h VotingSystem.h
voter.o: AcceptanceVotePickOne.h ApprovalNoInfo.h ApprovalWithPoll.h
voter.o: Condorcet.h FuzzyVotePickOne.h gauss.h InstantRunoffVotePickOne.h
voter.o: IRNR.h IteratedNormalizedRatings.h OneVotePickOne.h RandomElection.h
voter.o: RankedVotePickOne.h Bucklin.h VoteForAndAgainst.h WorkQueue.h
voter_main.o: Voter.h VoterSim.h ResultFile.h NameBlock.h VotingSystem.h
voter_main.o: OneVotePickOne.h RankedVotePickOne.h AcceptanceVotePickOne.h
voter_main.o: FuzzyVotePickOne.h InstantRunoffVotePickOne.h Condorcet.h
voter_main.o: IRNR.h STV.h IRNRP.h RandomElection.h ResultLog.h WorkQueue.h
voter_main.o: workQThread.h
