#include "GaussianRandom.h"
#include "PlaneSim.h"
#include "PlaneSimDraw.h"
#include "VotingSystem.h"
#include "XYSource.h"

#include "Condorcet.h"
#include "IRNRP.h"
#include "STV.h"
#include "STAR.h"

#if HAVE_PROTOBUF
#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/gzip_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/message_lite.h"
#include "MessageLiteWriter.h"
#include "trial.pb.h"

using google::protobuf::io::CodedOutputStream;
using google::protobuf::io::FileInputStream;
using google::protobuf::io::FileOutputStream;
using google::protobuf::io::GzipOutputStream;
using google::protobuf::io::ZeroCopyOutputStream;
#endif

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "arghandler.h"
#include "file_template.h"
#include "spacegraph_util.h"

#ifndef MAX_METHOD_ARGS
#define MAX_METHOD_ARGS 64
#endif

// This doesn't do anything but make sure these methods get linked in.
void* linker_tricking() {
  delete new STV();
  delete new STARVote();
  return new IRNRP();
}

volatile int goGently = 0;

void mysigint( int a ) {
  goGently = 1;
}

// TODO: put these into a spacegraph_util.h
void printEMList(void);
VotingSystem** getVotingSystemsFromMethodNames(const char* methodNames, int* length_OUT);
char* fileTemplate(const char* tpl, const char* methodName, const char* plane);

const char* usage =
"usage: spacegraph [-o main output png file name template]\n"
"\t[-tg test gauss png file name]\n"
"\t[-minx f][-miny f][-maxx f][-maxy f]\n"
"\t[-px i][-py i][-v voters][-n iter per pix][-Z sigma]\n"
"\t[-c \"candidateX Y\"][--list]\n"
"\t[--method electionmethod[,...]]\n"
"\t[--random random device][--planes plane file template]\n"
;

#ifndef MAX_METHOD_ENV
#define MAX_METHOD_ENV 200
#endif
const char defaultFoutname[] = "tsg.png";


// Command line arg helpers
class MethodEnvReference {
 public:
  const char** methodEnv;
  int* methodEnvCount;
  MethodEnvReference(const char** me, int* mer) : methodEnv(me), methodEnvCount(mer){}
};
static void addMethodEnv(const MethodEnvReference& mer, const char* arg) {
  mer.methodEnv[*(mer.methodEnvCount)] = arg;
  *(mer.methodEnvCount) = *(mer.methodEnvCount) + 1;
}
static void addCandidateArgWrapper(PlaneSim* sim, const char* arg) {
  sim->addCandidateArg(arg);
}


void runRandomTests(const char* pblog_name, int randomTests, int nthreads, PlaneSim& sim, PlaneSimThread* threads) {
#if HAVE_PROTOBUF
  int fd = open(pblog_name, O_WRONLY|O_APPEND|O_CREAT, 0666);
  if (fd < 0) {
    perror(pblog_name);
    exit(1);
  }
  ZeroCopyOutputStream* raw_output = new FileOutputStream(fd);
  assert(raw_output);
  ZeroCopyOutputStream* gz_output = new GzipOutputStream(raw_output);
  assert(gz_output);
  CodedOutputStream* coded_output = new CodedOutputStream(gz_output);
  assert(coded_output);
  MessageLiteWriter* writer = new CSMessageLiteWriter(coded_output);
  assert(writer);
  if (nthreads == 1) {
    Result2** resultsOut = new Result2*[sim.systemsLength];
    assert(resultsOut);
    for (int i = 0; i < sim.systemsLength; ++i) {
      resultsOut[i] = NULL;
    }
    for (int mct = 0; mct < randomTests; ++mct) {
      sim.runRandomXY(resultsOut);
      for (int i = 0; i < sim.systemsLength; ++i) {
        writer->writeMessage(resultsOut[i]);
      }
    }
    for (int i = 0; i < sim.systemsLength; ++i) {
      delete resultsOut[i];
    }
    delete [] resultsOut;
  } else {
    assert(nthreads > 1);
    int countdown = randomTests;
    pthread_mutex_t countLock;
    pthread_mutex_t writerLock;
    int err = pthread_mutex_init(&countLock, NULL);
    assert(err == 0);
    err = pthread_mutex_init(&writerLock, NULL);
    assert(err == 0);
    for (int i = 0; i < nthreads; ++i) {
      threads[i].writer = writer;
      threads[i].countToDo = &countdown;
      threads[i].countLock = &countLock;
      threads[i].writerLock = &writerLock;
    }
    for (int i = 0; i < nthreads; ++i) {
      pthread_create(&(threads[i].thread), NULL, runRandomTestThread, &(threads[i]));
    }
    for (int i = 0; i < nthreads; ++i) {
      pthread_join(threads[i].thread, NULL);
    }
    pthread_mutex_destroy(&countLock);
    pthread_mutex_destroy(&writerLock);
  }
  delete writer;
  delete coded_output;
  delete gz_output;
  delete raw_output;
  close(fd);
#else
  perror("not compiled with protobuf support");
  exit(1);
#endif
}

static const int nthreads_default = 1;

