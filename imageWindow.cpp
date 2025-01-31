#include <QMainWindow>
#include <QImage>
#include <QLabel>
#include <QScrollArea>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDir>
#include "imageWindow.h"
#include <QScrollBar>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QPainter>
#include <QTimer>
#include <QShortcut>
// #include <QOpenGLWidget> // 需要link
// #include <QtConcurrent/QtConcurrent> //注意不是QtConcurrent

// 构造函数参数应为 QWidget* 而非 QMainWindow*
imageWindow::imageWindow(QWidget *parent) : QMainWindow(parent), m_scale(1.0), m_accumulatedDelta(0), m_isAnimating(false) {
    setCursor(Qt::OpenHandCursor);

    createActions();
    setupContextMenu();
    setupScrollArea();
    setupImageLabel();

    targetScaleFactor = 1.0;
    // 初始化定时器
    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true); // 单次触发
    m_debounceTimer->setInterval(20);    // 20ms 延迟
    connect(m_debounceTimer, &QTimer::timeout, this, &imageWindow::applyZoom);

    // 快捷键
    QShortcut *resetShortcut = new QShortcut(QKeySequence("Ctrl+1"), this);
    connect(resetShortcut, &QShortcut::activated, this, &imageWindow::resetImage);
    // 设置 Ctrl+0 快捷键
    QShortcut *fitToWindowShortcut = new QShortcut(QKeySequence("Ctrl+0"), this);
    connect(fitToWindowShortcut, &QShortcut::activated, this, &imageWindow::fitToWindow);

    // 初始化动画
    m_scaleAnimation = new QPropertyAnimation(this, "scale");
    m_scaleAnimation->setDuration(300); // 动画时长 300ms
    m_scaleAnimation->setEasingCurve(QEasingCurve::InOutQuad); // 平滑过渡

    m_scrollAnimation = new QPropertyAnimation(this, "scrollValue");
    m_scrollAnimation->setDuration(300); // 动画时长 300ms
    m_scrollAnimation->setEasingCurve(QEasingCurve::InOutQuad); // 平滑过渡

    setWindowTitle("Image Viewer");
    // this->show();
}

// 必须定义析构函数
imageWindow::~imageWindow() {
    this->hide();
    imageLabel->deleteLater();
    scrollArea->deleteLater();
}

// 创建右键菜单动作
void imageWindow::createActions() {
    copyAction = new QAction("Copy", this);
    saveAction = new QAction("Save As...", this);
    openFolderAction = new QAction("Open Containing Folder", this);

    // 连接动作到槽函数
    connect(copyAction, &QAction::triggered, this, &imageWindow::copyImage);
    connect(saveAction, &QAction::triggered, this, &imageWindow::saveImage);
    connect(openFolderAction, &QAction::triggered, this, &imageWindow::openContainingFolder);
}

// 实现 copyImage 槽函数
void imageWindow::copyImage() {
    if (!imageLabel->pixmap().isNull()) {
        QClipboard *clipboard = QApplication::clipboard();
        clipboard->setPixmap(imageLabel->pixmap());
    } else {
        QMessageBox::warning(this, "Error", "No image to copy!");
    }
}

// 设置右键菜单
void imageWindow::setupContextMenu() {
    contextMenu = new QMenu(this);
    contextMenu->addAction(copyAction);
    contextMenu->addAction(saveAction);
    contextMenu->addAction(openFolderAction);
}

// 从mainwindow接受图片
void imageWindow::setImage(const QByteArray &newImage) {
    if (!newImage.isNull()) {
        image.loadFromData(newImage);
        originalPixmap.loadFromData(newImage);
        // 可以不使用setpixmap，而是重写绘制窗口的函数实现缩放，但是很麻烦
        // imageLabel->setPixmap(QPixmap::fromImage(image));
        // // 初始，平滑缩放
        // imageLabel->setPixmap(QPixmap::fromImage(image).scaled(
        //     image.size() * m_scale, Qt::KeepAspectRatio, Qt::SmoothTransformation
        //     ));
        this->resize(600, 600);
        updateImageLabel();
        this->show();
        update();
    } else {
        QMessageBox::warning(this, "Error", "Received invalid image!");
    }
}

