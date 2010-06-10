#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/gzip_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/message_lite.h"
#include "MessageLiteWriter.h"
#include "PlaneSimDraw.h"
#include "ResultAccumulation.h"
#include "trial.pb.h"
#include "arghandler.h"
#include "file_template.h"

using google::protobuf::io::CodedInputStream;
using google::protobuf::io::FileInputStream;
using google::protobuf::io::GzipInputStream;
using google::protobuf::io::ZeroCopyInputStream;

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/**
   0     px-1
 |---|---|---|
minx		maxx
 */

#define xPixel(dx) (((dx) - minx) * (px) / (maxx - minx))
#define yPixel(dy) (((dy) - miny) * (py) / (maxy - miny))

bool readConfigFile(const char* configName, Configuration* config) {
	int fd = open(configName, O_RDONLY);
	if (fd < 0) {
		perror(configName);
		return false;
	}
	FileInputStream* fis = new FileInputStream(fd);
	assert(fis);
	bool ok = config->ParseFromZeroCopyStream(fis);
	delete fis;
	close(fd);
	return ok;
}

// spacegraph_util
char* fileTemplate(const char* tpl, const char* methodName, const char* plane);

char emptyString[] = "";
char* emptySystemNames[] = {emptyString, NULL};

int main(int argc, const char** argv) {
	const char* inname = "mc.pb";
	int px = 100;
	int py = 100;
	int numc = 4;
	double minx = -1.0;
	double miny = -1.0;
	double maxx = 1.0;
	double maxy = 1.0;
	//const char* pfilename = "mc.png";
	const char* foutname;
	const char* configName = NULL;
	int seats = 1;
	int* choiceXYpxPos = NULL;
	char** systemNames = emptySystemNames;
	int numVotingSystems = 1;
	const char* planesPrefix = NULL;
	const char* configString = "";
	
	int argi = 1;
	while (argi < argc) {
		StringArg("config", &configName);
		StringArg("in", &inname);
		IntArg("numc", &numc);
		DoubleArg("minx", &minx);
		DoubleArg("miny", &miny);
		DoubleArg("maxx", &maxx);
		DoubleArg("maxy", &maxy);
		StringArg("out", &foutname);
		IntArg("px", &px);
		IntArg("py", &py);
		StringArg("planes", &planesPrefix);
		fprintf(stderr, "bogus arg \"%s\"\n", argv[argi]);
		exit(1);
	}
	
	if (configName != NULL) {
		Configuration config;
		if (readConfigFile(configName, &config)) {
			assert(config.dimensions() == 2);
			assert(config.voter_model() == Configuration::CANDIDATE_COORDS);
			seats = config.seats();
			// TODO: implement multiseat rendering.
			assert(seats == 1);
			numc = config.choices();
			minx = config.minx();
			miny = config.miny();
			maxx = config.maxx();
			maxy = config.maxy();
			choiceXYpxPos = new int[numc * 2];
			assert(numc == config.candidate_coords_size()/2);
			for (int i = 0; i < numc; ++i) {
				choiceXYpxPos[i*2] = xPixel(config.candidate_coords(i*2));
				choiceXYpxPos[i*2 + 1] = yPixel(config.candidate_coords(i*2 + 1));
			}
			if (config.system_names_size() > 0) {
				numVotingSystems = config.system_names_size();
				systemNames = new char*[numVotingSystems];
				for (int nv = 0; nv < numVotingSystems; ++nv) {
					systemNames[nv] = strdup(config.system_names(nv).c_str());
				}
			}
		} else {
			fprintf(stderr, "%s: read config file failed\n", configName);
			exit(1);
		}
	}

	ResultAccumulation** accum = new ResultAccumulation*[numVotingSystems];
	for (int nv = 0; nv < numVotingSystems; ++nv) {
	  accum[nv] = new ResultAccumulation(px, py, numc);
	}
	
	int fd = open(inname, O_RDONLY);
	if (fd < 0) {
		perror(inname);
		exit(1); return 1;
	}
	assert(fd >= 0);
	FileInputStream* fis = new FileInputStream(fd);
	assert(fis);
	GzipInputStream* gzis = new GzipInputStream(fis);
	assert(gzis);
	CodedInputStream* input = new CodedInputStream(gzis);
	assert(input);
	CSMessageLiteReader* reader = new CSMessageLiteReader(input);
	assert(reader);
	
	Result2 result;
	int resultCount = 0;
	while (reader->readMessage(&result)) {
		int systemIndex = 0;
		if (result.has_system()) {
			systemIndex = result.system();
			assert(systemIndex < numVotingSystems);
		}
		resultCount++;
		for (int wi = 0; wi < seats; ++wi) {
			accum[systemIndex]->incAccum(
				xPixel(result.coords(0)),
				yPixel(result.coords(1)),
				result.winners(wi));
		}
	}
	printf("read %d results\n", resultCount);
	
	// TODO: interpolate to fill in empty pixels.
	// TODO: emit list of empty pixels that should be targets for future sims.
	
	PlaneSimDraw* psd = new PlaneSimDraw(px, py, 3);
		printf("planesPrefix=%s, numc=%d\n", planesPrefix, numc);
	for (int vi = 0; vi < numVotingSystems; ++vi) {
		char* pfilename;
		if (planesPrefix) {
			static char planeStr[20];
			for (int plane = 0; plane < numc; ++plane) {
				snprintf(planeStr, sizeof(planeStr), "%d", plane);
				pfilename = fileTemplate(planesPrefix, systemNames[vi], planeStr);
				//printf("plane: %s\n", pfilename);
				psd->writePlanePNG(
					pfilename, plane, accum[vi],
					choiceXYpxPos[plane*2],
					choiceXYpxPos[plane*2 + 1],
					configString);
				free(pfilename);
			}
			pfilename = fileTemplate(planesPrefix, systemNames[vi], "_sum.png");
			//psd->writeSumPNG(pfilename, numc, accum[vi], choiceXYpxPos, configString);
			free(pfilename);
		}
		pfilename = fileTemplate(foutname, systemNames[vi], NULL);
		psd->writePNG( pfilename, numc, accum[vi], choiceXYpxPos, /*configString*/ NULL );
		free(pfilename);
	}
	delete psd;
	
	delete reader;
	delete input;
	delete gzis;
	delete fis;
	close(fd);
	return 0;
}
