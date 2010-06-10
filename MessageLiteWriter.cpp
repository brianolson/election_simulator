#include "MessageLiteWriter.h"

using google::protobuf::uint32;

MessageLiteWriter::~MessageLiteWriter(){}
MessageLiteReader::~MessageLiteReader(){}

CSMessageLiteWriter::CSMessageLiteWriter(
	CodedOutputStream* oat)
: out(oat)
{
}

CSMessageLiteWriter::~CSMessageLiteWriter(){}

bool CSMessageLiteWriter::writeMessage(const MessageLite* m) {
	out->WriteVarint32(m->ByteSize());
	return m->SerializeToCodedStream(out);
}

CSMessageLiteReader::CSMessageLiteReader(
	CodedInputStream* input)
: fin(input)
{
}

CSMessageLiteReader::~CSMessageLiteReader(){}

bool CSMessageLiteReader::readMessage(MessageLite* m) {
	uint32 size;
	bool ok = fin->ReadVarint32(&size);
	if (!ok) return ok;
	CodedInputStream::Limit l = fin->PushLimit(size);
	ok = m->ParseFromCodedStream(fin);
	fin->PopLimit(l);
	return ok;
}
