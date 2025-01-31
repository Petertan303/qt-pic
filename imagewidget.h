#ifndef IMAGEWIDGET_H
#define IMAGEWIDGET_H

#include <QWidget>
#include <QMainWindow>
#include <QImage>
#include <QByteArray>
#include <QWheelEvent>
#include <QPainter>

class ImageWidget : public QMainWindow
{
    Q_OBJECT
public:
    explicit ImageWidget(QWidget *parent = nullptr)
        : QMainWindow(parent), m_scale(1.0), m_xOffset(0), m_yOffset(0)
    {
        setMouseTracking(true);
    }

    void setImage(const QByteArray &img) {
        m_image.loadFromData(img);
        this->setWindowTitle("Image Viewer");
        this->resize(600, 600);
        this->show();
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.translate(m_xOffset, m_yOffset);
        p.scale(m_scale, m_scale);
        p.drawImage(0, 0, m_image);
    }

    void wheelEvent(QWheelEvent *e) override {
        const double zoomFactor = 1.15;
        const QPointF mousePos = e->position();

        // 计算缩放前的图像坐标
        const double oldX = (mousePos.x() - m_xOffset) / m_scale;
        const double oldY = (mousePos.y() - m_yOffset) / m_scale;

        // 更新缩放比例
        e->angleDelta().y() > 0 ? m_scale *= zoomFactor : m_scale /= zoomFactor;
        m_scale = qBound(0.1, m_scale, 50.0);  // 限制缩放范围

        // 调整偏移量保持鼠标位置不变
        m_xOffset = mousePos.x() - oldX * m_scale;
        m_yOffset = mousePos.y() - oldY * m_scale;

        update();
    }

private:
    QImage m_image;
    double m_scale;
    double m_xOffset;
    double m_yOffset;
};

#endif // IMAGEWIDGET_H
