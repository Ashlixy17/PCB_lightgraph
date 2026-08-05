#ifndef PROGRESSIVERENDERUTILS_H
#define PROGRESSIVERENDERUTILS_H

#include <QSize>
#include <QVector>
#include "ledstrip.h"

namespace ProgressiveRendering {

// 按目标像素预算等比缩小图片；不会放大原图，无效输入返回空尺寸。
QSize sizeForPixelBudget(const QSize& sourceSize, qint64 targetPixels);

// 根据 20 万像素预算返回拖动预览尺寸，小图直接保留原图尺寸。
QSize interactivePreviewSize(const QSize& sourceSize);

// 按图片大小返回去重后的可选中间阶段和最终原图尺寸。
QVector<QSize> refinementSizes(const QSize& sourceSize);

// 将原图坐标中的 LED 数据临时映射到目标预览尺寸，不修改项目原始数据。
QVector<LEDStrip> scaleLedStrips(
    const QVector<LEDStrip>& strips,
    const QSize& sourceSize,
    const QSize& targetSize);

}

#endif // PROGRESSIVERENDERUTILS_H
