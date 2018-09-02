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

// US House apportionment algorithm[1] to allocate next voter to a faction.
// [1] https://en.wikipedia.org/wiki/United_States_congressional_apportionment
void apportion(int* out, int* populations, int len, int seats) {
  for (int fi = 0; fi < len; fi++) {
    out[fi] = 0;
  }
  for (int vi = 0; vi < seats; vi++) {
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
}

class PRAllocatePopStat {
 public:
  int index;
  int pop;
  int D;
  int apportioned;
  PRAllocatePopStat()
      : index(0), pop(0), D(0), apportioned(0)
  {}
  PRAllocatePopStat(int index, int pop)
      : index(index), pop(pop), D(0), apportioned(0)
  {}
};

// modified from bresenham's algorithm for linear interpolation
// https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
//
// Bresenham's algorithm is frequently applied as a computer
// graphics method for drawing a line of pixels from 0,0 to x,y. In
// this case 'y' is the population of the most populous set (state,
// district, proportional constituency, etc), 'x' is the population
// of some other set. If we had as many seats as there were people,
// we would step through this algorithm and draw a line all the way
// to the end. Instead we count how many steps we have taken (seats
// apportioned) and stop when we hit the seats limit. Because
// Bresenham's algorithm closely follows the line we have a fair
// linear interpolation constrained by integer steps.
void proportionallyAllocate(int* out, int* populations, int len, int seats) {
  PRAllocatePopStat state[len];
  for (int i = 0; i < len; i++) {
    state[i].pop = populations[i];
    state[i].index = i;
  }
  PRAllocatePopStat* mostPopulous = &(state[0]);
  for (int i = 1; i < len; i++) {
    if (state[i].pop > mostPopulous->pop) {
      mostPopulous = &(state[i]);
    }
  }
  PRAllocatePopStat* everyoneElse[len-1];
  int outpos = 0;
  for (int i = 0; i < len; i++) {
    if (state[i].index == mostPopulous->index) {
      continue;
    }
    everyoneElse[outpos] = &(state[i]);
    everyoneElse[outpos]->D = (2 * everyoneElse[outpos]->pop) - mostPopulous->pop;
    outpos++;
  }
  assert(outpos == len-1);

  int apportionedSum = 0;
  while (true) {
    //fprintf(stderr, "PR apportion to %d (max pop)\n", mostPopulous->index);
    mostPopulous->apportioned++;
    apportionedSum++;
    if (apportionedSum >= seats) {
      //fprintf(stderr, "done\n");
      break;
    }
    // sort by D to fing which other group is furthest behind the line
    // and nedes to be caught up by adding to its apportionment.
    bool notSorted = true;
    while (notSorted) {
      notSorted = false;
      for (int i = 1; i < len - 1; i++) {
        if (everyoneElse[i-1]->D < everyoneElse[i]->D) {
          PRAllocatePopStat* t = everyoneElse[i];
          everyoneElse[i] = everyoneElse[i-1];
          everyoneElse[i-1] = t;
          notSorted = true;
        }
      }
    }
    for (int i = 0; i < len - 1; i++) {
      //fprintf(stderr, "ee[%d] i=%d pop=%d D=%d a=%d\n", i, everyoneElse[i]->index, everyoneElse[i]->pop, everyoneElse[i]->D, everyoneElse[i]->apportioned);
      if (everyoneElse[i]->D > 0) {
        //fprintf(stderr, "PR apportion to %d\n", everyoneElse[i]->index);
        everyoneElse[i]->apportioned++;
        apportionedSum++;
        if (apportionedSum >= seats) {
          //fprintf(stderr, "done\n");
          break;
        }
        everyoneElse[i]->D -= mostPopulous->pop;
      }
      everyoneElse[i]->D += (2 * everyoneElse[i]->pop);
    }
    if (apportionedSum >= seats) {
      break;
    }
  }
  for (int i = 0; i < len; i++) {
    out[i] = state[i].apportioned;
  }
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

  idealFactionWinners = new int[numFactions];
  //apportion(idealFactionWinners, factionPopulations, numFactions, seats);
  proportionallyAllocate(idealFactionWinners, factionPopulations, numFactions, seats);
#if 01
  fprintf(stderr, "voter populations:");
  for (int fi = 0; fi < numFactions; fi++) {
    fprintf(stderr, " %8d", factionPopulations[fi]);
  }
  fprintf(stderr, "\n");
  fprintf(stderr, " voters allocated:");
  for (int fi = 0; fi < numFactions; fi++) {
    fprintf(stderr, " %8d", idealFactionWinners[fi]);
  }
  fprintf(stderr, "\n");
  // TODO: what is fair? apportionment algorithm? equalizing voter power?
  fprintf(stderr, "      voter power:");
  for (int fi = 0; fi < numFactions; fi++) {
    fprintf(stderr, " %8.4g", double(idealFactionWinners[fi]) / double(factionPopulations[fi]));
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
