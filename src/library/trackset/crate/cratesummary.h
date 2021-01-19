#pragma once

#include <QObject>

#include "library/trackset/crate/crate.h"
#include "util/duration.h"

// A crate with aggregated track properties (total count + duration)
class CrateSummary : public QObject, public Crate {
    Q_OBJECT
  public:
    Q_PROPERTY(uint trackCount READ getTrackCount WRITE setTrackCount)
    Q_PROPERTY(double trackDuration READ getTrackDuration WRITE setTrackDuration)
    Q_PROPERTY(QString name READ getName WRITE setName)

    CrateSummary(CrateId id = CrateId());
    ~CrateSummary() override;

    // The number of all tracks in this crate
    uint getTrackCount() const {
        return m_trackCount;
    }
    void setTrackCount(uint trackCount) {
        m_trackCount = trackCount;
    }

    // The total duration (in seconds) of all tracks in this crate
    double getTrackDuration() const {
        return m_trackDuration;
    }
    void setTrackDuration(double trackDuration) {
        m_trackDuration = trackDuration;
    }
    // Returns the duration formatted as a string H:MM:SS
    QString getTrackDurationText() const {
        return mixxx::Duration::formatTime(getTrackDuration(), mixxx::Duration::Precision::SECONDS);
    }

  private:
    uint m_trackCount;
    double m_trackDuration;
};
