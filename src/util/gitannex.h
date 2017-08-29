#ifndef GITANNEX_H
#define GITANNEX_H

#include <QObject>
//#include <QJsonDocument>
#include <json.h>

enum class AnnexRequestState
    {
        AVAILABLE,
        NOT_AVAILABLE,
        TRANSFER,
        ERROR
  };


// GitAnnexProgress is a ProxyObject that emits progress updates on git annex operations
// as signals
/*
class GitAnnexProgress : public QObject
{
    Q_OBJECT
public:


signals:

friend GitAnnex;
};
*/

class GitAnnex : public QObject
{
    Q_OBJECT
public:
    explicit GitAnnex(QObject *parent = nullptr);
    explicit GitAnnex(QString location): m_location(location) {};
    static bool checkAvailability(bool refresh=false);

    Q_PROPERTY(QString problem READ getProblem);
    Q_PROPERTY(bool isAvailable READ checkAvailability);
    static QString getProblem();

    static bool runAnnexCommand(QString base, QStringList args, QtJson::JsonObject &result, bool json = true);
    // checks if the file in question is handled by a git annex repository
    static bool isAnnexedFile(QString location);
    static QString lookupRepository(QString location);

    Q_PROPERTY(QString location READ getLocation);
    Q_PROPERTY(AnnexRequestState state READ getState)
    Q_PROPERTY(int progress READ getProgress)
    Q_PROPERTY(QString error READ getError)
    Q_PROPERTY(QString repository READ getRepository)

    QString getLocation() {
        return m_location;
    }
    AnnexRequestState getState() {
        return m_state;
    }

    QString getRepository() {
        return GitAnnex::lookupRepository(m_location);
    }

    QString getError() {
        return m_error;
    }

    int getProgress() {
        return m_progress;
    }

    QString getRepositoryPath();

    // processes the backend operation.
    bool getFile();


signals:
    void fetchComplete(bool success);
    // notifies over the new progress
    void progress(int newProgress, int eta);
    // state changed
    void stateChanged(AnnexRequestState state);

public slots:
    AnnexRequestState ensureExists();
    void cancel();

private:
    QString m_location;
    AnnexRequestState m_state;
    QString m_error;
    int m_progress;



// static values for reporting problems
    static int gitannex_installed;
    static QString gitannex_problem;
    QAtomicInt m_stop;
};

#endif // GITANNEX_H
