#ifndef IMAGEWINDOW_H
#define IMAGEWINDOW_H

#include <QMainWindow>
#include <QImage>
#include <QLabel>
#include <QScrollArea>
#include <QMenu>
#include <QCoreApplication>
#include <QPropertyAnimation>

// namespace Ui {
// class imageWindow;  // 前向声明Ui类
// }

class imageWindow : public QMainWindow {
    Q_OBJECT
    // Q_PROPERTY(qreal scaleFactor READ getScaleFactor WRITE setScaleFactor) // 声明属性
public:
    explicit imageWindow(QWidget *parent = nullptr);
    ~imageWindow();

public slots:
    void setImage(const QByteArray &image);  // 新增槽函数，用于接收 QImage

protected:
    // void wheelEvent(QWheelEvent *event) override;      // 滚轮事件
    void contextMenuEvent(QContextMenuEvent *event) override;  // 右键菜单事件
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    // void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void saveImage();                   // 保存图片
    void openContainingFolder();        // 打开所在文件夹

private:
    void createActions();               // 创建菜单动作
    void setupContextMenu();            // 设置右键菜单
    void setupScrollArea();             // 设置滚动区域
    void setupImageLabel();             // 设置图片标签
    void setupMainLayout();             // 布局设置
    void adjustImageScale();            // 调整缩放比例
    void copyImage();                   // 复制图片
    void updateImageLabel();
    void applyZoom();
    void resetImage();
    void fitToWindow();
    void initTimer();
    void initShortcut();
    void initAnimation();

    // 成员变量
    QLabel *imageLabel;                 // 显示图片的标签
    QScrollArea *scrollArea;            // 滚动区域
    QMenu *contextMenu;                 // 右键菜单
    QAction *copyAction;                // 复制动作
    QAction *saveAction;                // 保存动作
    QAction *openFolderAction;          // 打开文件夹动作
    QImage image;                       // 当前显示的图片数据
    double targetScaleFactor;           // 目标缩放比例
    QPoint lastDragPos;                 // 记录上一次拖动的位置
    bool isDragging;                    // 是否正在拖动
    QString currentImagePath = QCoreApplication::applicationDirPath() + "/output/"; // 当前图片保存的路径

    double m_scale;           // 当前缩放比例
    int m_accumulatedDelta; // 累积的滚轮值
    QTimer *m_debounceTimer; // 防抖定时器
    bool m_isAnimating;      // 是否正在动画中
    QPixmap originalPixmap;  // 原始图片
    QPropertyAnimation *m_scaleAnimation; // 缩放动画
    QPropertyAnimation *m_scrollAnimation; // 滚动动画

    // 缩放比例属性（用于动画）
    Q_PROPERTY(double scale READ scale WRITE setScale)
    double scale() const { return m_scale; }
    void setScale(double scale);

    // 滚动值属性（用于动画）
    Q_PROPERTY(QPoint scrollValue READ scrollValue WRITE setScrollValue)
    QPoint scrollValue() const;
    void setScrollValue(QPoint value);
};

#endif
