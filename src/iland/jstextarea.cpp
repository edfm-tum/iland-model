#include "jstextarea.h"
#include <QTextEdit>
#include <QKeyEvent>
#include <QTextCursor>
#include <QTextBlock>
#include <QDebug>

JSTextArea::JSTextArea(QWidget *parent) : QTextEdit(parent)
{
    QFont font;
    font.setFamily("Courier");
    font.setFixedPitch(true);
    font.setPointSize(10);
    setFont(font);
}

void JSTextArea::keyPressEvent ( QKeyEvent * event )
{
    if( event->key() == Qt::Key_Return && event->modifiers()== Qt::ControlModifier)
    {
        // find out what to do:
        // if no text is selected, then the current line should be executed
        // if a text is selected, then the selected text should be executed
        QString code = textCursor().selectedText();
        if (code.isEmpty()) {
            QTextCursor cursor = textCursor();
            code = cursor.block().text();
            cursor.movePosition(QTextCursor::NextBlock);
            setTextCursor(cursor);
        }
        qDebug() << "code:" << code;
        if (!code.isEmpty())
            emit executeJS(code);
        event->accept();
    }
    else
        QTextEdit::keyPressEvent( event );
}