// 设置滚动区域
void imageWindow::setupScrollArea() {
    scrollArea = new QScrollArea;
    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setWidgetResizable(true);
    scrollArea->viewport()->installEventFilter(this); // 拦截事件
    setCentralWidget(scrollArea);
}


// 设置图片标签
void imageWindow::setupImageLabel() {
    imageLabel = new QLabel; // 这个是必需的
    imageLabel->setAlignment(Qt::AlignCenter);  // 图片居中
    imageLabel->setMouseTracking(true);         // 允许鼠标跟踪
    scrollArea->setWidget(imageLabel);
    setCentralWidget(scrollArea); // 注意不是imageLabel
    imageLabel->setPixmap(QPixmap::fromImage(image));
}

// 复位图片
void imageWindow::resetImage() {
    if (m_isAnimating) {
        return; // 如果正在动画中，直接返回
    }

    // 锁定滚轮事件
    m_isAnimating = true;

    // 设置动画起始值和结束值
    m_scaleAnimation->setStartValue(m_scale);
    m_scaleAnimation->setEndValue(1.0);

    m_scrollAnimation->setStartValue(QPoint(
        scrollArea->horizontalScrollBar()->value(),
        scrollArea->verticalScrollBar()->value()
        ));
    m_scrollAnimation->setEndValue(QPoint(0, 0));

    // 启动动画
    m_scaleAnimation->start();
    m_scrollAnimation->start();

    // 动画结束后解锁滚轮事件
    QTimer::singleShot(m_scaleAnimation->duration(), this, [this]() {
        m_isAnimating = false;
    });
}

// 自适应窗口大小
void imageWindow::fitToWindow() {
    if (m_isAnimating) {
        return; // 如果正在动画中，直接返回
    }

    // 锁定滚轮事件
    m_isAnimating = true;

    // 计算窗口和图片的宽高比
    QSize windowSize = scrollArea->viewport()->size();
    QSize imageSize = originalPixmap.size();

    double windowAspect = static_cast<double>(windowSize.width()) / windowSize.height();
    double imageAspect = static_cast<double>(imageSize.width()) / imageSize.height();

    // 计算自适应缩放比例
    double targetScale;
    if (windowAspect > imageAspect) {
        // 窗口更宽，按高度缩放
        targetScale = static_cast<double>(windowSize.height()) / imageSize.height();
    } else {
        // 窗口更高，按宽度缩放
        targetScale = static_cast<double>(windowSize.width()) / imageSize.width();
    }

    // 设置动画起始值和结束值
    m_scaleAnimation->setStartValue(m_scale);
    m_scaleAnimation->setEndValue(targetScale);

    m_scrollAnimation->setStartValue(QPoint(
        scrollArea->horizontalScrollBar()->value(),
        scrollArea->verticalScrollBar()->value()
        ));
    m_scrollAnimation->setEndValue(QPoint(0, 0));

    // 启动动画
    m_scaleAnimation->start();
    m_scrollAnimation->start();

    // 动画结束后解锁滚轮事件
    QTimer::singleShot(m_scaleAnimation->duration(), this, [this]() {
        m_isAnimating = false;
    });
}

// 缩放比例属性（用于动画）
Q_PROPERTY(double scale READ scale WRITE setScale)
void imageWindow::setScale(double scale) {
    m_scale = scale;
    updateImageLabel();
}

// 滚动值属性（用于动画）
QPoint imageWindow::scrollValue() const{
    return QPoint(
        scrollArea->horizontalScrollBar()->value(),
        scrollArea->verticalScrollBar()->value()
        );
}

void imageWindow::setScrollValue(QPoint value) {
    scrollArea->horizontalScrollBar()->setValue(value.x());
    scrollArea->verticalScrollBar()->setValue(value.y());
}

void imageWindow::updateImageLabel() {
    if (!image.isNull()) {
        // 计算缩放后的图片大小
        QSize scaledSize = image.size() * m_scale;
        QPixmap scaledPixmap = QPixmap::fromImage(image).scaled(
            scaledSize,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );

        // 设置缩放后的图片
        imageLabel->setPixmap(scaledPixmap);
        imageLabel->resize(scaledSize);  // 调整 imageLabel 的大小
        // 在事件过滤器中实现了滚轮值更改
    }
}


// 我绘制图片的方法可以是重写paintEvent，也可以是是setpix
// 而如果需要使用滚动条，那么paintEvent就相当于重写所有现成的东西，很麻烦，因此不使用。
// void imageWindow::paintEvent(QPaintEvent* event){

