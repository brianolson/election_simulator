#ifndef MESSAGE_LITE_WRITER_H
#define MESSAGE_LITE_WRITER_H

#include "google/protobuf/io/coded_stream.h"
#include "google/protobuf/message_lite.h"

using google::protobuf::MessageLite;
using google::protobuf::io::CodedOutputStream;
using google::protobuf::io::CodedInputStream;

class MessageLiteWriter {
public:
	virtual ~MessageLiteWriter();
	virtual bool writeMessage(const MessageLite*) = 0;
};

class MessageLiteReader {
public:
	virtual ~MessageLiteReader();
	virtual bool readMessage(MessageLite*) = 0;
};

class CSMessageLiteWriter : public MessageLiteWriter {
public:
	CSMessageLiteWriter(CodedOutputStream*);
	virtual ~CSMessageLiteWriter();
	virtual bool writeMessage(const MessageLite* m);
	
protected:
	CodedOutputStream* out;
};

class CSMessageLiteReader : public MessageLiteReader {
public:
	CSMessageLiteReader(CodedInputStream*);
	virtual ~CSMessageLiteReader();
	virtual bool readMessage(MessageLite*);
	
protected:
	CodedInputStream* fin;
};

#endif /* MESSAGE_LITE_WRITER_H */
