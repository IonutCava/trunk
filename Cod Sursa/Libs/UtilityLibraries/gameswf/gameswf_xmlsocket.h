// gameswf_xml.h      -- Rob Savoye <rob@welcomehome.org> 2005

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifndef __XMLSOCKET_H__
#define __XMLSOCKET_H__


#include "base/tu_config.h"
#include "gameswf_xml.h"
#include "gameswf_impl.h"
#include "gameswf_log.h"
#include "base/container.h"

#ifdef HAVE_LIBXML

namespace gameswf {

extern const int SOCKET_DATA;

class XMLSocket {
public:
  XMLSocket();
  ~XMLSocket();
  
  bool connect(const char *host, int port);
  bool send(tu_string str);
  void close();

  
  bool anydata(char **msgs);
  bool anydata(int sockfd, char **msgs);
  bool connected() { return _connect; };
  bool fdclosed() { return _closed; }
  bool xmlmsg() { return _xmldata; }
  
  void messagesClear()      { _messages.clear(); }
  void messageRemove(int x) { _messages.remove(x); }
  int messagesCount()       { return _messages.size(); }
  tu_string operator [] (int x)  { return _messages[x]; }
  
  bool processingData();
  void processing(bool x);
 
  // Event Handlers
  void onClose(tu_string);
  void onConnect(tu_string);
  void onData(tu_string);
  void onXML(tu_string);

  // These handle the array of XML nodes
  void push(as_object_interface *obj);
  void clear();
  int  count();

  int XMLSocket::checkSockets(void);
  int XMLSocket::checkSockets(int x);

 private:
  tu_string     _host;
  short         _port;
  int           _sockfd;
  bool          _data;
  bool          _xmldata;
  bool          _closed;
  bool          _connect;
  bool          _processing;
  array<tu_string> _messages;
  array<as_object_interface *>  _nodes;
};


struct xmlsocket_as_object : public gameswf::as_object
{
  XMLSocket obj;
};

void xmlsocket_connect(const fn_call& fn);
void xmlsocket_send(const fn_call& fn);
void xmlsocket_xml_new(const fn_call& fn);
void xmlsocket_new(const fn_call& fn);
void xmlsocket_close(const fn_call& fn);

// These are the event handlers called for this object
void xmlsocket_event_ondata(const fn_call& fn);
void xmlsocket_event_close(const fn_call& fn);
void xmlsocket_event_connect(const fn_call& fn);
void xmlsocket_event_xml(const fn_call& fn);

int check_sockets(int fd);
 
} // end of gameswf namespace

// HAVE_LIBXML
#endif

// __XMLSOCKETSOCKET_H__
#endif

