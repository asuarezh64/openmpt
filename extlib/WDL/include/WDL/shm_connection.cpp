#include "shm_connection.h"

#ifdef _WIN32
#define SHM_MINSIZE 64
#define SHM_HDRSIZE (4+4*4+2)
  // 4 bytes: buffer size (each channel)
  // 4 bytes: read pointer chan 0
  // 4 bytes: write pointer chan 0
  // 4 bytes: read pointer chan 1
  // 4 bytes: write pointer chan 1
  // 2 bytes: chan 0 refresh, chan 1 refresh
  // data follows (2x buffer size)


WDL_SHM_Connection::WDL_SHM_Connection(bool whichChan,
                      const char *uniquestring, // identify 
                      int shmsize, // bytes, whoever opens first decides
                      int timeout_sec

                    )
{
  m_timeout_sec=timeout_sec;
  m_last_recvt=time(NULL)+2; // grace period
  { // make shmsize the next power of two
    int a = shmsize;
    shmsize=2;
    while (shmsize < SHM_MINSIZE || shmsize<a) shmsize*=2;
  }

  m_file=INVALID_HANDLE_VALUE;
  m_filemap=NULL;
  m_mem=NULL;
  m_events[0]=m_events[1]=NULL;

  m_whichChan=whichChan ? 1 : 0;

  char buf[512];
  GetTempPath(sizeof(buf)-4,buf);
  if (!buf[0]) lstrcpyn(buf,"C:\\",32);
  if (buf[strlen(buf)-1] != '/' && buf[strlen(buf)-1] != '\\') strcat(buf,"\\");
  m_tempfn.Set(buf);
  m_tempfn.Append("WDL_SHM_");
  m_tempfn.Append(uniquestring);
  m_tempfn.Append(".tmp");


  HANDLE mutex=NULL;

  {
    WDL_String tmp;
    tmp.Set("WDL_SHM_");
    tmp.Append(uniquestring);
    tmp.Append(".mutex");
    mutex = CreateMutex(NULL,FALSE,tmp.Get());
  }

  if (mutex) WaitForSingleObject(mutex,INFINITE);

  DeleteFile(m_tempfn.Get()); // this is designed to fail if another process has it locked

  m_file=CreateFile(m_tempfn.Get(),GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE ,
        NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_TEMPORARY,NULL);
  
  int mapsize;
  if (m_file != INVALID_HANDLE_VALUE && 
        ((mapsize=GetFileSize(m_file,NULL)) < SHM_HDRSIZE+SHM_MINSIZE*2 || 
          mapsize == 0xFFFFFFFF))
  {
    WDL_HeapBuf tmp;
    memset(tmp.Resize(shmsize*2 + SHM_HDRSIZE),0,shmsize*2 + SHM_HDRSIZE);
    ((int *)tmp.Get())[0] = shmsize;
    DWORD d;
    WriteFile(m_file,tmp.Get(),tmp.GetSize(),&d,NULL);
  }

  if (mutex) 
  {
    ReleaseMutex(mutex);
    CloseHandle(mutex);
  }

  if (m_file!=INVALID_HANDLE_VALUE)
    m_filemap=CreateFileMapping(m_file,NULL,PAGE_READWRITE,0,0,NULL);

  if (m_filemap)
  {
    m_mem=(unsigned char *)MapViewOfFile(m_filemap,FILE_MAP_WRITE,0,0,0);

    WDL_String tmp;
    if (!(GetVersion()&0x80000000)) tmp.Set("Global\\WDL_SHM_");
    else tmp.Set("WDL_SHM_");
    tmp.Append(uniquestring);
    tmp.Append(".1");
    m_events[0]=CreateEvent(NULL,false,false,tmp.Get());
    tmp.Get()[strlen(tmp.Get())-1]++; 
    m_events[1]=CreateEvent(NULL,false,false,tmp.Get());
  }
}


WDL_SHM_Connection::~WDL_SHM_Connection()
{
  if (m_mem) UnmapViewOfFile(m_mem);
  if (m_filemap) CloseHandle(m_filemap);
  if (m_file != INVALID_HANDLE_VALUE) CloseHandle(m_file);
  DeleteFile(m_tempfn.Get());

  if (m_events[0]) CloseHandle(m_events[0]);
  if (m_events[1]) CloseHandle(m_events[1]);
}

bool WDL_SHM_Connection::WantSendKeepAlive() { return false; }

