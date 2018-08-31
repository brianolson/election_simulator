#include "arghandler.h"
#include "IRNRP.h"
#include "STV.h"
#include "Voter.h"

#include <assert.h>
#include <stdio.h>
#include <math.h>

class ProportionalConfig {
 public:
  int numFactions;
  int* factionSizes;
  int numVoters;
  int seats;
  // num choices = numFactions * seats


  void setFactions(VoterArray* they);
  void parseFactions(const char* factionString);
};

void ProportionalConfig::setFactions(VoterArray* they) {
  int factionCounts[numFactions];
  for (int fi = 0; fi < numFactions; fi++) {
    factionCounts[fi] = 0;
  }
  for (int vi = 0; vi < they->numv; vi++) {
    // Use the US House apportionment algorithm[1] to allocate next voter to a faction.
    // [1] https://en.wikipedia.org/wiki/United_States_congressional_apportionment
    double maxprio = -1.0;
    int maxfi = -1;
    for (int fi = 0; fi < numFactions; fi++) {
      double curCount = factionCounts[fi];
      double priority = double(factionSizes[fi]) / sqrt(curCount * (curCount + 1));
      if ((maxfi == -1) || (priority > maxprio)) {
        maxprio = priority;
        maxfi = fi;
      }
    }
    assert(maxfi >= 0);
    factionCounts[maxfi]++;

    // make a voter partisan to faction fi
    Voter& voter = (*they)[vi];
    for (int fi = 0; fi < numFactions; fi++) {
      if (fi == maxfi) {
        for (int si = 0; si < seats; si++) {
          voter.setPref((fi*seats) + si, 1.0 - (0.001 * si));
        }
      } else {
        for (int si = 0; si < seats; si++) {
          voter.setPref((fi*seats) + si, 0.0);
        }
      }
    }
  }
#if 0
  fprintf(stderr, "voters allocated:");
  for (int fi = 0; fi < numFactions; fi++) {
    fprintf(stderr, " %d", factionCounts[fi]);
  }
  fprintf(stderr, "\n");
#endif
}

void ProportionalConfig::parseFactions(const char* factionString) {
  int commas = 0;
  assert(factionString != NULL);
  const char* cp = factionString;
  while (*cp != '\0') {
    if (*cp == ',') {
      commas++;
    }
    cp++;
  }
  numFactions = commas+1;
  factionSizes = new int[numFactions];
  const char* sp = factionString;
  int outpos = 0;
  char* endptr;
  cp = factionString;
  while (true) {
    char c = *cp;
    if ((c == ',') || (c == '\0')) {
      long x = strtol(sp, &endptr, 10);
      if (endptr != cp) {
        fprintf(stderr, "error parsing faction string at chunk %d, did not parse '%c'\n", outpos, *endptr);
        exit(1);
      }
      factionSizes[outpos] = x;
      outpos++;
      if (c == '\0') {
        break;
      }
      sp = cp + 1;
      cp = sp + 1;
      continue;
    }
    cp++;
  }
}

void foo(ProportionalConfig* config) {
  VoterArray they;
  //ProportionalConfig* config = NULL;

  int numc = config->numFactions * config->seats;
  
  they.build(config->numVoters, numc);
  config->setFactions(&they);

  VotingSystem* algs[]{
    new STV(),
        new IRNRP(),
        NULL
  };
  const char* initArgs[] = {NULL};
  int* winners = new int[numc];
  for (int i = 0; algs[i] != NULL; i++) {
    VotingSystem* vs = algs[i];
    vs->init(initArgs);
    bool ok = vs->runMultiSeatElection(winners, they, config->seats);
    if (!ok) {
      fprintf(stderr, "error running %s\n", vs->name);
      exit(1);
    }
  }
};

int main( int argc, const char** argv ) {
  int argi = 1;
  const char* factionString = "60,40";
  ProportionalConfig config;
  config.numVoters = 1000;
  config.seats = 5;
  
  while (argi<argc) {
    IntArg("v", &config.numVoters);
    StringArg("f", &factionString);
    IntArg("seats", &config.seats);
  }
  config.parseFactions(factionString);
  foo(&config);
  return 0;
}
