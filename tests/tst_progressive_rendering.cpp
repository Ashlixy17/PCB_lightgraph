#include <QtTest>
#include <QElapsedTimer>
#include "progressiverendercontroller.h"
#include "progressiverenderutils.h"

class ProgressiveRenderingTests : public QObject {
    Q_OBJECT

private slots:
    void adaptivePreviewSizesRespectPixelBudgets();
    void invalidPixelBudgetReturnsEmptySize();
    void refinementStagesAdaptToImageClass();
    void tinyImageRemovesDuplicateRefinementSizes();
    void ledGeometryScalesWithoutChangingColor();
    void interactiveChangesAreThrottledWithoutStarvation();
    void releaseRefinesInExpectedOrder();
    void smallAndMediumImagesReleaseDirectlyToFullSize();
    void invalidatedGenerationDoesNotContinue();
    void invalidationExposesCurrentGeneration();
};

void ProgressiveRenderingTests::adaptivePreviewSizesRespectPixelBudgets() {
    // 小图不应被放大或缩小，拖动阶段直接使用原图尺寸。
    QCOMPARE(
        ProgressiveRendering::interactivePreviewSize(QSize(500, 400)),
        QSize(500, 400));

    // 实际出现卡顿的图片按 20 万像素预算应接近原先流畅的 1/4 预览负载。
    QCOMPARE(
        ProgressiveRendering::interactivePreviewSize(QSize(1388, 2048)),
        QSize(368, 543));

    // 1080p 和 4K 宽高比相同，按 20 万像素预算应得到相同预览尺寸。
    QCOMPARE(
        ProgressiveRendering::interactivePreviewSize(QSize(1920, 1080)),
        QSize(596, 335));
    QCOMPARE(
        ProgressiveRendering::interactivePreviewSize(QSize(3840, 2160)),
        QSize(596, 335));

    // 1200 万像素和超宽图验证低预算、64 位像素计算与等比缩放。
    QCOMPARE(
        ProgressiveRendering::interactivePreviewSize(QSize(4000, 3000)),
        QSize(516, 387));
    QCOMPARE(
        ProgressiveRendering::sizeForPixelBudget(QSize(20000, 100), 1000000),
        QSize(14142, 70));
}

void ProgressiveRenderingTests::invalidPixelBudgetReturnsEmptySize() {
    // 无效图片或非正像素预算不能产生可渲染尺寸。
    QCOMPARE(ProgressiveRendering::sizeForPixelBudget(QSize(), 1000000), QSize());
    QCOMPARE(ProgressiveRendering::sizeForPixelBudget(QSize(800, 600), 0), QSize());
}

void ProgressiveRenderingTests::refinementStagesAdaptToImageClass() {
    // 小图和恰好 100 万像素的中图松手后都只需要最终原图阶段。
    QCOMPARE(
        ProgressiveRendering::refinementSizes(QSize(500, 400)),
        QVector<QSize>({QSize(500, 400)}));
    QCOMPARE(
        ProgressiveRendering::refinementSizes(QSize(1000, 1000)),
        QVector<QSize>({QSize(1000, 1000)}));

    // 大图先细化到约 100 万像素，再恢复到精确原图尺寸。
    QCOMPARE(
        ProgressiveRendering::refinementSizes(QSize(3840, 2160)),
        QVector<QSize>({QSize(1333, 750), QSize(3840, 2160)}));
}

void ProgressiveRenderingTests::tinyImageRemovesDuplicateRefinementSizes() {
    // 极小图片的自适应阶段会得到相同尺寸，只应保留一次完整渲染。
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

    controller.sliderPressed(QSize(3840, 2160));
    controller.sliderValueChanged(QSize(3840, 2160));
    // 第一帧必须立即提交，持续拖动时不能因反复重置计时器而一直没有预览。
    QCOMPARE(spy.count(), 1);

    controller.sliderValueChanged(QSize(3840, 2160));
    controller.sliderValueChanged(QSize(3840, 2160));

    // 冷却周期内的多次变化只合并成下一帧，形成真正的节流而不是防抖。
    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 2, 100);
    QCOMPARE(spy.first().at(0).toSize(), QSize(596, 335));
    QCOMPARE(spy.first().at(2).toBool(), false);
}

void ProgressiveRenderingTests::releaseRefinesInExpectedOrder() {
    ProgressiveRenderController controller;
    QSignalSpy spy(
        &controller,
        SIGNAL(renderRequested(QSize,quint64,bool)));

    controller.sliderPressed(QSize(3840, 2160));
    controller.sliderReleased(QSize(3840, 2160));
    QTRY_COMPARE(spy.count(), 1);

    const quint64 generation = spy.at(0).at(1).toULongLong();
    QCOMPARE(spy.at(0).at(0).toSize(), QSize(1333, 750));

    QElapsedTimer stageTimer;
    stageTimer.start();
    controller.renderFinished(generation);
    QTRY_COMPARE(spy.count(), 2);
    // 中间阶段至少保留接近一帧的时间，避免 Qt 把多次 setPixmap 合并成最终一帧。
    QVERIFY(stageTimer.elapsed() >= 15);
    QCOMPARE(spy.at(1).at(0).toSize(), QSize(3840, 2160));
    QCOMPARE(spy.at(1).at(2).toBool(), true);
}

void ProgressiveRenderingTests::smallAndMediumImagesReleaseDirectlyToFullSize() {
    const QVector<QSize> sourceSizes({QSize(500, 400), QSize(1000, 1000)});

    // 小图和中图松手后都直接请求最终原图；两者的拖动预览尺寸由像素预算区分。
    for (const QSize& sourceSize : sourceSizes) {
        ProgressiveRenderController controller;
        QSignalSpy spy(
            &controller,
            SIGNAL(renderRequested(QSize,quint64,bool)));

        controller.sliderPressed(sourceSize);
        controller.sliderReleased(sourceSize);
        QTRY_COMPARE(spy.count(), 1);
        QCOMPARE(spy.first().at(0).toSize(), sourceSize);
        QCOMPARE(spy.first().at(2).toBool(), true);
    }
}

void ProgressiveRenderingTests::invalidatedGenerationDoesNotContinue() {
    ProgressiveRenderController controller;
    QSignalSpy spy(
        &controller,
        SIGNAL(renderRequested(QSize,quint64,bool)));

    controller.sliderPressed(QSize(3840, 2160));
    controller.sliderReleased(QSize(3840, 2160));
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
