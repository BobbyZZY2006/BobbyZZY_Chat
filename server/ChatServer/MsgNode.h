#pragma once
class MsgNode
{
public:
	MsgNode(size_t length);
	~MsgNode();
	void Clear();
	char* _data;
	size_t _totalLength;
	size_t _curLength;
};

class RecvNode :public MsgNode
{
	friend class LogicSystem;
public:
	RecvNode(size_t length, short msgid);

private:
	short _msgid;
};

class SendNode :public MsgNode {
	friend class LogicSystem;
public:
	SendNode(const char* msg,size_t length, short msgid);
private:
	short _msgid;
};