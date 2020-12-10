#pragma once

#include <QDateTime>
#include <QFont>
#include <QItemSelectionModel>
#include <QString>
#include <QTableView>

#include "library/libraryview.h"
#include "preferences/usersettings.h"
#include "track/track_decl.h"

class TrackModel;

class WLibraryTableView : public QTableView, public virtual LibraryView {
    Q_OBJECT

    struct ModelState {
        int scrollPosition;
        QModelIndexList selectionIndex;
        QModelIndex currentIndex;
        qint64 lastChange;
    };

  public:
    WLibraryTableView(QWidget* parent,
            UserSettingsPointer pConfig,
            const ConfigKey& vScrollBarPosKey);
    ~WLibraryTableView() override;

    void moveSelection(int delta) override;

    /// @brief saveTrackModelState function saves current position of scrollbar
    /// using string key - can be any value but should invariant for model
    /// @param key unique for trackmodel
    void saveTrackModelState(const QAbstractItemModel* model, const QString& key);

    /// @brief restoreTrackModelState function finds scrollbar value associated with model
    /// by given key and restores it
    /// @param key unique for trackmodel
    void restoreTrackModelState(const QAbstractItemModel* model, const QString& key);
    /// @brief clears the state cache until it's size is = kClearModelStatesLowWatermark
    void clearStateCache();
    void saveCurrentViewState() override;
    void restoreCurrentViewState() override;

  signals:
    void loadTrack(TrackPointer pTrack);
    void loadTrackToPlayer(TrackPointer pTrack, const QString& group, bool play = false);
    void trackSelected(TrackPointer pTrack);
    void onlyCachedCoverArt(bool);
    void scrollValueChanged(int);

  public slots:
    void setTrackTableFont(const QFont& font);
    void setTrackTableRowHeight(int rowHeight);
    void setSelectedClick(bool enable);

  protected:
    void focusInEvent(QFocusEvent* event) override;
    virtual QString getStateKey() const = 0;

  private:
    const UserSettingsPointer m_pConfig;
    const ConfigKey m_vScrollBarPosKey;

    // saves scrollposition/selection/etc
    QMap<QString, ModelState*> m_vModelState;
};
