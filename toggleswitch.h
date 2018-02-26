#ifndef TOGGLESWITCH_H
#define TOGGLESWITCH_H

#include <QWidget>
#include <QTimer>

class ToggleSwitch : public QWidget
{
	Q_OBJECT
public:
	explicit ToggleSwitch(bool currentState, int trackWidth = 60,
				int trackHeight = 32, QWidget *parent = nullptr);

public:
	bool isChecked() const;
	void acceptStateChange();

protected:
	void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
	void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
	/* 鼠标释放事件 - 切换开关状态、发射toggled()信号 */
	void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
	/* 大小改变事件 */
	void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
	/* 缺省大小 */
	QSize sizeHint() const Q_DECL_OVERRIDE;
	QSize minimumSizeHint() const Q_DECL_OVERRIDE;

signals:
	/* 状态改变时，发射信号 */
	void toggled(bool currentState);

private slots:
	/* 状态切换时，用于产生滑动效果 */
	void onTimeout();

private:
	QColor trackColor;
	QColor thumbColor;
	bool currentState;
	bool toState;
	int thumbX;
	int thumbY;
	double trackRadius;
	int trackWidth;
	int trackHeight;
	int trackMargin; /* 外边距 */
	int thumbWidth;
	int thumbHeight;
	QTimer *timer;

public slots:
};

#endif // TOGGLESWITCH_H
