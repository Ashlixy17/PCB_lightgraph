#ifndef PROGRESSIVERENDERUTILS_H
#define PROGRESSIVERENDERUTILS_H

#include <QSize>
#include <QVector>
#include "ledstrip.h"

namespace ProgressiveRendering {

// 按线性比例计算渲染尺寸；有效输入始终至少保留一个像素。
QSize scaledSize(const QSize& sourceSize, qreal linearScale);

// 返回去重后的 1/4、1/2、1:1 细化尺寸，最后一项始终是原图尺寸。
QVector<QSize> refinementSizes(const QSize& sourceSize);

// 将原图坐标中的 LED 数据临时映射到目标预览尺寸，不修改项目原始数据。
QVector<LEDStrip> scaleLedStrips(
    const QVector<LEDStrip>& strips,
    const QSize& sourceSize,
    const QSize& targetSize);

}

#endif // PROGRESSIVERENDERUTILS_H
