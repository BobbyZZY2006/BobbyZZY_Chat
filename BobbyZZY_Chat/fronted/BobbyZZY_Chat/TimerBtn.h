#pragma once
#include <QPushButton>
#include <QTimer>
#include "global.h"
class TimerBtn : public QPushButton
{
public:
	TimerBtn(QWidget* parent = nullptr);
	~TimerBtn();
	virtual void mousePressEvent(QMouseEvent* e) override;
	void changeEvent();
private:
	QTimer *_timer;
	int _count;
};

