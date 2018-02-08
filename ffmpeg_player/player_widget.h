#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QtWidgets/QWidget>
#include <QtWidgets/QBoxLayout>
class ffmpeg_player;

class player_widget : public QWidget
{
	Q_OBJECT

public:
	player_widget(QWidget* parent = 0);
	virtual ~player_widget();

public:
	void start(QString url);
	void stop();
	void set_subtitle(QString subtitle);

protected:
	virtual void paintEvent(QPaintEvent *event);
	virtual void resizeEvent(QResizeEvent *event);

private:
	static void printlog(void *ptr, int level, const char *fmt, va_list vl);

private:
	ffmpeg_player* m_pffmpeg_player;
};

#endif // VIDEOWIDGET_H
