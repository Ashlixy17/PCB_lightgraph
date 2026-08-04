#ifndef PROGRESSIVERENDERCONTROLLER_H
#define PROGRESSIVERENDERCONTROLLER_H

#include <QObject>
#include <QSize>
#include <QTimer>
#include <QVector>

class ProgressiveRenderController : public QObject {
    Q_OBJECT

public:
    explicit ProgressiveRenderController(QObject* parent = nullptr);

    void sliderPressed(const QSize& sourceSize);
    void sliderValueChanged(const QSize& sourceSize);
    void sliderReleased(const QSize& sourceSize);
    quint64 invalidate();
    quint64 currentGeneration() const;
    void renderFinished(quint64 generation);

signals:
    void renderRequested(const QSize& targetSize, quint64 generation, bool authoritative);

private:
    void queueNextRefinement(quint64 generation);
    void requestNextRefinement(quint64 generation);

    QTimer m_interactiveTimer;
    QSize m_sourceSize;
    QVector<QSize> m_refinementSizes;
    int m_nextRefinementIndex = 0;
    quint64 m_generation = 0;
    quint64 m_refinementGeneration = 0;
};

#endif // PROGRESSIVERENDERCONTROLLER_H
