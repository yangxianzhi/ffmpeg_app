#include "player_widget.h"
#include "ffmpeg_player.h"
#include <QtCore/QEvent>
#include <QtCore/QDebug>
#include <QtWidgets/QStyleOption>
#include <QtGui/QPainter>
#include <QtGui/QtEvents>

player_widget::player_widget(QWidget* parent /* = 0 */)
	:QWidget(parent)
{
 	setAttribute(Qt::WA_NativeWindow);
 
 	/* Make this widget a bit "lighter" */
 	setAttribute(Qt::WA_PaintOnScreen);
 	setAttribute(Qt::WA_NoSystemBackground);

	this->setStyleSheet(QStringLiteral("background-color: rgb(40, 30, 30);"));

	m_pffmpeg_player = new ffmpeg_player();
	m_pffmpeg_player->set_log_callback(printlog);
	m_pffmpeg_player->init((HWND)winId());
}


player_widget::~player_widget()
{
	if (m_pffmpeg_player != NULL)
	{
		delete m_pffmpeg_player;
		m_pffmpeg_player = NULL;
	}
}

void player_widget::paintEvent(QPaintEvent *event)
{
	QStyleOption opt;
	opt.initFrom(this);
	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
	QWidget::paintEvent(event);
}

void player_widget::resizeEvent(QResizeEvent *event)
{
	QSize size = event->size();
	m_pffmpeg_player->set_sdl_window_size(size.width(), size.height());
	QWidget::resizeEvent(event);
}

void player_widget::start(QString url)
{
	if (url.isEmpty())
	{
		qDebug() << "this url is not allow empty!!!";
		return;
	}

	if (m_pffmpeg_player != NULL)
	{
		m_pffmpeg_player->start(url.toUtf8().data());
	}
}

void player_widget::stop()
{
	if (m_pffmpeg_player != NULL)
	{
		m_pffmpeg_player->stop();
	}
}

void player_widget::set_subtitle(QString subtitle)
{
	if (m_pffmpeg_player != NULL)
	{
		m_pffmpeg_player->set_video_subtitle(subtitle.toUtf8().data());
	}
}

void player_widget::printlog(void *ptr, int level, const char *fmt, va_list vl)
{
	va_list vl2;
	char line[1024];
	static int print_prefix = 1;

	va_copy(vl2, vl);
	av_log_default_callback(ptr, level, fmt, vl);
	av_log_format_line(ptr, level, fmt, vl2, line, sizeof(line), &print_prefix);
	va_end(vl2);

	qDebug() << line;
}

