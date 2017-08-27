#include "util/gitannex.h"
#include <QObject>
#include <QProcess>
#include <QDir>
#include <QDebug>
#include <QString>
#include <QFileInfo>
#include <QRegExp>

GitAnnex::GitAnnex(QObject *parent) : QObject(parent)
{

}

bool GitAnnex::runAnnexCommand(QString base, QStringList args, QtJson::JsonObject &result, bool json) {
    QProcess annex;
    annex.setWorkingDirectory(base);
    QStringList xargs;
    xargs << "annex";
    if(json) {
        xargs << "-J";
    }
    xargs << args;
    annex.start("git", xargs);
    if (!annex.waitForStarted()) {
        qDebug() << "can't start git annex";
        // *result = nullptr;
        return false;
    }
    if (!annex.waitForFinished()) {
        return false;
    }

    QString output = annex.readAllStandardOutput();
    qDebug() << "git annex output" << output;

    QString stderr = annex.readAllStandardError();
    result.insert("_stderr", stderr);
    result.insert("_exitcode", annex.exitCode());

    if(json) {
        bool json_ok;
        QtJson::JsonObject jresult = QtJson::parse(output, json_ok).toMap();

        if(!json_ok) {
            result.insert("_stdout", output);
            return false;
        }
        qDebug() << "jresult:"; // << jresult.toString();
        qDebug() << QtJson::serialize(jresult);
        /*QMapIterator<QString, QVariant> i();
        while (i.hasNext()) {
            i.next();
            cout << i.key() << ": " << i.value().toString() << endl;
        }
        */
    } else {
        result.insert("_stdout", output);
    }

    if (annex.exitCode() != 0) {
        qDebug() << "Error executing git annex: " << stderr;
        return false;
    }
    return true;
}

int GitAnnex::gitannex_installed = -1;
QString GitAnnex::gitannex_problem = nullptr;

bool GitAnnex::checkAvailability(bool refresh) {
    if(!refresh && GitAnnex::gitannex_installed != -1) {
        return (bool)GitAnnex::gitannex_installed;
    }
    QProcess process;
    process.start("git", QStringList() << "annex" << "version");
    if (!process.waitForStarted()) {
        GitAnnex::gitannex_installed = 0;
        GitAnnex::gitannex_problem = "git annex is not installed";
        return false;
    }
    process.closeWriteChannel();

    if (!process.waitForFinished()) {
        GitAnnex::gitannex_installed = 0;
        GitAnnex::gitannex_problem = "could not run git annex";
        return false;
    }

    QByteArray result = process.readAll();

    qDebug() << result;
    QRegExp rx("^git-annex version: (\\d+)\\.(\\d{4})(\\d{2})(\\d{2}).*");
    QList<int> vertuple;

    if (rx.exactMatch(result)) {
        vertuple << rx.cap(1).toInt();
        vertuple << rx.cap(2).toInt();
        vertuple << rx.cap(3).toInt();
        vertuple << rx.cap(4).toInt();
    }
    // FIXME: check which version is lowest
    if (vertuple.length() < 4 || vertuple[0] < 5) {
        GitAnnex::gitannex_installed = 0;
        GitAnnex::gitannex_problem = "Installed version of git annex is too old";
        return false;
    }
    return true;
}

QString GitAnnex::getProblem() {
    return GitAnnex::gitannex_problem;
}

bool GitAnnex::isAnnexedFile(QString location) {
    QFileInfo fi(location);
    // FIXME: this likely needs some different test on windows
    if (fi.isSymLink()) {
        QString target = fi.symLinkTarget();
        QRegExp annex_test("(.*|(\\.\\./)*).git/annex/objects/.*", Qt::CaseInsensitive, QRegExp::RegExp2);
        if(annex_test.exactMatch(target)) {
            return true;
        }
        return false;
    }
    return false;
}

AnnexRequestState GitAnnex::ensureExists() {
    //QDir(m_location)
    QFileInfo fi(m_location);
    QString git_dir;
    QString fetch;
    if(fi.isFile()) {
        if(fi.exists()) {
            return AnnexRequestState::AVAILABLE;
        }
    } else if(fi.isSymLink()) {
        if(fi.exists()) {
            // file is a git annex symlink and points to a existing resource.
            return AnnexRequestState::AVAILABLE;
        }
        git_dir = fi.dir().absolutePath();
        fetch = fi.fileName();
    } else {
        git_dir = fi.absolutePath();
        fetch = ".";
    }
    return AnnexRequestState::TRANSFER;
}

