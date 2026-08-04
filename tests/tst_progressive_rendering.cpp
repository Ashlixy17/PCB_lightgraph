#include <QtTest>
#include <QElapsedTimer>
#include "progressiverendercontroller.h"
#include "progressiverenderutils.h"

class ProgressiveRenderingTests : public QObject {
    Q_OBJECT

private slots:
    void largeImageUsesExpectedThreeStageSizes();
    void tinyImageRemovesDuplicateRefinementSizes();
    void ledGeometryScalesWithoutChangingColor();
    void interactiveChangesAreThrottledWithoutStarvation();
    void releaseRefinesInExpectedOrder();
    void invalidatedGenerationDoesNotContinue();
    void invalidationExposesCurrentGeneration();
};

void ProgressiveRenderingTests::largeImageUsesExpectedThreeStageSizes() {
    const QSize sourceSize(800, 600);

    // 拖动阶段直接使用 1/4，松手后只需继续生成 1/2 和原图两个阶段。
    QCOMPARE(ProgressiveRendering::scaledSize(sourceSize, 0.25), QSize(200, 150));
    QCOMPARE(
        ProgressiveRendering::refinementSizes(sourceSize),
        QVector<QSize>({QSize(400, 300), QSize(800, 600)}));
}

void ProgressiveRenderingTests::tinyImageRemovesDuplicateRefinementSizes() {
    // 极小图片的两个细化阶段会得到相同尺寸，只应保留一次完整渲染。
    QCOMPARE(
        ProgressiveRendering::refinementSizes(QSize(1, 1)),
        QVector<QSize>({QSize(1, 1)}));
}

void ProgressiveRenderingTests::ledGeometryScalesWithoutChangingColor() {
    LEDStrip strip;
    strip.start = QPoint(80, 40);
    strip.end = QPoint(160, 120);
    strip.radius = 32;
    strip.color = QColor(12, 34, 56, 78);

    const QVector<LEDStrip> scaled = ProgressiveRendering::scaleLedStrips(
        QVector<LEDStrip>({strip}), QSize(800, 600), QSize(200, 150));

    QCOMPARE(scaled.size(), 1);
    QCOMPARE(scaled.first().start, QPoint(20, 10));
    QCOMPARE(scaled.first().end, QPoint(40, 30));
    QCOMPARE(scaled.first().radius, 8);
    QCOMPARE(scaled.first().color, QColor(12, 34, 56, 78));
}

void ProgressiveRenderingTests::interactiveChangesAreThrottledWithoutStarvation() {
    ProgressiveRenderController controller;
    QSignalSpy spy(
        &controller,
        SIGNAL(renderRequested(QSize,quint64,bool)));

    controller.sliderPressed(QSize(800, 600));
    controller.sliderValueChanged(QSize(800, 600));
    // 第一帧必须立即提交，持续拖动时不能因反复重置计时器而一直没有预览。
    QCOMPARE(spy.count(), 1);

    controller.sliderValueChanged(QSize(800, 600));
    controller.sliderValueChanged(QSize(800, 600));

    // 冷却周期内的多次变化只合并成下一帧，形成真正的节流而不是防抖。
    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 100);
    QCOMPARE(spy.first().at(0).toSize(), QSize(200, 150));
    QCOMPARE(spy.first().at(2).toBool(), false);
}

void ProgressiveRenderingTests::releaseRefinesInExpectedOrder() {
    ProgressiveRenderController controller;
    QSignalSpy spy(
        &controller,
        SIGNAL(renderRequested(QSize,quint64,bool)));

    controller.sliderPressed(QSize(800, 600));
    controller.sliderReleased(QSize(800, 600));
    QTRY_COMPARE(spy.count(), 1);

    const quint64 generation = spy.at(0).at(1).toULongLong();
    QCOMPARE(spy.at(0).at(0).toSize(), QSize(400, 300));

    QElapsedTimer stageTimer;
    stageTimer.start();
    controller.renderFinished(generation);
    QTRY_COMPARE(spy.count(), 2);
    // 中间阶段至少保留接近一帧的时间，避免 Qt 把多次 setPixmap 合并成最终一帧。
    QVERIFY(stageTimer.elapsed() >= 15);
    QCOMPARE(spy.at(1).at(0).toSize(), QSize(800, 600));
    QCOMPARE(spy.at(1).at(2).toBool(), true);
}

void ProgressiveRenderingTests::invalidatedGenerationDoesNotContinue() {
    ProgressiveRenderController controller;
    QSignalSpy spy(
        &controller,
        SIGNAL(renderRequested(QSize,quint64,bool)));

    controller.sliderPressed(QSize(800, 600));
    controller.sliderReleased(QSize(800, 600));
    QTRY_COMPARE(spy.count(), 1);

    const quint64 staleGeneration = spy.first().at(1).toULongLong();
    controller.invalidate();
    controller.renderFinished(staleGeneration);
    QTest::qWait(10);
    QCOMPARE(spy.count(), 1);
}

void ProgressiveRenderingTests::invalidationExposesCurrentGeneration() {
    ProgressiveRenderController controller;

    // 主窗口需要用同一代次判断全分辨率结果是否仍可用于导出。
    const quint64 generation = controller.invalidate();
    QCOMPARE(controller.currentGeneration(), generation);
}

// 调度器依赖 Qt 事件循环驱动计时器，因此测试使用无界面的 QCoreApplication 入口。
QTEST_GUILESS_MAIN(ProgressiveRenderingTests)
#include "tst_progressive_rendering.moc"
