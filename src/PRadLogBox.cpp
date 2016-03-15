//============================================================================//
// Derived from QTextEdit, redirect stdout and stderr to files                //
// Using file watcher to monitor the files, and display the new input to the  //
// GUI (this file reading way takes resources, but it is currently the only   //
// feasible way I figured out)                                                //
//                                                                            //
// Chao Peng                                                                  //
// 03/13/2016                                                                 //
//============================================================================//

#include <QFileSystemWatcher>
#include <QFile>
#include <QTextStream>
#include "PRadLogBox.h"
#include <cstdio>
#include <unistd.h>

PRadLogBox::PRadLogBox(QWidget *parent)
: QTextEdit(parent)
{
    fileWatcher = new QFileSystemWatcher(this);
    connect(fileWatcher, SIGNAL(fileChanged(QString)), this, SLOT(showLogs(QString)));

    outRedir = freopen("logs/system.log", "w", stdout);
    errRedir = freopen("logs/error.log", "w", stderr);

    if(outRedir) {
        setvbuf(stdout,NULL,_IONBF,0);

        fileWatcher->addPath("logs/system.log");

        QFile file("logs/system.log");
        outpos = file.size();
    }

    if(errRedir) {
        setvbuf(stderr,NULL,_IONBF,0);

        fileWatcher->addPath("logs/error.log");

        QFile file("logs/error.log");
        errpos = file.size();
    }

    QTextEdit::setReadOnly(true);
}

PRadLogBox::~PRadLogBox()
{
    if(outRedir) {
        fclose(outRedir);
        stdout = fdopen(STDOUT_FILENO, "w");
    }

    if(errRedir) {
        fclose(errRedir);
        stderr = fdopen(STDERR_FILENO, "w");
    }

    delete fileWatcher;
}

void PRadLogBox::showLogs(QString path)
{
    QFile file(path);
    file.open(QFile::ReadOnly | QFile::Text);
    QString alertHtml = "<font color=\"Red\">";
    QString infoHtml = "<font color=\"Black\">";
    QString endHtml = "</font><br>";
    QString type;

    if(path.contains("system.log")) {
        type = infoHtml;
        file.seek(outpos);
        outpos = file.size();
    } else {
        type = alertHtml;
        file.seek(errpos);
        errpos = file.size();
    }

    QTextStream in(&file);

    while(!in.atEnd())
    {
        QString line = in.readLine();
        QTextEdit::moveCursor(QTextCursor::End);
        QTextEdit::insertHtml(type+line+endHtml);
    }

}
