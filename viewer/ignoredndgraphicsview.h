#ifndef IGNOREDNDGRAPHICSVIEW_H
#define IGNOREDNDGRAPHICSVIEW_H

#include <QGraphicsView>

/**
 * @brief Для некоторых d&d событий подставляется реализация из QWidget, которая просто пробрасывает их наверх
 */
class IgnoreDndGraphicsView : public QGraphicsView
{
public:
    IgnoreDndGraphicsView(QWidget *parent = nullptr) : QGraphicsView(parent) {}
    IgnoreDndGraphicsView(QGraphicsScene *scene, QWidget *parent = nullptr) : QGraphicsView(scene, parent) {}

    // QWidget interface
protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    static bool needIgnore(QDropEvent *event);
};

#endif // IGNOREDNDGRAPHICSVIEW_H
