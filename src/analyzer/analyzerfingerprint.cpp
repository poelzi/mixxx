#include "analyzerfingerprint.h"

#include <QtDebug>
#include <QVector>

#include "util/audiosignal.h"

#include "proto/keys.pb.h"
#include "track/key_preferences.h"
#include "track/keyfactory.h"
#include "util/sample.h"
#include "util/samplebuffer.h"



using mixxx::track::io::key::ChromaticKey;
using mixxx::track::io::key::ChromaticKey_IsValid;
//using mixxx::AudioSource::kChannelCountStereo;

// Type declarations of *fprint and *encoded pointers need to account for Chromaprint API version
// (void* -> uint32_t*) and (void* -> char*) changed in versions v1.4.0 or later -- alyptik 12/2016
#if (CHROMAPRINT_VERSION_MINOR > 3) || (CHROMAPRINT_VERSION_MAJOR > 1)
    typedef uint32_t* uint32_p;
    typedef char* char_p;
#else
    typedef void* uint32_p;
    typedef void* char_p;
#endif

AnalyzerFingerprint::AnalyzerFingerprint(UserSettingsPointer pConfig)
        : m_pConfig(pConfig),
          m_iSampleRate(0),
          m_iTotalSamples(0),
          m_bPreferencesDuplicatedetectionEnabled(true),
          m_bPreferencesFastAnalysisEnabled(false),
          m_bPreferencesReanalyzeEnabled(false) {
}

AnalyzerFingerprint::~AnalyzerFingerprint() {
    // delete m_pVamp;
    if(m_chromactx) {
        chromaprint_free(m_chromactx);
    }
}

bool AnalyzerFingerprint::initialize(TrackPointer tio, int sampleRate, int totalSamples) {
    if (totalSamples == 0) {
        return false;
    }

    m_bPreferencesDuplicatedetectionEnabled = m_pConfig->getValue<bool>(
            ConfigKey("[Library]", "EnableFingerprinting"), true);
    if (!m_bPreferencesDuplicatedetectionEnabled) {
        qDebug() << "Fingerprinting disabled";
        return false;
    }

    m_iSampleRate = sampleRate;
    m_iTotalSamples = totalSamples;


    m_chromactx = chromaprint_new(CHROMAPRINT_ALGORITHM_DEFAULT);
    if(m_chromactx == NULL) {
        qDebug() << "can't create chromaprint context";
        return false;
    }
    if(!chromaprint_start(m_chromactx, sampleRate, kChannelCount)) {
        qDebug() << "error in chromaprint_start";
        return false;
    }
    return true;
}

bool AnalyzerFingerprint::isDisabledOrLoadStoredSuccess(TrackPointer tio) const {
        return false;
/*    bool bPreferencesFastAnalysisEnabled = m_pConfig->getValue<bool>(
            ConfigKey(KEY_CONFIG_KEY, KEY_FAST_ANALYSIS));

    QString library = m_pConfig->getValueString(
            ConfigKey(VAMP_CONFIG_KEY, VAMP_ANALYZER_KEY_LIBRARY));
    QString pluginID = m_pConfig->getValueString(
            ConfigKey(VAMP_CONFIG_KEY, VAMP_ANALYZER_KEY_PLUGIN_ID));

    // TODO(rryan): This belongs elsewhere.
    if (library.isEmpty() || library.isNull())
        library = "libmixxxminimal";

    // TODO(rryan): This belongs elsewhere.
    if (pluginID.isEmpty() || pluginID.isNull())
        pluginID = VAMP_ANALYZER_KEY_DEFAULT_PLUGIN_ID;

    const Keys keys(tio->getKeys());
    if (keys.isValid()) {
        QString version = keys.getVersion();
        QString subVersion = keys.getSubVersion();

        QHash<QString, QString> extraVersionInfo = getExtraVersionInfo(
            pluginID, bPreferencesFastAnalysisEnabled);
        QString newVersion = KeyFactory::getPreferredVersion();
        QString newSubVersion = KeyFactory::getPreferredSubVersion(extraVersionInfo);

        if (version == newVersion && subVersion == newSubVersion) {
            // If the version and settings have not changed then if the world is
            // sane, re-analyzing will do nothing.
            qDebug() << "Keys version/sub-version unchanged since previous analysis. Not analyzing.";
            return true;
        } else if (m_bPreferencesReanalyzeEnabled) {
            return false;
        } else {
            qDebug() << "Track has previous key detection result that is not up"
                     << "to date with latest settings but user preferences"
                     << "indicate we should not re-analyze it.";
            return true;
        }
    } else {
        // If we got here, we want to analyze this track.
        return false;
    }
*/
}