bool imageWindow::eventFilter(QObject *obj, QEvent *event){
    if (m_isAnimating && event->type() == QEvent::Wheel) {
        return true; // 动画期间锁定滚轮事件
    }
    if (event->type() == QEvent::Wheel) {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
        if (wheelEvent->modifiers() & Qt::ControlModifier) {
            // 累加滚轮值
            m_accumulatedDelta += wheelEvent->angleDelta().y();

            // 重置并启动定时器
            m_debounceTimer->start();

            return true; // 表示事件已经被处理
        }
    }
    return QWidget::eventFilter(obj, event);
}


void imageWindow::applyZoom() {
    if (m_accumulatedDelta == 0) {
        return; // 没有累积值，直接返回
    }

    const double zoomFactor = 1.15;
    const QPoint mousePos = mapFromGlobal(QCursor::pos()); // 获取当前鼠标位置

    // 获取当前滚动条的值
    int currentHScroll = scrollArea->horizontalScrollBar()->value();
    int currentVScroll = scrollArea->verticalScrollBar()->value();

    // 计算鼠标点在图像内容中的原始坐标
    double imageX = (mousePos.x() + currentHScroll) / m_scale;
    double imageY = (mousePos.y() + currentVScroll) / m_scale;

    // 根据累积的滚轮值更新缩放比例
    double zoomMultiplier = qPow(zoomFactor, m_accumulatedDelta / 120.0); // 120 是标准滚轮步长
    m_scale *= zoomMultiplier;
    m_scale = qBound(0.1, m_scale, 5.0);

    // 计算新的滚动条位置
    int newHScroll = qRound(imageX * m_scale - mousePos.x());
    int newVScroll = qRound(imageY * m_scale - mousePos.y());

    // 设置滚动条的值
    scrollArea->horizontalScrollBar()->setValue(newHScroll);
    scrollArea->verticalScrollBar()->setValue(newVScroll);

    // 更新图像显示
    updateImageLabel();

    // 重置累积值
    m_accumulatedDelta = 0;
}


// 右键菜单事件
void imageWindow::contextMenuEvent(QContextMenuEvent *event) {
    if (imageLabel->underMouse()) {
        contextMenu->exec(event->globalPos());
    }
}

// 保存图片
void imageWindow::saveImage() {
    QString outputDir = QDir::homePath() + "/output/";
    QDir dir(outputDir);
    if (!dir.exists()) {
        dir.mkpath(outputDir); // 创建文件夹
    }
    QString defaultFileName = outputDir + "untitled.png";
    QString filePath = QFileDialog::getSaveFileName(
        this, "Save Image", defaultFileName, "Images (*.png *.jpg *.bmp)"
        );
    if (!filePath.isEmpty()) {
        if (!imageLabel->pixmap().save(filePath)) {
            QMessageBox::warning(this, "Error", "Failed to save image!");
        }
    }
}

// 打开所在文件夹
void imageWindow::openContainingFolder() {
    if (!currentImagePath.isEmpty()) {
        // qDebug() << currentImagePath;
        QFileInfo fileInfo(currentImagePath);
        QDir dir = fileInfo.absoluteDir();
        qDebug() << dir;
        QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()));
    } else {
        QMessageBox::warning(this, "Error", "No image is loaded!");
    }
}

// 单击拖动图片
void imageWindow::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        setCursor(Qt::ClosedHandCursor);
        lastDragPos = event->pos(); // 记录鼠标按下时的位置
        isDragging = true;
    }
}

// 拖拽
void imageWindow::mouseMoveEvent(QMouseEvent *event) {
    if (isDragging) {
        QPoint delta = event->pos() - lastDragPos; // 计算移动距离
        lastDragPos = event->pos();

        // 调整滚动条的位置
        QScrollBar *hScrollBar = scrollArea->horizontalScrollBar();
        QScrollBar *vScrollBar = scrollArea->verticalScrollBar();
        hScrollBar->setValue(hScrollBar->value() - delta.x());
        vScrollBar->setValue(vScrollBar->value() - delta.y());
    }
}

// 释放鼠标左键
void imageWindow::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        setCursor(Qt::OpenHandCursor);
        isDragging = false;
    }
}
