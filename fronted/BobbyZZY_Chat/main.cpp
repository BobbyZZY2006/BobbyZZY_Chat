#include "mainwindow.h"
#include "global.h"
#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowIcon(QIcon(":/res/login.png"));
    QFile qss(":/style/stylesheet.qss");
    if(qss.open(QFile::ReadOnly)){
        qDebug("open success");
        QString style=QLatin1String(qss.readAll());
        qApp->setStyleSheet(style);
        qss.close();
    }else{
        qDebug("open failed");
    }
    QString filename = "config.ini";
	QString app_path = QApplication::applicationDirPath();
	QString config_path=QDir::toNativeSeparators(app_path + QDir::separator() + filename);
	qDebug("config path: %s", qPrintable(config_path));
	QSettings settings(config_path, QSettings::IniFormat);
	QString gate_host = settings.value("GateServer/host").toString();
	QString gate_port = settings.value("GateServer/port").toString();
	gate_url_prefix = "http://" + gate_host + ":" + gate_port;
	qDebug("gate_url_prefix: %s", qPrintable(gate_url_prefix));
    w.show();
    return a.exec();
}