int WDL_SHM_Connection::Run()
{
  if (!m_mem) return -1;

  int *hdr = (int *)m_mem;

  int shm_size = hdr[0];

  // todo: check to see if we just opened, if so, have a grace period
  if (shm_size < SHM_MINSIZE) return -1; 

  m_mem[4*5 + !!m_whichChan] = 1;
  if (m_timeout_sec > 0)
  {
    if (m_mem[4*5 + !m_whichChan])
    {
      m_last_recvt=time(NULL);
      m_mem[4*5 + !m_whichChan]=0;
    }
    else
    {
      if (time(NULL) > m_timeout_sec+m_last_recvt) return -1;
    }
  }

  int didStuff=0;

  // process writes
  int send_avail=send_queue.Available();
  if (send_avail>0)
  {
    int wc = !m_whichChan;
    unsigned char *data=m_mem+SHM_HDRSIZE+shm_size*wc;
    int rdptr = hdr[1 + wc*2];  // hopefully atomic
    int wrptr = hdr[1 + wc*2+1];
    int wrlen = shm_size - (wrptr-rdptr);
    if (wrlen > 0)
    {
      if (wrlen > send_avail) wrlen=send_avail;
      if (wrlen > shm_size) wrlen=shm_size; // should never happen !

      int idx = wrptr & (shm_size-1);
      int l = shm_size - idx;
      if (l > wrlen) l = wrlen;
      memcpy(data+idx,send_queue.Get(),l);
      if (l < wrlen) memcpy(data,(char*)send_queue.Get() + l,wrlen-l);

      hdr[1 + wc*2+1] = wrptr + wrlen; // advance write pointer, hopefluly atomic

      didStuff|=1;

      send_queue.Advance(wrlen);
      send_queue.Compact();

    }
  }

  // process reads
  {
    int wc = m_whichChan;
    unsigned char *data=m_mem+SHM_HDRSIZE+shm_size*wc;
    int rdptr = hdr[1 + wc*2]; 
    int wrptr = hdr[1 + wc*2+1]; // hopefully atomic
    int rdlen = wrptr-rdptr;
    if (rdlen > 0)
    {
      if (rdlen > shm_size) rdlen=shm_size; // should never happen !

      int idx = rdptr & (shm_size-1);
      int l = shm_size - idx;
      if (l > rdlen) l = rdlen;
      recv_queue.Add(data+idx,l);
      if (l < rdlen) recv_queue.Add(data,rdlen-l);

      hdr[1 + wc*2] = wrptr; // hopefully atomic, bring read pointer up to write pointer
      didStuff|=2;
    }
  }

  if (didStuff)
  {
    if (m_events[!m_whichChan]) SetEvent(m_events[!m_whichChan]);
    return 1;
  }


  return 0;
}


#else

#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include "swell/swell-internal.h"

static void sigpipehandler(int sig) { }

// socket version
WDL_SHM_Connection::WDL_SHM_Connection(bool whichChan, // first created must be whichChan=0
                      const char *uniquestring, // identify 
                      int shmsize, 
                      int timeout_sec)
{
  m_sockbufsize = shmsize;
  if (m_sockbufsize<16384) m_sockbufsize=16384;
  else if (m_sockbufsize>1024*1024) m_sockbufsize=1024*1024;

  m_rdbufsize = min(m_sockbufsize,65536);
  m_rdbuf = (char *)malloc(m_rdbufsize);

  static bool hasSigHandler;
  if (!hasSigHandler) 
  {
    signal(SIGPIPE,sigpipehandler);
    hasSigHandler=true;
  }
  m_timeout_sec=timeout_sec;
  m_last_recvt=time(NULL)+3; // grace period
  m_next_keepalive = time(NULL)+1;
  
  m_tempfn.Set("/tmp/WDL_SHM.");
  m_tempfn.Append(uniquestring);
  m_tempfn.Append(".tmp");

  if (!whichChan) unlink(m_tempfn.Get());

  m_sockaddr=malloc(sizeof(struct sockaddr_un) + strlen(m_tempfn.Get()));
  m_listen_socket=0;
  m_socket=0;
  m_isConnected=false;
  m_waitevt=0;

  struct sockaddr_un *addr = (struct sockaddr_un *)m_sockaddr;
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path,m_tempfn.Get());
  #ifdef __APPLE__
    int l = SUN_LEN(addr)+1;
    if (l>255)l=255;
    addr->sun_len=l;
  #endif

  int s = socket(AF_UNIX,SOCK_STREAM,0);
  if (s==-1) return;

  if (!whichChan)
  {
    bind(s,(struct sockaddr*)addr,SUN_LEN(addr));
    listen(s,1);

    m_listen_socket = s;
    m_isConnected=false;
    
    fcntl(s, F_SETFL, fcntl(s,F_GETFL) | O_NONBLOCK); 
  }
  else
  {
    m_socket = s;
    fcntl(s, F_SETFL, fcntl(s,F_GETFL) | O_NONBLOCK);

    int bsz=m_sockbufsize;
    setsockopt(s,SOL_SOCKET,SO_SNDBUF,(char *)&bsz,sizeof(bsz));
    bsz=m_sockbufsize;
    setsockopt(s,SOL_SOCKET,SO_RCVBUF,(char *)&bsz,sizeof(bsz));
    
    m_isConnected = !connect(s,(struct sockaddr*)addr,SUN_LEN(addr));
  } 

  if (m_socket || m_listen_socket)
  { 
    SWELL_InternalObjectHeader_SocketEvent *se = (SWELL_InternalObjectHeader_SocketEvent*)malloc(sizeof(SWELL_InternalObjectHeader_SocketEvent));
    memset(se,0,sizeof(SWELL_InternalObjectHeader_SocketEvent));
    se->hdr.type = INTERNAL_OBJECT_EXTERNALSOCKET;
    se->hdr.count = 1;
    se->socket[0]=m_socket? m_socket : m_listen_socket;
    m_waitevt = (HANDLE)se;
  }
}

