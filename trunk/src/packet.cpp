#include "packet.h"
using namespace std;

message::message() {
	message_type = -1;
	message_body = NULL;
	message_body_length = 0;
}

message_capabilities::message_capabilities() : message() {
	message_type = MSG_CAPABILITIES;
}

message_capabilities::~message_capabilities() {
	delete message_body;
}

void message_capabilities::set_capability(const string& capability, const string& value) {
	capabilities[capability] = value;
}

string message_capabilities::get_capability(const string& capability) const {
	if(capabilities.count(capability)) {
		map<string, string>::const_iterator i = capabilities.find(capability);
		if(i != capabilities.end()) {
			return i->second;
		}
	}
	return string("UNDEFINED");
}

message_capabilities::operator uint8*() {
	if(message_body) return message_body;
	message_body_length=0;
}
