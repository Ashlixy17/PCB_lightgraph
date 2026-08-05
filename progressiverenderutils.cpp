#include "progressiverenderutils.h"

#include <QtGlobal>
#include <cmath>

namespace {
// 拖动阶段控制在约 20 万像素，接近原固定 1/4 预览的流畅负载。
constexpr qint64 kInteractivePixelBudget = 200000;
// 超过 100 万像素的大图松手后先细化一次，再恢复到完整原图。
constexpr qint64 kRefinementPixelBudget = 1000000;
}

namespace ProgressiveRendering {

QSize sizeForPixelBudget(const QSize& sourceSize, qint64 targetPixels) {
    if (!sourceSize.isValid() || sourceSize.isEmpty() || targetPixels <= 0) {
        return QSize();
    }

    // 使用 64 位整数计算总像素，避免高分辨率图片宽高相乘时溢出。
    const qint64 sourcePixels = static_cast<qint64>(sourceSize.width())
        * static_cast<qint64>(sourceSize.height());
    if (sourcePixels <= targetPixels) {
        return sourceSize;
    }

    const double scale = std::sqrt(
        static_cast<double>(targetPixels) / static_cast<double>(sourcePixels));
    int width = qMax(1, static_cast<int>(std::floor(sourceSize.width() * scale)));
    int height = qMax(1, static_cast<int>(std::floor(sourceSize.height() * scale)));

    // 极端宽高比可能因最小一像素保护略超预算，收紧较长轴以守住计算上限。
    if (static_cast<qint64>(width) * height > targetPixels) {
        if (width >= height) {
            width = qMax(1, static_cast<int>(targetPixels / height));
        } else {
            height = qMax(1, static_cast<int>(targetPixels / width));
        }
    }
    return QSize(width, height);
}

QSize interactivePreviewSize(const QSize& sourceSize) {
    return sizeForPixelBudget(sourceSize, kInteractivePixelBudget);
}

QVector<QSize> refinementSizes(const QSize& sourceSize) {
    QVector<QSize> sizes;
    if (!sourceSize.isValid() || sourceSize.isEmpty()) {
        return sizes;
    }

    // 只有超过 100 万像素的大图才需要中间细化阶段。
    const QSize refinementSize = sizeForPixelBudget(sourceSize, kRefinementPixelBudget);
    if (refinementSize != sourceSize) {
        sizes.append(refinementSize);
    }

    // 最后一阶段始终使用精确原图尺寸，并跳过取整产生的重复尺寸。
    if (sizes.isEmpty() || sizes.constLast() != sourceSize) {
        sizes.append(sourceSize);
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
