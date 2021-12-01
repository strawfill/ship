#include "placeholderframe.h"

#include <QHBoxLayout>
#include <QLabel>

PlaceholderFrame::PlaceholderFrame(QWidget *parent) : QFrame(parent)
{
    setFrameStyle(QFrame::Box|QFrame::Plain);

    auto hlayout{ new QHBoxLayout(this) };
    auto label{ new QLabel(this) };
    label->setAlignment(Qt::AlignCenter);
    hlayout->addWidget(label);


    label->setText("\n\n\nДля начала работы перетащите или вставьте через " +
                   QKeySequence(QKeySequence::Paste).toString() +
                   "\nсюда файл или текст с исходными данными"
                   "\n\n\n\nСовет:"
                   "\nМасштаб графического отображения можно"
                   "\nизменять с помощью вращения колёсика мыши"
                   "\nдвумя разными по сути способосами:"
                   "\nС нажатым Ctrl"
                   "\nС нажатым Ctrl+Shift"
                   "\n");
}
