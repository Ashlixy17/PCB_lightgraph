#include <QtTest>
#include "progressiverenderutils.h"

class ProgressiveRenderingTests : public QObject {
    Q_OBJECT

private slots:
    void largeImageUsesExpectedFourStageSizes();
    void tinyImageRemovesDuplicateRefinementSizes();
    void ledGeometryScalesWithoutChangingColor();
};

void ProgressiveRenderingTests::largeImageUsesExpectedFourStageSizes() {
    const QSize sourceSize(800, 600);

    QCOMPARE(ProgressiveRendering::scaledSize(sourceSize, 0.125), QSize(100, 75));
    QCOMPARE(
        ProgressiveRendering::refinementSizes(sourceSize),
        QVector<QSize>({QSize(200, 150), QSize(400, 300), QSize(800, 600)}));
}

void ProgressiveRenderingTests::tinyImageRemovesDuplicateRefinementSizes() {
    // 极小图片的三个细化阶段会得到相同尺寸，只应保留一次完整渲染。
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

QTEST_APPLESS_MAIN(ProgressiveRenderingTests)
#include "tst_progressive_rendering.moc"
