/********** tell emacs we use -*- c++ -*- style comments *******************
 $Revision: 1.1 $  $Author: trey $  $Date: 2007-03-05 08:58:26 $

 @file    zmdpRockExplore.cc
 @brief   No brief

 Copyright (c) 2002-2006, Trey Smith.  All rights reserved.

 Licensed under the Apache License, Version 2.0 (the "License"); you may
 not use this file except in compliance with the License.  You may
 obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 implied.  See the License for the specific language governing
 permissions and limitations under the License.

 ***************************************************************************/

#include <assert.h>
#include <sys/time.h>
#include <getopt.h>
#include <signal.h>

#include <iostream>
#include <fstream>

#include "RockExplore.h"
#include "zmdpCommonTime.h"
//#include "solverUtils.h"
#include "zmdpMainConfig.h"

#include "zmdpMainConfig.cc" // embed default config file

using namespace std;
//using namespace MatrixUtils;
using namespace zmdp;

bool userTerminatedG = false;

void sigIntHandler(int sig) {
  userTerminatedG = true;

  printf("*** received SIGINT, user pressed control-C ***\n"
	 "terminating run and writing output policy as soon as the solver returns control\n");
  fflush(stdout);
}

void setSignalHandler(int sig, void (*handler)(int)) {
  struct sigaction act;
  memset (&act, 0, sizeof(act));
  act.sa_handler = handler;
  act.sa_flags = SA_RESTART;
  if (-1 == sigaction (sig, &act, NULL)) {
    cerr << "ERROR: unable to set handler for signal "
         << sig << endl;
    exit(EXIT_FAILURE);
  }
}

#if 0
void doSolve(const ZMDPConfig& config, SolverParams& p)
{
  init_matrix_utils();
  StopWatch run;

  printf("%05d reading model file and allocating data structures\n",
	 (int) run.elapsedTime());
  SolverObjects so;
  constructSolverObjects(so, p, config);

  // initialize the solver
  printf("%05d calculating initial heuristics\n",
	 (int) run.elapsedTime());
  so.solver->planInit(so.sim->getModel(), &config);
  printf("%05d finished initialization, beginning to improve policy\n",
	 (int) run.elapsedTime());
  
  setSignalHandler(SIGINT, &sigIntHandler);

  double lastPrintTime = -1000;
  bool reachedTargetPrecision = false;
  bool reachedTimeout = false;
  int numSolverCalls = 0;
  while (!(reachedTargetPrecision || reachedTimeout || userTerminatedG)) {
    // make a call to the solver
    reachedTargetPrecision =
      so.solver->planFixedTime(so.sim->getModel()->getInitialState(),
			       /* maxTime = */ -1, p.terminateRegretBound);
    numSolverCalls++;

    // check timeout
    double elapsed = run.elapsedTime();
    if (elapsed >= p.terminateWallclockSeconds) {
      reachedTimeout = true;
    }

    // print a progress update every 10 seconds
    if ((elapsed - lastPrintTime > 10)
	|| reachedTargetPrecision || reachedTimeout || userTerminatedG) {
      ValueInterval intv = so.solver->getValueAt(so.sim->getModel()->getInitialState());
      printf("%05d %6d calls to solver, bounds [%8.4f .. %8.4f], regret <= %g\n",
	     (int) elapsed, numSolverCalls, intv.l, intv.u, (intv.u - intv.l));
      lastPrintTime = elapsed;
    }
  }

  // say why the run ended
  if (reachedTargetPrecision) {
    printf("%05d terminating run; reached target regret bound of %g\n",
	   (int) run.elapsedTime(), p.terminateRegretBound);
  } else if (reachedTimeout) {
    printf("%05d terminating run; passed specified timeout of %g seconds\n",
	   (int) run.elapsedTime(), p.terminateWallclockSeconds);
  } else {
    printf("%05d terminating run; caught SIGINT from user\n",
	   (int) run.elapsedTime());
  }

  // write out a policy
  if (NULL == p.policyOutputFile) {
    printf("%05d (not outputting policy)\n", (int) run.elapsedTime());
  } else {
    printf("%05d writing policy to '%s'\n", (int) run.elapsedTime(), p.policyOutputFile);
    so.bounds->writePolicy(p.policyOutputFile);
  }

  // finish up logging (if any, according to params specified in the config file)
  printf("%05d finishing logging (e.g., writing qValuesOutputFile if it was requested)\n",
	 (int) run.elapsedTime());
  so.solver->finishLogging();

  printf("%05d done\n", (int) run.elapsedTime());
}
#endif

