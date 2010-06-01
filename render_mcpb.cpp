#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/io/gzip_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/message_lite.h"
#include "MessageLiteWriter.h"
#include "PlaneSimDraw.h"
#include "ResultAccumulation.h"
#include "trial.pb.h"
#include "arghandler.h"

using google::protobuf::io::CodedInputStream;
using google::protobuf::io::FileInputStream;
using google::protobuf::io::GzipInputStream;
using google::protobuf::io::ZeroCopyInputStream;

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/**
   0      px-1
 |---|---|---|
minx        maxx
 */

#define xPixel(dx) (((dx) - minx) * (px) / (maxx - minx))
#define yPixel(dy) (((dy) - miny) * (py) / (maxy - miny))


int main(int argc, const char** argv) {
	const char* inname = "mc.pb";
	int px = 100;
	int py = 100;
	int numc = 4;
	double minx = -1.0;
	double miny = -1.0;
	double maxx = 1.0;
	double maxy = 1.0;
	const char* pfilename = "mc.png";
	
	int argi = 1;
	while (argi < argc) {
		IntArg("px", &px);
		IntArg("py", &py);
		IntArg("numc", &numc);
		DoubleArg("minx", &minx);
		DoubleArg("miny", &miny);
		DoubleArg("maxx", &maxx);
		DoubleArg("maxy", &maxy);
		StringArg("in", &inname);
		StringArg("out", &pfilename);
		fprintf(stderr, "bogus arg \"%s\"\n", argv[argi]);
		exit(1);
	}
	
	ResultAccumulation* accum = new ResultAccumulation(px, py, numc);
	
	int fd = open(inname, O_RDONLY);
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
		resultCount++;
		accum->incAccum(
			xPixel(result.coords(0)),
			yPixel(result.coords(1)),
			result.winners(0));
	}
	printf("read %d results\n", resultCount);
	
	// TODO: interpolate to fill in empty pixels.
	// TODO: emit list of empty pixels that should be targets for future sims.
	
	PlaneSimDraw* psd = new PlaneSimDraw(px, py, 3);
	psd->writePNG( pfilename, numc, accum, /*choiceXYpxPos*/ NULL, /*configString*/ NULL );
	delete psd;
	
	delete reader;
	delete input;
	delete gzis;
	delete fis;
	close(fd);
	return 0;
}
