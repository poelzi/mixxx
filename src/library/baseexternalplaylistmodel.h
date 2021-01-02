#pragma once

#include <QItemDelegate>
#include <QModelIndex>
#include <QObject>
#include <QString>
#include <QStringBuilder>
#include <QtSql>

#include "library/basesqltablemodel.h"
#include "library/dao/playlistdao.h"
#include "library/dao/trackdao.h"
#include "library/librarytablemodel.h"
#include "library/trackmodel.h"
#include "util/string.h"

class BaseExternalPlaylistModel : public BaseSqlTableModel {
    Q_OBJECT
  public:
    BaseExternalPlaylistModel(QObject* pParent, TrackCollectionManager* pTrackCollectionManager,
                              const char* settingsNamespace, const QString& playlistsTable,
                              const QString& playlistTracksTable, QSharedPointer<BaseTrackCache> trackSource);

    ~BaseExternalPlaylistModel() override;

    void setPlaylist(const QString& path_name);

    TrackPointer getTrack(const QModelIndex& index) const override;
    TrackId getTrackId(const QModelIndex& index) const override;
    bool isColumnInternal(int column) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    CapabilitiesFlags getCapabilities() const override;
    QString modelKey() const override {
        return QLatin1String("external/") +
                QString::number(m_currentPlaylistId) +
                QLatin1String("#") +
                currentSearch();
    }

  private:
    TrackId doGetTrackId(const TrackPointer& pTrack) const override;

    QString m_playlistsTable;
    QString m_playlistTracksTable;
    QSharedPointer<BaseTrackCache> m_trackSource;
    int m_currentPlaylistId;
};
