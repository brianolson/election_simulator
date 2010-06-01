#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/message_lite.h"

class MessageLiteWriter {
public:
	virtual bool writeMessage(const google::protobuf::MessageLite*) = 0;
};

class MessageLiteReader {
public:
	virtual bool readMessage(google::protobuf::MessageLite*) = 0;
};

class CSMessageLiteWriter : public MessageLiteWriter {
public:
	CSMessageLiteWriter(google::protobuf::io::CodedOutputStream*);
	virtual bool writeMessage(const google::protobuf::MessageLite* m);
	
protected:
	google::protobuf::io::CodedOutputStream* out;
};

class CSMessageLiteReader : public MessageLiteReader {
public:
	CSMessageLiteReader(google::protobuf::io::CodedInputStream*);
	virtual bool readMessage(google::protobuf::MessageLite*);
	
protected:
	google::protobuf::io::CodedInputStream* fin;
};
