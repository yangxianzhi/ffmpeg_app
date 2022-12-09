#ifndef TESTPLAYER_H
#define TESTPLAYER_H

#include <QtWidgets/QWidget>
class player_widget;
class QLineEdit;
class testPlayer : public QWidget
{
	Q_OBJECT

public:
	testPlayer(QWidget *parent = 0);
	~testPlayer();

private slots:
	void start();
	void stop();
	void subtitle();

private:
	player_widget **v;
	QLineEdit *pUrlEdit;
	QLineEdit *pSubtitleEdit;
};

#endif // TESTPLAYER_H
