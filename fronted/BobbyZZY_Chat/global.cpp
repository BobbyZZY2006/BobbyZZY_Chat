#include "global.h"

QString gate_url_prefix = "";
std::function<void(QWidget*)> repolish = [](QWidget* w) {
	w->style()->unpolish(w);
	w->style()->polish(w);
};

std::function<QString(QString)> xorString = [](QString s) {
	int length = s.length() % 255;
	for (size_t i = 0; i < length; i++)
	{
		s[i] = static_cast<ushort>(s[i].unicode() ^ (length));
	}
	return s;
	};