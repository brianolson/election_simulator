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
  // configured unitless ratio: e.g. [60,30,10]; need not sum to 100
  int* factionSizes;
  // how many voters were apportioned into each faction
  int* factionPopulations;
  // given voter populations, how many of each should win?
  int* idealFactionWinners;
  int numVoters;
  int seats;
  // num choices = numFactions * seats

  ProportionalConfig();
  ~ProportionalConfig();
  void setFactions(VoterArray* they);
  void parseFactions(const char* factionString);

  // TODO: implement
  int winnerProportionalityError(int* winners);
};

ProportionalConfig::ProportionalConfig()
    : numFactions(0),
      factionSizes(NULL),
      factionPopulations(NULL),
      idealFactionWinners(NULL)
{}

ProportionalConfig::~ProportionalConfig() {
  if (factionSizes != NULL) {
    delete [] factionSizes;
    factionSizes = NULL;
  }
  if (factionPopulations != NULL) {
    delete [] factionPopulations;
    factionPopulations = NULL;
  }
  if (idealFactionWinners != NULL) {
    delete [] idealFactionWinners;
    idealFactionWinners = NULL;
  }
}

void apportion(int* out, int* populations, int len, int seats) {
  for (int fi = 0; fi < len; fi++) {
    out[fi] = 0;
  }
  for (int vi = 0; vi < seats; vi++) {
    // Use the US House apportionment algorithm[1] to allocate next voter to a faction.
    // [1] https://en.wikipedia.org/wiki/United_States_congressional_apportionment
    double maxprio = -1.0;
    int maxfi = -1;
    for (int fi = 0; fi < len; fi++) {
      double curCount = out[fi];
      // TODO: this denominator gives infinite priority to any group with zero seats, which actually _isn't_ fair. Fine for states, not good for PR.
      double priority = double(populations[fi]) / sqrt(curCount * (curCount + 1));
      if ((maxfi == -1) || (priority > maxprio)) {
        maxprio = priority;
        maxfi = fi;
      }
    }
    assert(maxfi >= 0);
    out[maxfi]++;
  }
#if 01
  fprintf(stderr, "voter populations:");
  for (int fi = 0; fi < len; fi++) {
    fprintf(stderr, " %8d", populations[fi]);
  }
  fprintf(stderr, "\n");
  fprintf(stderr, " voters allocated:");
  for (int fi = 0; fi < len; fi++) {
    fprintf(stderr, " %8d", out[fi]);
  }
  fprintf(stderr, "\n");
  // TODO: what is fair? apportionment algorithm? equalizing voter power?
  fprintf(stderr, "      voter power:");
  for (int fi = 0; fi < len; fi++) {
    fprintf(stderr, " %0.6g", double(out[fi]) / double(populations[fi]));
  }
  fprintf(stderr, "\n");
#endif
}

void ProportionalConfig::setFactions(VoterArray* they) {
  factionPopulations = new int[numFactions];
  for (int fi = 0; fi < numFactions; fi++) {
    factionPopulations[fi] = 0;
  }
  for (int vi = 0; vi < they->numv; vi++) {
    // Use the US House apportionment algorithm[1] to allocate next voter to a faction.
    // [1] https://en.wikipedia.org/wiki/United_States_congressional_apportionment
    double maxprio = -1.0;
    int maxfi = -1;
    for (int fi = 0; fi < numFactions; fi++) {
      double curCount = factionPopulations[fi];
      double priority = double(factionSizes[fi]) / sqrt(curCount * (curCount + 1));
      if ((maxfi == -1) || (priority > maxprio)) {
        maxprio = priority;
        maxfi = fi;
      }
    }
    assert(maxfi >= 0);
    factionPopulations[maxfi]++;

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
    fprintf(stderr, " %d", factionPopulations[fi]);
  }
  fprintf(stderr, "\n");
#endif

  idealFactionWinners = new int[numFactions];
  apportion(idealFactionWinners, factionPopulations, numFactions, seats);
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

int ProportionalConfig::winnerProportionalityError(int* winners) {
  int winnersByFaction[numFactions];
  int errcount = 0;
  for (int fi = 0; fi < numFactions; fi++) {
    winnersByFaction[fi] = 0;
  }
  for (int wi = 0; wi < seats; wi++) {
    int f = winners[wi] / seats;
    winnersByFaction[f]++;
  }
  fprintf(stderr, " ideal:");
  for (int fi = 0; fi < numFactions; fi++) {
    fprintf(stderr, " %d", idealFactionWinners[fi]);
  }
  fprintf(stderr, "\nactual:");
  for (int fi = 0; fi < numFactions; fi++) {
    fprintf(stderr, " %d", winnersByFaction[fi]);
  }
  fprintf(stderr, "\n");
  for (int fi = 0; fi < numFactions; fi++) {
    errcount += abs(idealFactionWinners[fi] - winnersByFaction[fi]);
  }
  return errcount;
}

void foo(ProportionalConfig* config) {
  VoterArray they;

  int numc = config->numFactions * config->seats;

  they.build(config->numVoters, numc);
  config->setFactions(&they);

  VotingSystem* algs[]{
    new STV(),
        new IRNRP(),
        NULL
  };
  const char* initArgs[] = {NULL}; // "debug=/tmp/pdebug",
  int* winners = new int[numc];
  for (int i = 0; algs[i] != NULL; i++) {
    VotingSystem* vs = algs[i];
    vs->init(initArgs);
    bool ok = vs->runMultiSeatElection(winners, they, config->seats);
    if (!ok) {
      fprintf(stderr, "error running %s\n", vs->name);
      exit(1);
    }
    int prErr = config->winnerProportionalityError(winners);
    fprintf(stderr, "%s: proportionality error=%d\n", vs->name, prErr);
    fprintf(stderr, "winners (%d seats):", config->seats);
    for (int c = 0; c < numc; c++) {
      fprintf(stderr, " %d", winners[c]);
    }
    fprintf(stderr, "\n");
  }
  delete [] winners;
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
