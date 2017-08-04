#ifndef ANALYZER_AnalyzerFingerprint_H
#define ANALYZER_AnalyzerFingerprint_H

#include <QHash>
#include <QString>
#include <chromaprint.h>

#include "analyzer/analyzer.h"
#include "util/audiosignal.h"
//#include "analyzer/vamp/vampanalyzer.h"
#include "preferences/usersettings.h"
#include "track/track.h"

class AnalyzerFingerprint : public Analyzer {
  public:
    AnalyzerFingerprint(UserSettingsPointer pConfig);
    virtual ~AnalyzerFingerprint();

    bool initialize(TrackPointer tio, int sampleRate, int totalSamples) override;
    bool isDisabledOrLoadStoredSuccess(TrackPointer tio) const override;
    void process(const CSAMPLE *pIn, const int iLen) override;
    void finalize(TrackPointer tio) override;
    void cleanup(TrackPointer tio) override;

  private:
    static QHash<QString, QString> getExtraVersionInfo(
        QString pluginId, bool bPreferencesFastAnalysis);

    UserSettingsPointer m_pConfig;
    ChromaprintContext *m_chromactx;
    QString m_pluginId;
    int m_iSampleRate;
    int m_iTotalSamples;

    bool m_bPreferencesDuplicatedetectionEnabled;
    bool m_bPreferencesFastAnalysisEnabled;
    bool m_bPreferencesReanalyzeEnabled;
    static constexpr int kChannelCount = mixxx::AudioSignal::kChannelCountStereo;

};

#endif /* ANALYZER_AnalyzerFingerprint_H */
