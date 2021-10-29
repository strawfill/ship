#ifndef GRAPHICSVIEWZOOMER_H
#define GRAPHICSVIEWZOOMER_H

#include <QObject>
#include <QPointF>

class QGraphicsView;

/**
 * @brief Класс фильтра событий для QGraphicsView, чтобы включить там зуммирование
 *
 * Пример использование:
 * GraphicsViewZoomer* z = new GraphicsViewZoomer(ui->graphicsView);
 * z->set_modifiers(Qt::NoModifier);
 *
 * Взято из:
 * https://stackoverflow.com/questions/19113532/qgraphicsview-zooming-in-and-out-under-mouse-position-using-mouse-wheel
 * (там второй ответ что ли)
 *
 */
class GraphicsViewZoomer : public QObject
{
    Q_OBJECT
public:
    explicit GraphicsViewZoomer(QGraphicsView* view);
    void gentle_zoom(double factor);
    void set_modifiers(Qt::KeyboardModifiers modifiers);
    void set_zoom_factor_base(double value);

private:
    QGraphicsView* _view;
    Qt::KeyboardModifiers _modifiers;
    double _zoom_factor_base;
    QPointF target_scene_pos, target_viewport_pos;
    bool eventFilter(QObject* object, QEvent* event);

signals:
    void zoomed();
};

#endif // GRAPHICSVIEWZOOMER_H
