#include "TimeshiftBuffer.h"
#include "client.h"
#include "platform/util/util.h"

using namespace ADDON;

/* indicate that caller can handle truncated reads, where function returns before entire buffer has been filled */
#define READ_TRUNCATED 0x01

/* indicate that that caller support read in the minimum defined chunk size, this disables internal cache then */
#define READ_CHUNKED   0x02

/* use cache to access this file */
#define READ_CACHED     0x04

/* open without caching. regardless to file type. */
#define READ_NO_CACHE  0x08

/* calcuate bitrate for file while reading */
#define READ_BITRATE   0x10

TimeshiftBuffer::TimeshiftBuffer(CStdString streampath, CStdString bufferpath)
  : m_bufferPath(bufferpath)
{
  XBMC->Log(LOG_DEBUG, "timeshift source=%s", streampath.c_str());
  m_streamHandle = XBMC->OpenFile(streampath, READ_CHUNKED | READ_NO_CACHE);
  m_bufferPath += "/tsbuffer.ts";
  m_filebufferWriteHandle = XBMC->OpenFileForWrite(m_bufferPath, true);
  m_writePos = 0;
  Sleep(100);
  m_filebufferReadHandle = XBMC->OpenFile(m_bufferPath, READ_CHUNKED | READ_NO_CACHE);
  m_running = true;
  CreateThread();
}

TimeshiftBuffer::~TimeshiftBuffer(void)
{
  Stop();
  if (IsRunning())
    StopThread();

  if (m_filebufferWriteHandle)
    XBMC->CloseFile(m_filebufferWriteHandle);
  if (m_filebufferReadHandle)
    XBMC->CloseFile(m_filebufferReadHandle);
  if (m_streamHandle)
    XBMC->CloseFile(m_streamHandle);
}

bool TimeshiftBuffer::IsValid()
{
  return (m_streamHandle != NULL);
}

void TimeshiftBuffer::Stop()
{
  m_running = false;
}

void *TimeshiftBuffer::Process()
{
  XBMC->Log(LOG_DEBUG, "Timeshift: thread started");
  byte buffer[STREAM_READ_BUFFER_SIZE];

  while (m_running)
  {
    unsigned int read = XBMC->ReadFile(m_streamHandle, buffer, sizeof(buffer));
    XBMC->WriteFile(m_filebufferWriteHandle, buffer, read);

    m_mutex.Lock();
    m_writePos += read;
    m_mutex.Unlock();
  }
  XBMC->Log(LOG_DEBUG, "Timeshift: thread stopped");
  return NULL;
}

long long TimeshiftBuffer::Seek(long long position, int whence)
{
  if (m_filebufferReadHandle)
    return XBMC->SeekFile(m_filebufferReadHandle, position, whence);
  return -1;
}

long long TimeshiftBuffer::Position()
{
  if (m_filebufferReadHandle)
    return XBMC->GetFilePosition(m_filebufferReadHandle);
  return -1;
}

long long TimeshiftBuffer::Length()
{
  if (m_filebufferReadHandle)
    return XBMC->GetFileLength(m_filebufferReadHandle);
  return 0;
}

int TimeshiftBuffer::ReadData(unsigned char *buffer, unsigned int size)
{
  if (!m_filebufferReadHandle)
    return 0;

  /* make sure we never read above the current write position */
  int64_t readPos  = XBMC->GetFilePosition(m_filebufferReadHandle);
  int64_t writePos = 0;
  unsigned int timeWaited = 0;

WAIT:
  m_mutex.Lock();
  writePos = m_writePos;
  m_mutex.Unlock();
  //FIXME ugly hack
  if (timeWaited > BUFFER_READ_TIMEOUT)
  {
    XBMC->Log(LOG_DEBUG, "Timeshift: Read timed out; waited %u", timeWaited);
    return -1;
  }

  if (readPos + size > writePos)
  {
    Sleep(BUFFER_READ_WAITTIME);
    timeWaited += BUFFER_READ_WAITTIME;
    goto WAIT;
  }

  unsigned int read = XBMC->ReadFile(m_filebufferReadHandle, buffer, size);

#if 0
  /* make sure we never read above the current write position */
  int64_t readPos  = XBMC->GetFilePosition(m_filebufferReadHandle);
  int64_t writePos = 0;
  do
  {
    /* refresh write position */
    m_mutex.Lock();
    writePos = m_writePos;
    m_mutex.Unlock();
    //TODO add some sort of timeout
  }
  while(readPos + size > writePos);

  unsigned int read = XBMC->ReadFile(m_filebufferReadHandle, buffer, size);

  unsigned int timeWaited = 0;
  while (read < size && timeWaited < BUFFER_READ_TIMEOUT)
  {
    Sleep(BUFFER_READ_WAITTIME);
    timeWaited += BUFFER_READ_WAITTIME;
    read += XBMC->ReadFile(m_filebufferReadHandle, buffer, size - read);
  }

  if (timeWaited > BUFFER_READ_TIMEOUT)
    XBMC->Log(LOG_DEBUG, "Timeshift: Read timed out; waited %u", timeWaited);
#endif


  return read;
}
