#include "DvbData.h"
#include "client.h"
#include "platform/util/util.h"
#include "tinyxml/tinyxml.h"
#include "tinyxml/XMLUtils.h"
#include <inttypes.h>
#include <set>
#include <iterator>
#include <sstream>
#include <algorithm>

using namespace ADDON;
using namespace PLATFORM;

Dvb::Dvb()
  : m_connected(false), m_backendVersion(0)
{
  // simply add user@pass in front of the URL if username/password is set
  CStdString auth("");
  if (!g_username.empty() && !g_password.empty())
    auth.Format("%s:%s@", g_username.c_str(), g_password.c_str());
  m_url.Format("http://%s%s:%u/", auth.c_str(), g_hostname.c_str(), g_webPort);
}

Dvb::~Dvb()
{
  StopThread();
}

bool Dvb::Open()
{
  CLockObject lock(m_mutex);

  m_connected = CheckBackendVersion();
  if (!m_connected)
    return false;

  XBMC->Log(LOG_INFO, "Starting separate polling thread...");
  CreateThread();

  return IsRunning();
}

bool Dvb::IsConnected()
{
  return m_connected;
}

CStdString Dvb::GetBackendName()
{
  // RS api doesn't provide a reliable way to extract the server name
  return "DVBViewer";
}

CStdString Dvb::GetBackendVersion()
{
  CStdString version;
  version.Format("%u.%u.%u.%u",
      m_backendVersion >> 24 & 0xFF,
      m_backendVersion >> 16 & 0xFF,
      m_backendVersion >> 8 & 0xFF,
      m_backendVersion & 0xFF);
  return version;
}

void *Dvb::Process()
{
  int updateTimer = 0;
  XBMC->Log(LOG_DEBUG, "%s starting", __FUNCTION__);

  while (!IsStopped())
  {
    Sleep(1000);
    ++updateTimer;
  }

  CLockObject lock(m_mutex);
  m_started.Broadcast();

  return NULL;
}

CStdString Dvb::GetHttpXML(const CStdString& url)
{
  CStdString result;
  void *fileHandle = XBMC->OpenFile(url, READ_NO_CACHE);
  if (fileHandle)
  {
    char buffer[1024];
    while (int bytesRead = XBMC->ReadFile(fileHandle, buffer, 1024))
      result.append(buffer, bytesRead);
    XBMC->CloseFile(fileHandle);
  }
  return result;
}

bool Dvb::CheckBackendVersion()
{
  //CStdString url = BuildURL("api/version.html");
  CStdString url("http://edy:edy@192.168.178.28:8089/api/version.html");
  CStdString req = GetHttpXML(url);

  TiXmlDocument doc;
  XBMC->Log(LOG_DEBUG, "VOR Parse: errorid=%d error=%d", doc.ErrorId(), doc.Error());
  doc.Parse(req);
  XBMC->Log(LOG_DEBUG, "NACH Parse: errorid=%d error=%d", doc.ErrorId(), doc.Error());
  if (doc.Error())
  {
    XBMC->Log(LOG_ERROR, "Unable to connect to the backend service. Error: %s",
        doc.ErrorDesc());
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30500));
    Sleep(10000);
    return false;
  }

  XBMC->Log(LOG_NOTICE, "Checking backend version...");
  if (doc.RootElement()->QueryUnsignedAttribute("iver", &m_backendVersion)
      != TIXML_SUCCESS)
  {
    XBMC->Log(LOG_ERROR, "Unable to parse version");
    return false;
  }
  XBMC->Log(LOG_NOTICE, "Version: %u", m_backendVersion);

  if (m_backendVersion < RS_VERSION_NUM)
  {
    XBMC->Log(LOG_ERROR, "Recording Service version %s or higher required",
        RS_VERSION_STR);
    XBMC->QueueNotification(QUEUE_ERROR, XBMC->GetLocalizedString(30501),
        RS_VERSION_STR);
    Sleep(10000);
    return false;
  }

  return true;
}

CStdString Dvb::BuildURL(const char* path, ...)
{
  CStdString url(m_url);
  va_list argList;
  va_start(argList, path);
  url.AppendFormatV(path, argList);
  va_end(argList);
  return url;
}

CStdString Dvb::BuildExtURL(const CStdString& baseURL, const char* path, ...)
{
  CStdString url(baseURL);
  // simply add user@pass in front of the URL if username/password is set
  if (!g_username.empty() && !g_password.empty())
  {
    CStdString auth;
    auth.Format("%s:%s@", g_username.c_str(), g_password.c_str());
    CStdString::size_type pos = url.find("://");
    if (pos != CStdString::npos)
      url.insert(pos + strlen("://"), auth);
  }
  va_list argList;
  va_start(argList, path);
  url.AppendFormatV(path, argList);
  va_end(argList);
  return url;
}
