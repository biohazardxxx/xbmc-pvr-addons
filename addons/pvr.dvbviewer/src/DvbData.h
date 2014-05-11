#pragma once

#ifndef PVR_DVBVIEWER_DVBDATA_H
#define PVR_DVBVIEWER_DVBDATA_H

#include "client.h"
#include "platform/util/StdString.h"
#include "platform/threads/threads.h"
#include <list>

#define CHANNELDAT_HEADER_SIZE       (7)
#define ENCRYPTED_FLAG               (1 << 0)
#define RDS_DATA_FLAG                (1 << 2)
#define VIDEO_FLAG                   (1 << 3)
#define AUDIO_FLAG                   (1 << 4)
#define ADDITIONAL_AUDIO_TRACK_FLAG  (1 << 7)
#define DAY_SECS                     (24 * 60 * 60)
#define DELPHI_DATE                  (25569)

// minimum version required
#define RS_VERSION_MAJOR   1
#define RS_VERSION_MINOR   26
#define RS_VERSION_PATCH1  0
#define RS_VERSION_PATCH2  0
#define RS_VERSION_NUM  (RS_VERSION_MAJOR << 24 | RS_VERSION_MINOR << 16 | \
                          RS_VERSION_PATCH1 << 8 | RS_VERSION_PATCH2)
#define RS_VERSION_STR  XSTR(RS_VERSION_MAJOR) "." XSTR(RS_VERSION_MINOR) \
                          "." XSTR(RS_VERSION_PATCH1) "." XSTR(RS_VERSION_PATCH2)

class Dvb
  : public PLATFORM::CThread
{
public:
  Dvb(void);
  ~Dvb();

  bool Open();
  bool IsConnected();

  CStdString GetBackendName();
  CStdString GetBackendVersion();

protected:
  virtual void *Process(void);

private:
  // functions
  CStdString GetHttpXML(const CStdString& url);

  // helper functions
  bool CheckBackendVersion();
  CStdString BuildURL(const char* path, ...);
  CStdString BuildExtURL(const CStdString& baseURL, const char* path, ...);

private:
  bool m_connected;
  unsigned int m_backendVersion;

  CStdString m_url;

  PLATFORM::CMutex m_mutex;
  PLATFORM::CCondition<bool> m_started;
};

#endif
