#ifndef PROGRESSIVERENDERCONTROLLER_H
#define PROGRESSIVERENDERCONTROLLER_H

#include <QObject>
#include <QSize>
#include <QTimer>
#include <QVector>

/**
 * @brief 渐进渲染调度器
 * 负责合并滑块拖动事件、安排细化阶段并淘汰过期渲染请求。
 * 本类只管理渲染时序，不执行具体的图像处理。
 */
class ProgressiveRenderController : public QObject {
    Q_OBJECT

public:
    explicit ProgressiveRenderController(QObject *parent = nullptr);

    /**
     * @brief 开始一次滑块拖动，取消之前尚未完成的细化阶段
     * @param sourceSize 原图尺寸
     */
    void sliderPressed(const QSize& sourceSize);

    /**
     * @brief 接收拖动中的最新值，并按固定帧率合并预览请求
     * @param sourceSize 原图尺寸
     */
    void sliderValueChanged(const QSize& sourceSize);

    /**
     * @brief 结束滑块拖动，按图片大小启动后续细化阶段
     * @param sourceSize 原图尺寸
     */
    void sliderReleased(const QSize& sourceSize);

    /**
     * @brief 使旧渲染请求失效，并返回新的渲染代次
     * @return 新的有效渲染代次
     */
    quint64 invalidate();

    /**
     * @brief 获取当前有效的渲染代次
     * @return 当前有效的渲染代次
     */
    quint64 currentGeneration() const;

    /**
     * @brief 通知调度器指定代次的一帧已经完成
     * @param generation 已完成帧所属的渲染代次
     */
    void renderFinished(quint64 generation);

signals:
    /**
     * @brief 请求主窗口渲染指定尺寸
     * @param targetSize 当前阶段的渲染尺寸
     * @param generation 用于识别过期请求的渲染代次
     * @param authoritative 是否为可用于导出的完整分辨率结果
     */
    void renderRequested(const QSize& targetSize, quint64 generation, bool authoritative);

private:
    void queueNextRefinement(quint64 generation, int delayMs);
    void requestNextRefinement(quint64 generation);

    // 拖动预览状态：计时器限制帧率，待处理标记用于合并密集事件。
    QTimer m_interactiveTimer;
    QSize m_sourceSize;
    bool m_interactivePending = false;

    // 松手后的细化状态：尺寸列表按顺序保存尚待渲染的阶段。
    QVector<QSize> m_refinementSizes;
    int m_nextRefinementIndex = 0;

    // 每次参数变化都会递增代次，旧代次的回调不得继续推进细化。
    quint64 m_generation = 0;
    quint64 m_refinementGeneration = 0;
};

#endif // PROGRESSIVERENDERCONTROLLER_H