int main( int argc, const char** argv ) {
  const char* votingSystemName = NULL;
  const char** methodEnv = new const char*[MAX_METHOD_ENV];
  int methodEnvCount = 0;
  PlaneSim sim;
  const char* foutname = defaultFoutname;
  const char* testgauss = NULL;
  int nvoters = nvoters_default;
  int nthreads = nthreads_default;
  const char* planesPrefix = NULL;
  const char* randomDevice = NULL;
  int randomTests = 0;
  const char* pblog_name = "mc.pb";
#if HAVE_PROTOBUF
  int configId = -1;
  const char* configOut = NULL;
#endif
  const char* candidateSpecString = NULL;
  int numVotingSystems = 1;
  VotingSystem** systems = NULL;

  int argi = 1;
  while (argi < argc) {
    StringArgWithCallback("c", addCandidateArgWrapper, &sim);
    StringArg("candidates", &candidateSpecString);
    BoolArg("combine", &(sim.doCombinatoricExplode));
#if HAVE_PROTOBUF
    // configuration in a protobuf in a file.
    IntArg("config-id", &configId);
    IntArg("config_id", &configId);
    StringArg("config-file", &configOut);
    StringArg("config_file", &configOut);
    StringArg("config-out", &configOut);
    StringArg("config_out", &configOut);
#endif
    BoolArg("linearFallof", &(sim.linearFalloff));
    BoolArg("manhattan", &(sim.manhattanDistance));
    DoubleArg("maxx", &(sim.maxx));
    DoubleArg("maxy", &(sim.maxy));
    DoubleArg("minx", &(sim.minx));
    DoubleArg("miny", &(sim.miny));
    StringArg("mc-out", &pblog_name);
    StringArg("mc_out", &pblog_name);
    IntArg("mc-tests", &randomTests);
    IntArg("mc_tests", &randomTests);
    StringArg("method", &votingSystemName);
    IntArg("n", &(sim.electionsPerPixel));
    StringArg("o", &foutname);
    StringArgWithCallback("opt", addMethodEnv, MethodEnvReference(methodEnv, &methodEnvCount));
    IntArg("px", &(sim.px));
    IntArg("py", &(sim.py));
    StringArg("planes", &planesPrefix);
    StringArg("random", &randomDevice);
    IntArg("seats", &(sim.seats));
    StringArg("tg", &testgauss);
    IntArg("threads", &nthreads);
    IntArg("v", &nvoters);
    DoubleArg("Z", &(sim.voterSigma));
    if ( ! strcmp( argv[argi], "--list" ) ) {
      printEMList();
      exit(0);
    } else {
      fprintf( stderr, "bogus argv[%d] \"%s\"\n", argi, argv[argi] );
      fputs( usage, stderr );
      fputs( "Known election methods:\n", stderr );
      printEMList();
      exit(1);
    }
    argi++;
  }
  if (sim.seats != 1) {
    char* seatsMethodArg = new char[20];  // TODO: this leaks
    sprintf(seatsMethodArg, "seats=%d", sim.seats);
    methodEnv[methodEnvCount] = seatsMethodArg;
    methodEnvCount++;
  }
  if (candidateSpecString) {
    assert(sim.candcount == 0);
    char* tspec = strdup(candidateSpecString);
    // null out every other comma, submit candidate args.
    int commaCount = 0;
    char* cur = tspec;
    char* lastStart = tspec;
    while (*cur != '\0') {
      if (*cur == ',') {
        if (commaCount == 1) {
          *cur = '\0';
          sim.addCandidateArg(lastStart);
          lastStart = cur + 1;
          commaCount = 0;
        } else {
          commaCount = 1;
        }
      }
      cur++;
    }
    if (cur != lastStart) {
      sim.addCandidateArg(lastStart);
    }
  }
  if ( votingSystemName != NULL ) {
    systems = getVotingSystemsFromMethodNames(votingSystemName, &numVotingSystems);
  }
	
#if HAVE_PROTOBUF
  if (configOut != NULL) {
    int err = readConfigFile(&configOut, sim, &nvoters, &numVotingSystems, &systems);
    if (err != 0) {
      fprintf(stderr, "failed to read config file %s\n", configOut);
      exit(1); return 1;
    }
  }
#endif		
  if ( sim.candcount == 0 && testgauss == NULL ) {
    fprintf( stderr, "error, no candidate positions specified\n");
    exit(1);
  }
  if ( randomDevice != NULL ) {
    sim.rootRandom = new FileDoubleRandom(randomDevice, 8*1024);
  } else {
    sim.rootRandom = new ClibDoubleRandom();
  }
  if ( testgauss != NULL ) {
    testGauss(testgauss, &sim, nvoters);
    return 0;
  }
  if (systems == NULL) {
    systems = new VotingSystem*[numVotingSystems];
    systems[0] = new Condorcet();
  }
  if ( methodEnvCount > 0 ) {
    methodEnv[methodEnvCount] = NULL;
    for (int vi = 0; vi < numVotingSystems; ++vi) {
      systems[vi]->init( methodEnv );
    }
  }
  sim.build( nvoters );
  {
    char cfgstr[512];
    sim.configStr( cfgstr, sizeof(cfgstr) );
    fputs( cfgstr, stdout );
    fputs( "\n", stdout );
  }
  sim.setVotingSystems(systems, numVotingSystems);
  PlaneSim* sims = NULL;
  PlaneSimThread* threads = NULL;
  if ( nthreads <= 1 ) {
    sim.gRandom = new GaussianRandom(sim.rootRandom);
  } else {
#if 1
    sim.rootRandom = new ClibDoubleRandom();
    sim.gRandom = new GaussianRandom(sim.rootRandom);
#else
    sim.rootRandom = new LockingDoubleRandomWrapper(sim.rootRandom);
    sim.gRandom = new GaussianRandom(
        new BufferDoubleRandomWrapper(sim.rootRandom, 512, true));
#endif
    sims = new PlaneSim[nthreads-1];
    threads = new PlaneSimThread[nthreads];
    int i;
    for (i = 0; i < nthreads-1; ++i) {
      sims[i].coBuild(sim);
      threads[i].sim = &(sims[i]);
    }
    threads[i].sim = &sim;
  }
#if HAVE_PROTOBUF
  if (configOut != NULL) {
    // If it already exists, read it and check that it is compatible with command line, otherwise write the file.
    Configuration config;
    if (configId >= 0) {
      config.set_config_id(configId);
    }
    config.set_voters(nvoters);
    assert(sim.they.numc == sim.candcount);
    config.set_choices(sim.they.numc);
    config.set_error(0.0);
    config.set_dimensions(2);
    config.set_voter_model(Configuration::CANDIDATE_COORDS);
    config.set_seats(sim.seats);
    for (int c = 0; c < sim.candcount; ++c) {
      config.add_candidate_coords(sim.candidates[c].x);
      config.add_candidate_coords(sim.candidates[c].y);
    }
    config.set_voter_sigma(sim.voterSigma);
    for (int s = 0; s < numVotingSystems; ++s) {
      config.add_system_names(systems[s]->name);
    }
    config.set_minx(sim.minx);
    config.set_miny(sim.miny);
    config.set_maxx(sim.maxx);
    config.set_maxy(sim.maxy);
    if (config.IsInitialized()) {
      int fd = open(configOut, O_WRONLY|O_CREAT, 0666);
      ZeroCopyOutputStream* raw_output = new FileOutputStream(fd);
      config.SerializeToZeroCopyStream(raw_output);
      delete raw_output;
      close(fd);
    } else {
      fprintf(stderr, "%s:%d set config fields but not Initialized\n",
              __FILE__, __LINE__);
      exit(1);
      return 1;
    }
  }
#endif
  if ( randomTests > 0 ) {
    runRandomTests(pblog_name, randomTests, nthreads, sim, threads);
    return 0;
  } else if ( nthreads <= 1 ) {
    sim.runXYSource( new XYSource(sim.px, sim.py) );
  } else {
    XYSource* source = new XYSource(sim.px, sim.py);
    for (int i = 0; i < nthreads; ++i) {
      threads[i].source = source;
    }
    for (int i = 0; i < nthreads; ++i) {
      pthread_create(&(threads[i].thread), NULL, runPlaneSimThread, &(threads[i]));
    }
    for (int i = 0; i < nthreads; ++i) {
      pthread_join(threads[i].thread, NULL);
    }
  }
  PlaneSimDraw* psd = new PlaneSimDraw(sim.px, sim.py, 3);
  char* configString = new char[1024];
  sim.configStr(configString, 1024);
  int* choiceXYpxPos = new int[sim.they.numc * 2];
  for (int i = 0; i < sim.they.numc; ++i) {
    choiceXYpxPos[i*2    ] = sim.xCoordToIndex(sim.candidates[i].x);
    choiceXYpxPos[i*2 + 1] = sim.yCoordToIndex(sim.candidates[i].y);
  }
  for (int vi = 0; vi < numVotingSystems; ++vi) {
    char* pfilename;
    if (planesPrefix) {
      static char planeStr[20];
      for (int plane = 0; plane < sim.they.numc; ++plane) {
        snprintf(planeStr, sizeof(planeStr), "%d", plane);
        pfilename = fileTemplate(planesPrefix, sim.systems[vi]->name, planeStr);
        psd->writePlanePNG(
            pfilename, plane, sim.accum[vi],
            sim.xCoordToIndex(sim.candidates[plane].x),
            sim.yCoordToIndex(sim.candidates[plane].y),
            configString);
        free(pfilename);
      }
      pfilename = fileTemplate(planesPrefix, sim.systems[vi]->name, "_sum.png");
      psd->writeSumPNG(pfilename, sim.they.numc, sim.accum[vi], choiceXYpxPos, configString, sim.electionsPerPixel * sim.seats);
      free(pfilename);
    }
    pfilename = fileTemplate(foutname, sim.systems[vi]->name, NULL);
    psd->writePNG( pfilename, sim.they.numc, sim.accum[vi], choiceXYpxPos, configString );
    free(pfilename);
  }
  delete [] configString;
}
