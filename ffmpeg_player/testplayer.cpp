#include "testplayer.h"
#include "player_widget.h"
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLineEdit>

#define  PLAYER_SIZE 4

testPlayer::testPlayer(QWidget *parent)
	: QWidget(parent)
{
	QVBoxLayout *vLayout = new QVBoxLayout(this);
	vLayout->setMargin(1);

	v = (player_widget**)malloc(sizeof(player_widget*) * PLAYER_SIZE);
	QGridLayout *gridLayout = new QGridLayout(this);
	gridLayout->setMargin(1);
	gridLayout->setSpacing(1);
	for (int i = 0; i < PLAYER_SIZE; ++i)
	{
		v[i] = new player_widget(this);
		gridLayout->addWidget(v[i], i/2, i%2);
	}
	vLayout->addLayout(gridLayout);

	pUrlEdit = new QLineEdit(this);
	pUrlEdit->setText("rtsp://47.104.83.137:10554/E529A17020011_1");
	pUrlEdit->setText("rtsp://www.kfchain.com:554/E529A17010225_2");

	QPushButton* pStart = new QPushButton(this);
	pStart->setText("start");
	connect(pStart, SIGNAL(released()), this, SLOT(start()));

	QPushButton* pStop = new QPushButton(this);
	pStop->setText("stop");
	connect(pStop, SIGNAL(released()), this, SLOT(stop()));

	QHBoxLayout *hLayout = new QHBoxLayout(this);
	hLayout->setMargin(1);
	hLayout->addWidget(pUrlEdit);
	hLayout->addWidget(pStart);
	hLayout->addWidget(pStop);
	vLayout->addLayout(hLayout);

	QHBoxLayout *hLayout1 = new QHBoxLayout(this);
	hLayout1->setMargin(1);

	pSubtitleEdit = new QLineEdit(this);
	pSubtitleEdit->setText(QString::fromLocal8Bit("车辆精细化综合运管系统 "));

	QPushButton* pSubtitle = new QPushButton(this);
	pSubtitle->setText("subtitle");
	connect(pSubtitle, SIGNAL(released()), this, SLOT(subtitle()));

	hLayout1->addWidget(pSubtitleEdit);
	hLayout1->addWidget(pSubtitle);

	vLayout->addLayout(hLayout1);

	this->resize(1282, 730);
}

testPlayer::~testPlayer()
{
	for (int i = 0; i < PLAYER_SIZE; ++i)
	{
		delete v[i];
		v[i] = NULL;
	}
	delete v;
	v = NULL;
}

void testPlayer::start()
{
	for (int i = 0; i < PLAYER_SIZE; ++i)
	{
		v[i]->start(pUrlEdit->text());
	}
}

void testPlayer::stop()
{
	for (int i = 0; i < PLAYER_SIZE; ++i)
	{
		v[i]->stop();
	}
}


void testPlayer::subtitle()
{
	for (int i = 0; i < PLAYER_SIZE; ++i)
	{
		v[i]->set_subtitle(pSubtitleEdit->text());
	}
}