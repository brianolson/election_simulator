#include "MessageLiteWriter.h"

CSMessageLiteWriter::CSMessageLiteWriter(
	google::protobuf::io::CodedOutputStream* oat)
: out(oat)
{
}

bool CSMessageLiteWriter::writeMessage(const google::protobuf::MessageLite* m) {
	out->WriteVarint32(m->ByteSize());
	return m->SerializeToCodedStream(out);
}

CSMessageLiteReader::CSMessageLiteReader(
	google::protobuf::io::CodedInputStream* input)
: fin(input)
{
}

bool CSMessageLiteReader::readMessage(google::protobuf::MessageLite* m) {
	google::protobuf::uint32 size;
	bool ok = fin->ReadVarint32(&size);
	if (!ok) return ok;
	google::protobuf::io::CodedInputStream::Limit l = fin->PushLimit(size);
	ok = m->ParseFromCodedStream(fin);
	fin->PopLimit(l);
	return ok;
}
