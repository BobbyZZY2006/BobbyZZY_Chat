#include "MsgNode.h"
#include "const.h"
MsgNode::MsgNode(size_t length)
{
	_data = new char[length];
	_totalLength = length;
	_curLength = 0;
}

void MsgNode::Clear()
{
	_curLength = 0;
	memset(_data, 0, _totalLength);
}

MsgNode::~MsgNode()
{
	if (_data) {
		delete[] _data;
		_data = nullptr;
	}
}

RecvNode::RecvNode(size_t length,short msgid)
	:MsgNode(length), _msgid(msgid)
{
}

SendNode::SendNode(const char* msg, size_t length, short msgid)
	:MsgNode(length + MSG_HEAD_LENGTH), _msgid(msgid)
{
	short msgid_net = htons(msgid);
	memcpy(_data, &msgid_net, MSG_ID_LENGTH);
	short length_net = htons(length);
	memcpy(_data + MSG_ID_LENGTH, &length_net, MSG_LENGTH_LENGTH);
	memcpy(_data + MSG_HEAD_LENGTH, msg, length);
	_curLength = length + MSG_HEAD_LENGTH;
}