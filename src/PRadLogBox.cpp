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
: QTextEdit(parent), fileWatcher(new QFileSystemWatcher(this)), logOn(false)
{
    connect(fileWatcher, SIGNAL(fileChanged(QString)), this, SLOT(handleFileChange(QString)));

    outRedir = freopen("logs/system.log", "a", stdout);
    setvbuf(stdout,NULL,_IONBF,0);

    errRedir = freopen("logs/error.log", "a", stderr);
    setvbuf(stderr,NULL,_IONBF,0);

    TurnOnLog();

    QTextEdit::setReadOnly(true);
}


PRadLogBox::~PRadLogBox()
{
    stdout = fdopen(STDOUT_FILENO, "w");
    stderr = fdopen(STDERR_FILENO, "w");
    fclose(outRedir);
    fclose(errRedir);
    delete fileWatcher;
}


void PRadLogBox::TurnOnLog()
{
    fileWatcher->addPath("logs/system.log");
    QFile out_file("logs/system.log");
    outpos = out_file.size();

    fileWatcher->addPath("logs/error.log");
    QFile err_file("logs/error.log");
    errpos = err_file.size();

    logOn = true;
}


void PRadLogBox::TurnOffLog()
{
    if(!logOn)
        return;

    fileWatcher->removePath("logs/system.log");
    fileWatcher->removePath("logs/error.log");
    logOn = false;
}


void PRadLogBox::handleFileChange(QString path)
{
    if(path.contains("system"))
        ShowStdOut();

    if(path.contains("error"))
        ShowStdErr();
}


void PRadLogBox::ShowStdOut()
{
    QFile file("logs/system.log");
    file.open(QFile::ReadOnly | QFile::Text);
    file.seek(outpos);
    outpos = file.size();

    QString header = "<font color=\"Black\">";
    QString end = "</font><br>";

    QTextStream in(&file);

    while(!in.atEnd())
    {
        QString line = in.readLine();
        QTextEdit::moveCursor(QTextCursor::End);
        QTextEdit::insertHtml(header+line+end);
    }
}


void PRadLogBox::ShowStdErr()
{
    QFile file("logs/error.log");
    file.open(QFile::ReadOnly | QFile::Text);
    file.seek(errpos);
    errpos = file.size();

    QString header = "<font color=\"Red\">";
    QString end = "</font><br>";

    QTextStream in(&file);

    while(!in.atEnd())
    {
        QString line = in.readLine();
        QTextEdit::moveCursor(QTextCursor::End);
        QTextEdit::insertHtml(header+line+end);
    }
}
