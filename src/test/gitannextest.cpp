#include <gtest/gtest.h>

#include "mixxxtest.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryFile>
//#include <QTemporaryDir>
#include <QtDebug>
#include <QProcess>
#include <QTimer>
#include "json.h"

#include "util/assert.h"
#include "util/gitannex.h"

namespace {

const QDir kTestDir(QDir::current().absoluteFilePath("src/test/gitannex-test-data"));

class GitAnnexTest : public MixxxTest {};

class FileRemover final {
public:
    explicit FileRemover(const QString& fileName)
        : m_fileName(fileName) {
    }
    ~FileRemover() {
        QFile::remove(m_fileName);
    }
private:
    QString m_fileName;
};

bool removeDir(const QString & dirName)
{
    bool result = true;
    QDir dir(dirName);

    if (dir.exists(dirName)) {
        Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
            qDebug() << "remove: " << info.baseName();
            if (info.isDir()) {
                result = removeDir(info.absoluteFilePath());
            }
            else {
                result = QFile::remove(info.absoluteFilePath());
            }

            if (!result) {
                return result;
            }
        }
        result = dir.rmdir(dirName);
    }
    return result;
}

class DirRemover final {
public:
    explicit DirRemover(const QString& fileName)
        : m_dirName(fileName) {
    }
    ~DirRemover() {
        removeDir(m_dirName);
    }
private:
    QString m_dirName;
};


QString generateTemporaryFileName(const QString& fileNameTemplate) {
    QTemporaryFile tmpFile(fileNameTemplate);
    tmpFile.open();
    DEBUG_ASSERT(tmpFile.exists());
    const QString tmpFileName(tmpFile.fileName());
    FileRemover tmpFileRemover(tmpFileName);
    return tmpFileName;
}

QString generateTemporaryDir(const QString& dirTemplate, bool createDir=true) {
    QDir tmp_path = QDir(QDir::tempPath());
    if(createDir) {
        tmp_path.mkdir(dirTemplate);
    }
    return tmp_path.filePath(dirTemplate);
}

void copyFile(const QString& srcFileName, const QString& dstFileName) {
    QFile srcFile(srcFileName);
    DEBUG_ASSERT(srcFile.exists());
    qDebug() << "Copying file"
            << srcFileName
            << "->"
            <<dstFileName;
    srcFile.copy(dstFileName);
    QFile dstFile(dstFileName);
    DEBUG_ASSERT(dstFile.exists());
    DEBUG_ASSERT(srcFile.size() == dstFile.size());
}



QString initRepo(const QString& target) {
    QString repo_dir = generateTemporaryDir(target);
    QObject *parent;
    QString program = "";
    QStringList arguments;
    arguments << "init";

    QProcess *git = new QProcess();
    git->setWorkingDirectory(repo_dir);
    git->start("git", arguments);
    qDebug() << "init repo: " << repo_dir;


    QStringList arg2;
    arg2 << "init";
    QtJson::JsonObject result;
    GitAnnex::runAnnexCommand(repo_dir, arg2, result, false);
    return repo_dir;
    /*QProcess *annex = new QProcess();
    git->setWorkingDirectory(repo_dir);
    QStringList arguments;
    arguments << "annex" << "init";
    git->start("git", arguments);
    */
    //GitAnnex::runAnnexCommand(QString base, QStringList args, QtJson::JsonObject &result, bool json)
}


TEST_F(GitAnnexTest, TestGitAnnexAvailability) {
    qDebug() << "gav" << GitAnnex::checkAvailability();
    EXPECT_TRUE(GitAnnex::checkAvailability());
    //EXPECT_NULL(GitAnnex::checkAvailability());
    //EXPECT_EQ(NULL, GitAnnex::getProblem());
}

TEST_F(GitAnnexTest, TestGitAnnexCommand) {
    QString repo_dir = generateTemporaryDir("mixxtests");
    QStringList args;
    args << "itsainvalidcommandforsure";
    QtJson::JsonObject result;

    EXPECT_FALSE(GitAnnex::runAnnexCommand(repo_dir, args, result, false));
    // runAnnexCommand will push the stderr and exit code into the result
    EXPECT_EQ(result.size(), 3);
    EXPECT_GT(result["_stderr"].toString().length(), 100);

    result.clear();

    // running invalid with json on
    EXPECT_FALSE(GitAnnex::runAnnexCommand(repo_dir, args, result, true));
    // runAnnexCommand will push the stderr and exit code into the result
    EXPECT_EQ(result.size(), 3);
    EXPECT_GT(result["_stderr"].toString().length(), 100);


}

