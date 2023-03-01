#include "jstextarea.h"
#include <QTextEdit>
#include <QKeyEvent>
#include <QTextCursor>
#include <QTextBlock>
#include <QDebug>
#include <QSyntaxHighlighter>


// Syntax highlighter, from Qt Example
Highlighter::Highlighter(QTextDocument *parent)
      : QSyntaxHighlighter(parent)
  {
      HighlightingRule rule;

      keywordFormat.setForeground(Qt::darkBlue);
      keywordFormat.setFontWeight(QFont::Bold);
      QStringList keywordPatterns;
      keywordPatterns << "\\babstract\\b" << "\\barguments\\b" << "\\bboolean\\b" << "\\bbreak\\b" << "\\bbytecase\\b"
                      << "\\bcatch\\b" << "\\bchar\\b" << "\\bclass\\b" << "\\bconst\\b" << "\\bcontinue\\b"
                      << "\\bdebugger\\b" << "\\bdefault\\b" << "\\bdelete\\b" << "\\bdo\\b" << "\\bdouble\\b"
                      << "\\belse\\b" << "\\benum*\\b" << "\\beval\\b" << "\\bexport\\b" << "\\bextends*\\b"
                      << "\\bfalse\\b" << "\\bfinal\\b" << "\\bfinally\\b" << "\\bfloat\\b" << "\\bfor\\b"
                      << "\\bfunction\\b" << "\\bgoto\\b" << "\\bif\\b" << "\\bimplements\\b" << "\\bimport\\b"
                      << "\\bin\\b" << "\\binstanceof\\b" << "\\bint\\b" << "\\binterface\\b" << "\\blet\\b"
                      << "\\blong\\b" << "\\bnative\\b" << "\\bnew\\b" << "\\bnull\\b" << "\\bpackage\\b"
                      << "\\bprivate\\b" << "\\bprotected\\b" << "\\bpublic\\b" << "\\breturn\\b" << "\\bshort\\b"
                      << "\\bstatic\\b" << "\\bsuper\\b" << "\\bswitch\\b" << "\\bsynchronized\\b" << "\\bthis\\b"
                      << "\\bthrow\\b" << "\\bthrows\\b" << "\\btransient\\b" << "\\btrue\\b" << "\\btry\\b"
                      << "\\btypeof\\b" << "\\bvar\\b" << "\\bvoid\\b" << "\\bvolatile\\b" << "\\bwhile\\b"
                      << "\\bwith\\b" << "\\byield\\b";
      foreach (const QString &pattern, keywordPatterns) {
          rule.pattern = QRegExp(pattern);
          rule.format = keywordFormat;
          highlightingRules.append(rule);
      }

      classFormat.setFontWeight(QFont::Bold);
      classFormat.setForeground(Qt::darkMagenta);
      rule.pattern = QRegExp("\\bQ[A-Za-z]+\\b");
      rule.format = classFormat;
      highlightingRules.append(rule);

      singleLineCommentFormat.setForeground(Qt::darkGreen);
      rule.pattern = QRegExp("//[^\n]*");
      rule.format = singleLineCommentFormat;
      highlightingRules.append(rule);

      multiLineCommentFormat.setForeground(Qt::darkGreen);

      quotationFormat.setForeground(Qt::gray);
      rule.pattern = QRegExp("\".*\"");
      rule.format = quotationFormat;
      highlightingRules.append(rule);

      singleQuotationFormat.setForeground(Qt::gray);
      rule.pattern = QRegExp("\'.*\'");
      rule.format = singleQuotationFormat;
      highlightingRules.append(rule);

      functionFormat.setFontItalic(true);
      functionFormat.setForeground(Qt::blue);
      rule.pattern = QRegExp("\\b[A-Za-z0-9_]+(?=\\()");
      rule.format = functionFormat;
      highlightingRules.append(rule);

      commentStartExpression = QRegExp("/\\*");
      commentEndExpression = QRegExp("\\*/");
  }

  void Highlighter::highlightBlock(const QString &text)
  {
      foreach (const HighlightingRule &rule, highlightingRules) {
          QRegExp expression(rule.pattern);
          int index = expression.indexIn(text);
          while (index >= 0) {
              int length = expression.matchedLength();
              setFormat(index, length, rule.format);
              index = expression.indexIn(text, index + length);
          }
      }
      setCurrentBlockState(0);

      int startIndex = 0;
      if (previousBlockState() != 1)
          startIndex = commentStartExpression.indexIn(text);

      while (startIndex >= 0) {
          int endIndex = commentEndExpression.indexIn(text, startIndex);
          int commentLength;
          if (endIndex == -1) {
              setCurrentBlockState(1);
              commentLength = text.length() - startIndex;
          } else {
              commentLength = endIndex - startIndex
                              + commentEndExpression.matchedLength();
          }
          setFormat(startIndex, commentLength, multiLineCommentFormat);
          startIndex = commentStartExpression.indexIn(text, startIndex + commentLength);
      }
  }



/* JSTextArea implementation */

JSTextArea::JSTextArea(QWidget *parent) : QTextEdit(parent)
{
    QFont font;
    font.setFamily("Courier");
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
    font.setPointSize(10);
    setFont(font);
    highlighter = new Highlighter(document());
    const int tabStop = 2;  // 4 characters

    QFontMetrics metrics(font);
    setTabStopDistance(tabStop * metrics.horizontalAdvance(' '));
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


