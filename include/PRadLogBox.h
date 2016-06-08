#ifndef PRAD_LOG_BOX_H
#define PRAD_LOG_BOX_H

#include <QTextEdit>

#define MAX_NEWLOG_LENGTH 2048

class QFileSystemWatcher;

class PRadLogBox : public QTextEdit
{
    Q_OBJECT

public:
    PRadLogBox(QWidget *parent = NULL);
    virtual ~PRadLogBox();
    void ShowStdOut();
    void ShowStdErr();
    void TurnOnLog();
    void TurnOffLog();

private slots:
    void handleFileChange(QString path);

private:
    qint64 outpos;
    qint64 errpos;
    QFileSystemWatcher *fileWatcher;
    FILE *outRedir;
    FILE *errRedir;
    bool logOn;
};

#endif