TEST_F(GitAnnexTest, TestGitAnnexGet) {
    // Generate a file name for the temporary file
    //const QString tmpFileName = generateTemporaryFileName("no_id3v1_mp3");
//    int argc = 1;
//    char* argv[] = { "mixxx-test", NULL };
//    argv = (char *[]){"programName", "para1", "para2", "para3", NULL};
/*    char  arg0[] = "programName";
    char  arg1[] = "arg";
    char  arg2[] = "another arg";
    char* argv[] = { &arg0[0], &arg1[0], &arg2[0], NULL };
    int   argc   = (int)(sizeof(argv) / sizeof(argv[0])) - 1;
*/
    //QCoreApplication app(argc, &argv[0]);
    //MixxxTest::ApplicationScope applicationScope(argc, argv);

    //QCoreApplication app(argc, argv);

    QString repo1 = initRepo("annex1");
    QString repo2 = generateTemporaryDir("annex2", false);

    // write some files and add them to git annex
    QString test_file1 = QDir(repo1).filePath("test_file1");
    QFile tf1(test_file1);
    tf1.open(QIODevice::ReadWrite);
    tf1.write("test1");
    tf1.close();

    // create a symbolic link that does not point to a annexed file for testing
    QFile::link("/dev/null", QDir(repo1).filePath("link1"));
    QFile::link(".git/notannex/something", QDir(repo1).filePath("link2"));

    EXPECT_FALSE(GitAnnex::isAnnexedFile(test_file1));
    EXPECT_FALSE(GitAnnex::isAnnexedFile(QDir(repo1).filePath("link1")));
    EXPECT_FALSE(GitAnnex::isAnnexedFile(QDir(repo1).filePath("link2")));

    QStringList args;
    args << "add" << test_file1;
    QtJson::JsonObject result;

    EXPECT_TRUE(GitAnnex::runAnnexCommand(repo1, args, result, false));
    EXPECT_TRUE(GitAnnex::isAnnexedFile(test_file1));

    args.clear();
    result.clear();
    args << "sync";

    EXPECT_TRUE(GitAnnex::runAnnexCommand(repo1, args, result, false));

    QStringList argsclone;
    argsclone << "clone" << repo1 << repo2;

    QProcess *git = new QProcess();
    //git->setWorkingDirectory(repo);
    git->start("git", argsclone);
    qDebug() << "err" << git->readAllStandardError();
    qDebug() << "err" << git->readAllStandardOutput();
    git->waitForFinished();

    args.clear();
    result.clear();
    args << "init";

    EXPECT_TRUE(GitAnnex::runAnnexCommand(repo2, args, result, false));

    // we now have a library with non synced files which we can use to test the file backend

    QString test_file1_pull = QDir(repo2).filePath("test_file1");
    GitAnnex ga(test_file1_pull);
    AnnexRequestState exists = ga.ensureExists();

    EXPECT_EQ(exists, AnnexRequestState::TRANSFER);


    //QTimer exitTimer;
    //QObject::connect(&exitTimer, &QTimer::timeout, &app, QCoreApplication::quit);
    //exitTimer.start();
    //application()->processEvents();
    //app.exec();

    //qDebug() << "init repo: " << repo_dir;


    /*
    // Create the temporary file by copying an exiting file
    copyFile(kTestDir.absoluteFilePath("empty.mp3"), tmpFileName);

    // Ensure that the temporary file is removed after the test
    FileRemover tmpFileRemover(tmpFileName);

    // Verify that the file has no tags
    {
        TagLib::MPEG::File mpegFile(
                TAGLIB_FILENAME_FROM_QSTRING(tmpFileName));
        EXPECT_FALSE(mixxx::taglib::hasID3v1Tag(mpegFile));
        EXPECT_FALSE(mixxx::taglib::hasID3v2Tag(mpegFile));
        EXPECT_FALSE(mixxx::taglib::hasAPETag(mpegFile));
    }

    // Write metadata -> only an ID3v2 tag should be added
    mixxx::TrackMetadata trackMetadata;
    trackMetadata.setTitle("title");
    ASSERT_EQ(OK, mixxx::taglib::writeTrackMetadataIntoFile(
            trackMetadata, tmpFileName, mixxx::taglib::FileType::MP3));

    // Check that the file only has an ID3v2 tag after writing metadata
    {
        TagLib::MPEG::File mpegFile(
                TAGLIB_FILENAME_FROM_QSTRING(tmpFileName));
        EXPECT_FALSE(mixxx::taglib::hasID3v1Tag(mpegFile));
        EXPECT_TRUE(mixxx::taglib::hasID3v2Tag(mpegFile));
        EXPECT_FALSE(mixxx::taglib::hasAPETag(mpegFile));
    }

    // Write metadata again -> only the ID3v2 tag should be modified
    trackMetadata.setTitle("title2");
    ASSERT_EQ(OK, mixxx::taglib::writeTrackMetadataIntoFile(
            trackMetadata, tmpFileName, mixxx::taglib::FileType::MP3));

    // Check that the file (still) only has an ID3v2 tag after writing metadata
    {
        TagLib::MPEG::File mpegFile(
                TAGLIB_FILENAME_FROM_QSTRING(tmpFileName));
        EXPECT_FALSE(mixxx::taglib::hasID3v1Tag(mpegFile));
        EXPECT_TRUE(mixxx::taglib::hasID3v2Tag(mpegFile));
        EXPECT_FALSE(mixxx::taglib::hasAPETag(mpegFile));
    }
    */
}

}  // anonymous namespace