void usage(const char* cmdName) {
  cerr <<
    "usage: " << cmdName << " OPTIONS <model>\n"
    "  -h or --help           Print this help\n"
    "  --version              Print version information (CFLAGS used at compilation)\n"
    "  -c or --config <file>  Specify a config file to read options from\n"
    "  --genConfig <file>     Generate an example config file and exit\n"
    "\n"
    "  zmdpRockExplore generates an output policy for a search strategy and\n"
    "  problem you select.  It runs the search strategy in an anytime\n"
    "  fashion, periodically printing bounds on the value of the initial\n"
    "  state to console so that you can monitor progress.  When the run ends,\n"
    "  the final policy is output to the file you specify.  There are several\n"
    "  options for how to end the run: you can specify a desired regret bound\n"
    "  for the output solution, specify a fixed timeout, or just use ctrl-C to\n"
    "  interrupt the algorithm when you are satisfied (it will output the final\n"
    "  policy before exiting).\n"
    "\n"
    "  ZMDP gets configuration information from three places: (1) Default values\n"
    "  are embedded in the binary at compile time.  (2) If you specify a config file\n"
    "  using the --config option, any fields set in that file override the defaults.\n"
    "  (3) You can override individual fields in the config file from the command line.\n"
    "  For instance, if the config file specifies 'searchStrategy frtdp' you can override\n"
    "  with '--searchStrategy hsvi' at the command line.\n"
    "\n"
    "  To generate an example config file (including default values and comments describing\n"
    "  all the parameters), use the '--genConfig <file>' option.\n"
    "\n"
    "For convenience, there are also some abbreviations:\n"
    "  -s = --searchStrategy\n"
    "  -t = --modelType\n"
    "  -o = --policyOutputFile\n"
    "  -f = --useFastModelParser 1\n"
    "  -p = --terminateRegretBound\n"
    "\n"
    "Examples:\n"
    "  " << cmdName << " RockSample_4_4.pomdp\n"
    "  " << cmdName << " large-b.racetrack\n"
    "  " << cmdName << " -t 60 -o my.policy RockSample_4_4.pomdp\n"
    "  " << cmdName << " -s lrtdp RockSample_4_4.pomdp\n"
    "  " << cmdName << " -f RockSample_5_7.pomdp\n"
    "\n"
;
  exit(-1);
}

