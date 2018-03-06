#include "toggleswitch.h"
#include <QPainter>
#include <QMouseEvent>
#include <QDebug>

#define TRACK_TOGGLED_COLOR "#9ad3f5"
#define TRACK_UNTOGGLED_COLOR "#c6c6c6"
#define TRACK_DISABLED_COLOR "#d6d6d6"
#define THUMB_TOGGLED_COLOR "#0a77b5"
#define THUMB_UNTOGGLED_COLOR "#7c7c7c"
#define THUMB_DISABLED_COLOR "#b9b9b9"

ToggleSwitch::ToggleSwitch(bool currentState, int trackWidth, int trackHeight, QWidget *parent) :
			QWidget(parent),
			currentState(currentState),
			trackWidth(trackWidth),
			trackHeight(trackHeight)
{
	trackRadius = trackHeight/2;
	trackMargin = 3;
	thumbWidth = thumbHeight = trackHeight;

	if (currentState) {
		thumbX = trackWidth - thumbWidth;
		thumbY = 0;
		trackColor = TRACK_TOGGLED_COLOR;
		thumbColor = THUMB_TOGGLED_COLOR;
	} else {
		thumbX = 0;
		thumbY = 0;
		trackColor = TRACK_UNTOGGLED_COLOR;
		thumbColor = THUMB_UNTOGGLED_COLOR;
	}

	/* 鼠标滑过光标形状 - 手型 */
	setCursor(Qt::PointingHandCursor);
	/* 连接信号槽 */
	timer = new QTimer();
	timer->setInterval(10);
	connect(timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
}

/* 绘制开关 */
void ToggleSwitch::paintEvent(QPaintEvent *event)
{
	if (!isEnabled()) {
		trackColor = TRACK_DISABLED_COLOR;
		thumbColor = THUMB_DISABLED_COLOR;
	}
	Q_UNUSED(event);

	QPainter painter(this);
	painter.setPen(Qt::NoPen);
	painter.setRenderHint(QPainter::Antialiasing);
	QPainterPath path;

	painter.setBrush(trackColor);
	path.addRoundedRect(QRectF(trackMargin, trackMargin, trackWidth - 2 * trackMargin, trackHeight - 2 * trackMargin), trackRadius, trackRadius);
	painter.drawPath(path.simplified());

	painter.setBrush(thumbColor);
	painter.drawEllipse(QRectF(thumbX, thumbY, thumbWidth, thumbHeight));
}

/* 鼠标按下事件 */
void ToggleSwitch::mousePressEvent(QMouseEvent *event)
{
	if (isEnabled()) {
		if (event->buttons() & Qt::LeftButton) {
			event->accept();
		} else {
			event->ignore();
		}
	}
}

/* 鼠标释放事件 - 切换开关状态、发射toggled()信号 */
void ToggleSwitch::mouseReleaseEvent(QMouseEvent *event)
{
	if (!isEnabled())
		return;
	if ((event->type() == QMouseEvent::MouseButtonRelease)
				&& (event->button() == Qt::LeftButton)) {
		event->accept();
		toState = !currentState;
		emit toggled(toState);
	} else {
		event->ignore();
	}
}

/* 大小改变事件 */
void ToggleSwitch::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);
}

/* 默认大小 */
QSize ToggleSwitch::sizeHint() const
{
	return minimumSizeHint();
}

/* 最小大小 */
QSize ToggleSwitch::minimumSizeHint() const
{
	return QSize(2 * (trackHeight + trackMargin), trackHeight + 2 * trackMargin);
}

/* 切换状态 - 滑动 */
void ToggleSwitch::onTimeout()
{
	if (currentState) {
		thumbX += 3;
		if (thumbX >= trackWidth - thumbWidth)
			timer->stop();
	} else {
		thumbX -= 3;
		if (thumbX <= 0)
			timer->stop();
	}
	/* 3 may not be an integer multiple of distance */
	if (thumbX > trackWidth - thumbWidth) thumbX = trackWidth - thumbWidth;
	if (thumbX < 0) thumbX = 0;
	update();
}

/* 返回开关状态 - 打开：true 关闭：false */
bool ToggleSwitch::isChecked() const
{
	return currentState;
}

void ToggleSwitch::acceptStateChange()
{
	currentState = toState;
	if (currentState) {
		trackColor = QColor(TRACK_TOGGLED_COLOR);
		thumbColor = QColor(THUMB_TOGGLED_COLOR);
	} else {
		trackColor = QColor(TRACK_UNTOGGLED_COLOR);
		thumbColor = QColor(THUMB_UNTOGGLED_COLOR);
	}
	timer->start();
}
