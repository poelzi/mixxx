#pragma once

#include <QAction>
#include <QHeaderView>
#include <QMap>
#include <QMenu>
#include <QWidget>
#include <QContextMenuEvent>

#include "proto/headers.pb.h"

class TrackModel;

// Thanks to StackOverflow http://stackoverflow.com/questions/1163030/qt-qtableview-and-horizontalheader-restorestate
// answer with this code snippet: http://codepad.org/2gPIMPYU
class HeaderViewState {
public:
    HeaderViewState() {}

    // Populate the object based on the provided live view.
    explicit HeaderViewState(const QHeaderView& headers);

    // Populate from an existing protobuf, mostly for testing.
    explicit HeaderViewState(const mixxx::library::HeaderViewState& pb)
            : m_view_state(pb) { }

    // Populate the object with the serialized protobuf data provided.
    explicit HeaderViewState(const QString& base64serialized);

    // Returns a serialized protobuf of the current state.
    QString saveState() const;
    // Apply the state to the provided view.  The data in the object may be
    // changed if the header format has changed.
    void restoreState(QHeaderView* headers);

    // returns false if no headers are listed to be shown.
    bool healthy() const {
        if (m_view_state.header_state_size() == 0) {
            return false;
        }
        for (int i = 0; i < m_view_state.header_state_size(); ++i) {
            if (!m_view_state.header_state(i).hidden()) {
                return true;
            }
        }
        return false;
    }

private:
    mixxx::library::HeaderViewState m_view_state;
};


class WTrackTableViewHeader : public QHeaderView {
    Q_OBJECT
  public:
    explicit WTrackTableViewHeader(Qt::Orientation orientation, QWidget* parent = nullptr);

    void contextMenuEvent(QContextMenuEvent* event) override;
    void setModel(QAbstractItemModel* model) override;

    void saveHeaderState();
    void restoreHeaderState();
    void loadDefaultHeaderState();
     /** returns false if the header state is stored in the database (on first time usgae) **/
    bool hasPersistedHeaderState();
    bool getEnableSaveState() {
        return m_actionEnableSaveState.isChecked();
    }
    void setEnableSaveState(bool enable) {
        m_actionEnableSaveState.setChecked(enable);
    }

  private slots:
    void showOrHideColumn(int);
    void slotDeleteHeaderState();

  private:
    int hiddenCount();
    void clearActions();
    TrackModel* getTrackModel();
    void buildMenu();
    void ensureColumnsAreVisible();

    QMenu m_menu;
    QMap<int, QAction*> m_columnActions;
    QAction m_actionEnableSaveState;
    QAction m_actionForget;
    bool m_headerChanged;
};