int main(int argc, char **argv) {
  RockExplore model;
  RockExploreParams& p = model.params;

  p.width = 4;
  p.height = 4;
  p.initPos = RockExplorePos(0,2);
  p.rockGoodPrior = 0.7;
  p.costMove = -1;
  p.costCheck = -1;
  p.costIllegal = -100;
  p.costSampleBad = -10;
  p.rewardSampleGood = 10;
  p.rewardExit = 10;
  p.discountFactor = 0.95;

  p.numRocks = 4;
  p.rockPos.push_back(RockExplorePos(1,3));
  p.rockPos.push_back(RockExplorePos(2,1));
  p.rockPos.push_back(RockExplorePos(3,1));
  p.rockPos.push_back(RockExplorePos(1,0));

  RockExploreState s;
  s.isTerminalState = false;
  s.robotPos = p.initPos;
  s.rockIsGood.push_back(true);
  s.rockIsGood.push_back(false);
  s.rockIsGood.push_back(false);
  s.rockIsGood.push_back(true);

  std::vector<double> probRockIsGood;
  probRockIsGood.push_back(0.8);
  probRockIsGood.push_back(0.2);
  probRockIsGood.push_back(0.1);
  probRockIsGood.push_back(0.9);

  string rmap;
  model.getMap(rmap, s, probRockIsGood);
  cout << rmap << endl;

#if 0
  SolverParams p;

  ostringstream outs;
  if (zmdpDebugLevelG >= 1) {
    // save arguments for debug printout later
    for (int i=1; i < argc; i++) {
      outs << argv[i] << " ";
    }
  }

  bool argsOnly = false;
  const char* configFileName = NULL;
  ZMDPConfig commandLineConfig;

  p.cmdName = argv[0];
  for (int argi=1; argi < argc; argi++) {
    std::string args = argv[argi];
    if (!argsOnly && '-' == args[0]) {
      if (args == "--") {
	argsOnly = true;
      } else if (args == "-h" || args == "--help") {
	usage(argv[0]);
      } else if (args == "--version") {
	cout << "CFLAGS = " << CFLAGS << endl;
	exit(EXIT_SUCCESS);
      } else if (args == "-c" || args == "--config") {
	if (++argi == argc) {
	  fprintf(stderr, "ERROR: found -c option without argument (use -h for help)\n");
	  exit(EXIT_FAILURE);
	}
	configFileName = argv[argi];
      } else if (args == "--genConfig") {
	if (++argi == argc) {
	  fprintf(stderr, "ERROR: found --genConfig option without argument (use -h for help)\n");
	  exit(EXIT_FAILURE);
	}
	embedWriteToFile(argv[argi], defaultConfig);
	printf("wrote config file with default settings to %s\n", argv[argi]);
	exit(EXIT_SUCCESS);
      } else {
	// replace abbreviations
	if (args == "-s") {
	  args = "--searchStrategy";
	} else if (args == "-t") {
	  args = "--modelType";
	} else if (args == "-f") {
	  commandLineConfig.setBool("useFastModelParser", true);
	  continue;
	} else if (args == "-p") {
	  args = "--terminateRegretBound";
	} else if (args == "-o") {
	  args = "--policyOutputFile";
	}

	if (args.find("--") != 0) {
	  fprintf(stderr, "ERROR: found unknown option '%s' (use -h for help)\n",
		  args.c_str());
	  exit(EXIT_FAILURE);
	} else {
	  if (++argi == argc) {
	    fprintf(stderr, "ERROR: found %s option without argument (-h for help)\n",
		    args.c_str());
	    exit(EXIT_FAILURE);
	  }
	  commandLineConfig.setString(args.substr(2), argv[argi]);
	}
      }
    } else {
      if (NULL == p.probName) {
	p.probName = argv[argi];
      } else {
	fprintf(stderr, "ERROR: expected exactly 1 argument (use -h for help)\n");
	exit(EXIT_FAILURE);
      }
    }
  }
  if (NULL == p.probName) {
    fprintf(stderr, "ERROR: expected exactly 1 argument (use -h for help)\n");
    exit(EXIT_FAILURE);
  }

  // config step 1: read defaults embedded in binary
  ZMDPConfig config;
  config.readFromString("<defaultConfig>", defaultConfig.data);

  // config step 2: overwrite defaults with values specified in config file
  // (signal an error if any new unexpected fields are defined)
  config.setOverWriteOnlyMode(true);
  if (NULL != configFileName) {
    config.readFromFile(configFileName);
  }

  // config step 3: overwrite with values specified on command line
  config.readFromConfig(commandLineConfig);

  if (zmdpDebugLevelG >= 1) {
    printf("CFLAGS = %s\n", CFLAGS);
    printf("ARGS = %s\n", outs.str().c_str());
    fflush(stdout);
  }

  p.setValues(config);
  doSolve(config, p);
#endif

  return 0;
}

/***************************************************************************
 * REVISION HISTORY:
 * $Log: not supported by cvs2svn $
 *
 ***************************************************************************/