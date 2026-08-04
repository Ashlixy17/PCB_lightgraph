#include "progressiverendercontroller.h"

#include "progressiverenderutils.h"

namespace {
// 拖动预览限制在约 30 FPS；细化帧保留约一至两次屏幕刷新时间。
constexpr int kInteractiveFrameIntervalMs = 30;
constexpr int kRefinementFrameIntervalMs = 25;
}

ProgressiveRenderController::ProgressiveRenderController(QObject* parent)
    : QObject(parent) {
    // 拖动期间按固定节奏读取最新值，避免密集事件饿死预览帧。
    m_interactiveTimer.setInterval(kInteractiveFrameIntervalMs);
    m_interactiveTimer.setSingleShot(true);
    connect(&m_interactiveTimer, &QTimer::timeout, this, [this]() {
        if (!m_interactivePending || !m_sourceSize.isValid() || m_sourceSize.isEmpty()) {
            return;
        }

        m_interactivePending = false;
        // 在渲染前进入下一冷却周期；期间到来的变化会被合并到再下一帧。
        m_interactiveTimer.start();
        emit renderRequested(
            ProgressiveRendering::scaledSize(m_sourceSize, 0.125),
            m_generation,
            false);
    });
}

void ProgressiveRenderController::sliderPressed(const QSize& sourceSize) {
    // 一次新的交互会让之前排队的细化阶段全部失效。
    invalidate();
    m_sourceSize = sourceSize;
}

void ProgressiveRenderController::sliderValueChanged(const QSize& sourceSize) {
    m_sourceSize = sourceSize;

    // 每个新值使用新代次并取消旧细化，但不重启正在运行的节流周期。
    ++m_generation;
    m_refinementGeneration = 0;
    m_refinementSizes.clear();
    m_nextRefinementIndex = 0;
    if (!m_sourceSize.isValid() || m_sourceSize.isEmpty()) {
        m_interactivePending = false;
        m_interactiveTimer.stop();
        return;
    }

    if (m_interactiveTimer.isActive()) {
        m_interactivePending = true;
        return;
    }

    // 一次新拖动的首帧立即显示，后续帧才受 30 FPS 上限约束。
    m_interactivePending = false;
    m_interactiveTimer.start();
    emit renderRequested(
        ProgressiveRendering::scaledSize(m_sourceSize, 0.125),
        m_generation,
        false);
}

void ProgressiveRenderController::sliderReleased(const QSize& sourceSize) {
    const quint64 generation = invalidate();
    m_sourceSize = sourceSize;
    m_refinementSizes = ProgressiveRendering::refinementSizes(sourceSize);
    m_nextRefinementIndex = 0;
    m_refinementGeneration = generation;

    // 用事件队列启动第一阶段，避免在滑块信号栈内同步执行重渲染。
    queueNextRefinement(generation, 0);
}

quint64 ProgressiveRenderController::invalidate() {
    m_interactiveTimer.stop();
    m_interactivePending = false;
    ++m_generation;
    m_refinementGeneration = 0;
    m_refinementSizes.clear();
    m_nextRefinementIndex = 0;
    return m_generation;
}

quint64 ProgressiveRenderController::currentGeneration() const {
    return m_generation;
}

void ProgressiveRenderController::renderFinished(quint64 generation) {
    if (generation != m_generation || generation != m_refinementGeneration) {
        return;
    }

    // 当前阶段完成后再排下一阶段，让界面有机会先显示已得到的结果。
    queueNextRefinement(generation, kRefinementFrameIntervalMs);
}

void ProgressiveRenderController::queueNextRefinement(quint64 generation, int delayMs) {
    QTimer::singleShot(delayMs, this, [this, generation]() {
        requestNextRefinement(generation);
    });
}

void ProgressiveRenderController::requestNextRefinement(quint64 generation) {
    if (generation != m_generation || generation != m_refinementGeneration
        || m_nextRefinementIndex >= m_refinementSizes.size()) {
        return;
    }

    const QSize targetSize = m_refinementSizes.at(m_nextRefinementIndex++);
    const bool authoritative = targetSize == m_sourceSize;
    emit renderRequested(targetSize, generation, authoritative);
}
