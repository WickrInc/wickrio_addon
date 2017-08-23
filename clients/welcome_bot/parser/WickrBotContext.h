#ifndef WICKRBOTCONTEXT_H
#define WICKRBOTCONTEXT_H

// this class is used to connect up to whatever the local environment is

// Context is what it's named in Android land

class WickrBotContext
{
    static QString filesdir;

public:
    static void initialize() {
        theContext = new WickrAppContext();
        filesdir = QStandardPaths::writableLocation( QStandardPaths::DataLocation );
        QDir dir = QDir::root();
        dir.mkpath(filesdir);
    }

    static QDir getDir(QString filename) {
        QString dname = getFilePath(filename);
        QDir dir(dname);
        if( !dir.exists() )
            dir.mkdir(dname);
        return dir;
    }

    static QString getFilesDir() {
        return filesdir + "/";
    }

    static QStringList getFilesList() {
        QDir home(filesdir);
        return home.entryList(QDir::Files);
    }

    static QString getFilePath(QString filename) {
        return getFilesDir() + filename;
    }

    static QFile *getFileStreamPath(QString filename) {
        return new QFile(getFilePath(filename));
    }

    static bool deleteFile(QString filename);
};

#endif // WICKRBOTCONTEXT_H
