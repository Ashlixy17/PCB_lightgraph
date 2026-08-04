#include "progressiverendercontroller.h"

#include "progressiverenderutils.h"

ProgressiveRenderController::ProgressiveRenderController(QObject* parent)
    : QObject(parent) {
    // 拖动期间把密集的 valueChanged 合并为约 30 毫秒一次，避免主线程重复计算过期帧。
    m_interactiveTimer.setInterval(30);
    m_interactiveTimer.setSingleShot(true);
    connect(&m_interactiveTimer, &QTimer::timeout, this, [this]() {
        if (!m_sourceSize.isValid() || m_sourceSize.isEmpty()) {
            return;
        }

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

    // 每个新值使用新代次；计时器重启后只会提交最后一次变化。
    invalidate();
    m_sourceSize = sourceSize;
    m_interactiveTimer.start();
}

void ProgressiveRenderController::sliderReleased(const QSize& sourceSize) {
    const quint64 generation = invalidate();
    m_sourceSize = sourceSize;
    m_refinementSizes = ProgressiveRendering::refinementSizes(sourceSize);
    m_nextRefinementIndex = 0;
    m_refinementGeneration = generation;

    // 用事件队列启动第一阶段，避免在滑块信号栈内同步执行重渲染。
    queueNextRefinement(generation);
}

quint64 ProgressiveRenderController::invalidate() {
    m_interactiveTimer.stop();
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
    queueNextRefinement(generation);
}

void ProgressiveRenderController::queueNextRefinement(quint64 generation) {
    QTimer::singleShot(0, this, [this, generation]() {
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
