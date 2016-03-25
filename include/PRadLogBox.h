#ifndef PRAD_LOG_BOX_H
#define PRAD_LOG_BOX_H

#include <QTextEdit>

class QFileSystemWatcher;

class PRadLogBox : public QTextEdit
{
    Q_OBJECT

public:
    PRadLogBox(QWidget *parent = NULL);
    virtual ~PRadLogBox();

public slots:
    void showLogs(QString path);

private:
    qint64 outpos;
    qint64 errpos;
    QFileSystemWatcher *fileWatcher;
    FILE *outRedir;
    FILE *errRedir;
};

#endif
