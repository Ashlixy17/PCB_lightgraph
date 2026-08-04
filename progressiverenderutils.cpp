#include "progressiverenderutils.h"

#include <QtGlobal>

namespace ProgressiveRendering {

QSize scaledSize(const QSize& sourceSize, qreal linearScale) {
    if (!sourceSize.isValid() || sourceSize.isEmpty() || linearScale <= 0.0) {
        return QSize();
    }

    // 宽高使用相同线性比例，避免渐进预览改变原图宽高比。
    const int width = qMax(1, qRound(sourceSize.width() * linearScale));
    const int height = qMax(1, qRound(sourceSize.height() * linearScale));
    return QSize(width, height);
}

QVector<QSize> refinementSizes(const QSize& sourceSize) {
    QVector<QSize> sizes;
    if (!sourceSize.isValid() || sourceSize.isEmpty()) {
        return sizes;
    }

    // 拖动阶段已经显示 1/4，松手后只继续提升到 1/2 和原图。
    const qreal scales[] = {0.5, 1.0};
    for (qreal scale : scales) {
        const QSize size = scaledSize(sourceSize, scale);
        // 极小图片可能在多个比例下得到相同尺寸，跳过重复计算。
        if (sizes.isEmpty() || sizes.constLast() != size) {
            sizes.append(size);
        }
    }
    return sizes;
}

QVector<LEDStrip> scaleLedStrips(
    const QVector<LEDStrip>& strips,
    const QSize& sourceSize,
    const QSize& targetSize) {

    QVector<LEDStrip> scaled;
    if (!sourceSize.isValid() || sourceSize.isEmpty()
        || !targetSize.isValid() || targetSize.isEmpty()) {
        return scaled;
    }

    const qreal scaleX = static_cast<qreal>(targetSize.width()) / sourceSize.width();
    const qreal scaleY = static_cast<qreal>(targetSize.height()) / sourceSize.height();
    const qreal radiusScale = qMin(scaleX, scaleY);
    scaled.reserve(strips.size());

    for (const LEDStrip& source : strips) {
        LEDStrip target = source;
        target.start = QPoint(
            qRound(source.start.x() * scaleX),
            qRound(source.start.y() * scaleY));
        target.end = QPoint(
            qRound(source.end.x() * scaleX),
            qRound(source.end.y() * scaleY));
        // 半径使用较小轴比例，避免取整差异把灯光范围放大到目标图之外。
        target.radius = qMax(1, qRound(source.radius * radiusScale));
        scaled.append(target);
    }

    return scaled;
}

}
