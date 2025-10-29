#include "TimerBtn.h"
#include <QMouseEvent>
#include "registerdialog.h"
TimerBtn::TimerBtn(QWidget* parent) :QPushButton(parent), _count(5) {
	_timer = new QTimer(this);
	connect(_timer, &QTimer::timeout, this, [this]() {
		if (_count == 0) {
			_timer->stop();
			_count = 10;
			this->setText("获取");
			this->setEnabled(true);
			this->setStyleSheet(
				"QPushButton { color: black; background: none; }"
				"QPushButton:hover { background: #e0e0e0; }"
				"QPushButton:pressed { background: #b0b0b0; }"
			);
			return;
		}
		this->setText(QString::number(_count));
		_count--;
		});
}

TimerBtn::~TimerBtn() {
	if (_timer->isActive()) {
		_timer->stop();
	}
}

void TimerBtn::mousePressEvent(QMouseEvent* e) {
	if (e->button() == Qt::LeftButton) {
		// 在这里处理鼠标左键释放事件
		qDebug() << "MyButton was released!";
		emit clicked();
	}
	QPushButton::mousePressEvent(e);
}

void TimerBtn::changeEvent() {
	qDebug() << "MyButton changed";
	this->setEnabled(false);
	this->setText(QString::number(_count));
	_timer->start(1000);
}