void AnalyzerFingerprint::process(const CSAMPLE *pIn, const int iLen) {
    std::vector<SAMPLE> fingerprintSamples(iLen * kChannelCount);
    // Convert floating-point to integer
    SampleUtil::convertFloat32ToS16(
            &fingerprintSamples[0],
            pIn,
            fingerprintSamples.size());
    //if(!chromaprint_feed(m_chromactx, , iLen)
    int success = chromaprint_feed(m_chromactx, &fingerprintSamples[0], fingerprintSamples.size());
    if (!success) {
        qDebug() << " error in chromaprint_feed ";
    }
    return;
    /*
    if (m_pVamp == NULL)
        return;
    bool success = m_pVamp->Process(pIn, iLen);
    if (!success) {
        delete m_pVamp;
        m_pVamp = NULL;
    }
    */
}

void AnalyzerFingerprint::cleanup(TrackPointer tio) {
    Q_UNUSED(tio);
    if(m_chromactx) {
        chromaprint_free(m_chromactx);
        m_chromactx = NULL;
    }
    //delete m_pVamp;
    //m_pVamp = NULL;
}

void AnalyzerFingerprint::finalize(TrackPointer tio) {
    /*if (m_pVamp == NULL) {
        return;
    }
    */
    int success =  chromaprint_finish(m_chromactx);

    //bool success = m_pVamp->End();
    //qDebug() << "Key Detection" << (success ? "complete" : "failed");
    qDebug() << "Fingerprint calculation finished" << (success ? "complete" : "failed");

    uint32_p fprint = NULL;
    int size, ret = 0;
    uint32_t hash = 0;
    ret = chromaprint_get_fingerprint_hash(m_chromactx, &hash);
    if (ret == 0) {
         qDebug() << "can't create hash";
         hash = 0;
    }
    qDebug() << "hash: " << hash;
    ret = chromaprint_get_raw_fingerprint(m_chromactx, &fprint, &size);
    QByteArray fingerprint;
    if (ret == 1) {
        char_p encoded = NULL;
        int encoded_size = 0;
        chromaprint_encode_fingerprint(fprint, size,
                                       CHROMAPRINT_ALGORITHM_DEFAULT,
                                       &encoded,
                                       &encoded_size, 1);

        fingerprint.append(reinterpret_cast<char*>(encoded), encoded_size);

        chromaprint_dealloc(fprint);
        chromaprint_dealloc(encoded);
    }
    chromaprint_free(m_chromactx);
    m_chromactx = NULL;

    qDebug() << "FP:" << fingerprint.toHex();
    //QVector<double> frames = m_pVamp->GetInitFramesVector();
    //QVector<double> keys = m_pVamp->GetLastValuesVector();
    // delete m_pVamp;
    // m_pVamp = NULL;
    //
    // if (frames.size() == 0 || frames.size() != keys.size()) {
    //     qWarning() << "AnalyzerFingerprint: Key sequence and list of times do not match.";
    //     return;
    // }
    //
    // KeyChangeList key_changes;
    // for (int i = 0; i < keys.size(); ++i) {
    //     if (ChromaticKey_IsValid(keys[i])) {
    //         key_changes.push_back(qMakePair(
    //             // int() intermediate cast required by MSVC.
    //             static_cast<ChromaticKey>(int(keys[i])), frames[i]));
    //     }
    // }
    //
    // QHash<QString, QString> extraVersionInfo = getExtraVersionInfo(
    //     m_pluginId, m_bPreferencesFastAnalysisEnabled);
    // Keys track_keys = KeyFactory::makePreferredKeys(
    //     key_changes, extraVersionInfo,
    //     m_iSampleRate, m_iTotalSamples);
    // tio->setKeys(track_keys);
}

// static
QHash<QString, QString> AnalyzerFingerprint::getExtraVersionInfo(
    QString pluginId, bool bPreferencesFastAnalysis) {
    QHash<QString, QString> extraVersionInfo;
    extraVersionInfo["vamp_plugin_id"] = pluginId;
    if (bPreferencesFastAnalysis) {
        extraVersionInfo["fast_analysis"] = "1";
    }
    return extraVersionInfo;
}
