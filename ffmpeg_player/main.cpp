
#include <QtWidgets/QApplication>
#include <QtCore/QDebug>
#include "player_widget.h"
#include "testplayer.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	testPlayer w;
	w.show();
	return a.exec();
}