WDL_SHM_Connection::~WDL_SHM_Connection()
{
  if (m_socket) close(m_socket);
  if (m_listen_socket) close(m_listen_socket);
  free(m_waitevt); // don't CloseHandle(), since its just referencing our socket
  free(m_sockaddr);

  if (m_tempfn.Get()[0]) unlink(m_tempfn.Get());
  free(m_rdbuf);
}

bool WDL_SHM_Connection::WantSendKeepAlive() 
{ 
  return !send_queue.GetSize() && time(NULL) >= m_next_keepalive;
}


int WDL_SHM_Connection::Run()
{
  if (!m_isConnected) 
  {
    if (m_listen_socket)
    {
      struct sockaddr_un remote={0,};
      socklen_t t = sizeof(struct sockaddr_un);
      int s;
      if ((s=accept(m_listen_socket,(struct sockaddr *)&remote,&t))>0)
      {
        close(m_listen_socket); 
        m_listen_socket=0;
        m_socket=s;

        fcntl(s, F_SETFL, fcntl(s,F_GETFL) | O_NONBLOCK); // nonblocking

        int bsz=m_sockbufsize;
        setsockopt(s,SOL_SOCKET,SO_SNDBUF,(char *)&bsz,sizeof(bsz));
        bsz=m_sockbufsize;
        setsockopt(s,SOL_SOCKET,SO_RCVBUF,(char *)&bsz,sizeof(bsz));

        if (m_waitevt)
        { 
          SWELL_InternalObjectHeader_SocketEvent *se = (SWELL_InternalObjectHeader_SocketEvent*)m_waitevt;
          se->socket[0]=s;
        }
        m_isConnected=true;
      }
    }
    else if (m_socket)
    {
      fd_set f;
      FD_ZERO(&f);
      FD_SET(m_socket,&f);
      struct timeval tv={0,};
      m_isConnected = select(m_socket+1,NULL,&f,NULL,&tv) > 0 && FD_ISSET(m_socket,&f);
    }
    if (!m_isConnected && m_timeout_sec>0 && time(NULL) > m_timeout_sec+m_last_recvt) return -1;
    if (!m_isConnected) return 0;
  }
  if (!m_socket) return -1;

  bool sendcnt=false;
  bool recvcnt=false;
  for (;;)
  {
    bool hadAct=false;
    while (recv_queue.Available()<128*1024*1024) 
    {
      int n=read(m_socket,m_rdbuf,m_rdbufsize);
      if (n>0)
      {
        recv_queue.Add(m_rdbuf,n);
        hadAct=true;
        recvcnt=true;
      }
      else if (n<0&&errno!=EAGAIN) { close(m_socket); m_socket=0; return -1; }
      else break;
    }
    while (send_queue.Available()>0)
    {
      int n = send_queue.Available();
      if (n > m_rdbufsize) n=m_rdbufsize;
      n = write(m_socket,send_queue.Get(),n);
      if (n > 0)
      {
        hadAct=true;
        sendcnt=true;
        send_queue.Advance(n); 
      }
      else if (n<0&&errno!=EAGAIN) { close(m_socket); m_socket=0; return -1; }
      else break;
    }
    if (!hadAct) break;  
  }
  if (sendcnt) send_queue.Compact();
  
  if (m_timeout_sec>0)
  {
    time_t now = time(NULL);
    if (recvcnt) m_last_recvt=now; 
    else if (now > m_timeout_sec+m_last_recvt) return -1;
    
    if (sendcnt||send_queue.GetSize()) m_next_keepalive = now + (m_timeout_sec+1)/2;
  }
  
  return sendcnt||recvcnt;
}
#endif
