/*
 * Compile command: gcc sctp_server_sp.c -o sctp_server_sp -lsctp
 */
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

struct AddrPortStrings
{
  char addrString[NI_MAXHOST];
  char portString[NI_MAXSERV];
};

static int addressToNameAndPort(
  const struct sockaddr* address,
  socklen_t addressSize,
  struct AddrPortStrings* addrPortStrings)
{
  int retVal;
  if ((retVal = getnameinfo(address,
                            addressSize,
                            addrPortStrings->addrString,
                            NI_MAXHOST,
                            addrPortStrings->portString,
                            NI_MAXSERV,
                            (NI_NUMERICHOST | NI_NUMERICSERV))) != 0)
  {
    printf("getnameinfo error: %s\n", gai_strerror(retVal));
    return -1;
  }
  return 0;
}

static void printLocalAddresses(int socket)
{
  struct sockaddr* addresses;
  int retVal;
  if ((retVal = sctp_getladdrs(socket, 0, &addresses)) < 0)
  {
    printf("printLocalAddresses sctp_getladdrs error\n");
  }
  else
  {
    int i;
    for (i = 0; i < retVal; ++i)
    {
      struct AddrPortStrings addrPortStrings;
      socklen_t addressSize = 0;
      if (addresses[i].sa_family == AF_INET)
      {
        addressSize = sizeof(struct sockaddr_in);
      }
      else if (addresses[i].sa_family == AF_INET6)
      {
        addressSize = sizeof(struct sockaddr_in6);
      }
      if (addressToNameAndPort(&addresses[i], addressSize, &addrPortStrings) == 0)
      {
        printf("local address %d %s:%s\n", i, addrPortStrings.addrString, addrPortStrings.portString);
      }
    }
    sctp_freeladdrs(addresses);
  }
}

static void printPeerAddresses(int socket)
{
  struct sockaddr* addresses;
  int retVal;
  if ((retVal = sctp_getpaddrs(socket, 0, &addresses)) < 0)
  {
    printf("printPeerAddresses sctp_getladdrs error\n");
  }
  else
  {
    int i;
    for (i = 0; i < retVal; ++i)
    {
      struct AddrPortStrings addrPortStrings;
      socklen_t addressSize = 0;
      if (addresses[i].sa_family == AF_INET)
      {
        addressSize = sizeof(struct sockaddr_in);
      }
      else if (addresses[i].sa_family == AF_INET6)
      {
        addressSize = sizeof(struct sockaddr_in6);
      }
      if (addressToNameAndPort(&addresses[i], addressSize, &addrPortStrings) == 0)
      {
        printf("peer address %d %s:%s\n", i, addrPortStrings.addrString, addrPortStrings.portString);
      }
    }
    sctp_freepaddrs(addresses);
  }
}


static void printSctpNotification(union sctp_notification* notification)
{
  switch (notification->sn_header.sn_type)
  {
  case SCTP_SN_TYPE_BASE:
    printf("SCTP_SN_TYPE_BASE\n");
    break;
  case SCTP_ASSOC_CHANGE:
  {
    struct sctp_assoc_change* assocChange = &(notification->sn_assoc_change);
    printf("SCTP_ASSOC_CHANGE\n");
    printf("sac_type = %d\n", assocChange->sac_type);
    printf("sac_outbound_streams = %d\n", assocChange->sac_outbound_streams);
    printf("sac_inbound_streams = %d\n", assocChange->sac_inbound_streams);
    printf("sac_state ");
    switch (assocChange->sac_state)
    {
    case SCTP_COMM_UP:
      printf("SCTP_COMM_UP\n");
      break;
    case SCTP_COMM_LOST:
      printf("SCTP_COMM_LOST\n");
      break;
    case SCTP_RESTART:
      printf("SCTP_RESTART\n");
      break;
    case SCTP_SHUTDOWN_COMP:
      printf("SCTP_SHUTDOWN_COMP\n");
      break;
    case SCTP_CANT_STR_ASSOC:
      printf("SCTP_CANT_STR_ASSOC\n");
      break;
    default:
      printf("unknown %d\n", assocChange->sac_state);
      break;
    }
    break;
  }
  case SCTP_PEER_ADDR_CHANGE:
  {
    struct sctp_paddr_change* addrChange = &(notification->sn_paddr_change);
    struct AddrPortStrings addrPortStrings;
    socklen_t addressSize;

    printf("SCTP_PEER_ADDR_CHANGE\n");
    printf("spc_state ");
    switch (addrChange->spc_state)
    {
    case SCTP_ADDR_AVAILABLE:
      printf("SCTP_ADDR_AVAILABLE\n");
      break;
    case SCTP_ADDR_UNREACHABLE:
      printf("SCTP_ADDR_UNREACHABLE\n");
      break;
    case SCTP_ADDR_REMOVED:
      printf("SCTP_ADDR_REMOVED\n");
      break;
    case SCTP_ADDR_ADDED:
      printf("SCTP_ADDR_ADDED\n");
      break;
    case SCTP_ADDR_MADE_PRIM:
      printf("SCTP_ADDR_MADE_PRIM\n");
      break;
    case SCTP_ADDR_CONFIRMED:
      printf("SCTP_ADDR_CONFIRMED\n");
      break;
    default:
      printf("unknown %d\n", addrChange->spc_state);
    }
    addressSize = 0;
    if (addrChange->spc_aaddr.ss_family == AF_INET)
    {
      addressSize = sizeof(struct sockaddr_in);
    }
    else if (addrChange->spc_aaddr.ss_family == AF_INET6)
    {
      addressSize = sizeof(struct sockaddr_in6);
    }
    if (addressToNameAndPort((struct sockaddr*)(&(addrChange->spc_aaddr)),
                             addressSize, &addrPortStrings) == 0)
    {
      printf("spc_aaddr %s:%s\n", addrPortStrings.addrString, addrPortStrings.portString);
    }
    break;
  }
  case SCTP_SEND_FAILED:
    printf("SCTP_SEND_FAILED\n");
    break;
  case SCTP_REMOTE_ERROR:
    printf("SCTP_REMOTE_ERROR\n");
    break;
  case SCTP_SHUTDOWN_EVENT:
    printf("SCTP_SHUTDOWN_EVENT\n");
    break;
  case SCTP_PARTIAL_DELIVERY_EVENT:
    printf("SCTP_PARTIAL_DELIVERY_EVENT\n");
    break;
  case SCTP_ADAPTATION_INDICATION:
    printf("SCTP_ADAPTATION_INDICATION\n");
    break;
  case SCTP_AUTHENTICATION_INDICATION:
    printf("SCTP_AUTHENTICATION_INDICATION\n");
    break;
  default:
    printf("unknown sn_type %d\n", notification->sn_header.sn_type);
    break;
  }
}

int main(int argc, char** argv)
{
  struct addrinfo hints;
  struct addrinfo* addressInfo = NULL;
  int i;
  int retVal;
  int optval;
  int serverSocket;
  struct sockaddr_storage clientAddress;
  struct AddrPortStrings clientAddrPortStrings;
  struct sctp_event_subscribe events;
  char buffer[2000];
  struct sctp_sndrcvinfo sndrcvinfo;
  int flags;
  socklen_t clientAddressSize;

  if (argc != 3)
  {
    printf("Usage: %s <address> <port>\n", argv[0]);
    return 1;
  }

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_SEQPACKET;
  hints.ai_protocol = IPPROTO_SCTP;
  hints.ai_flags = (AI_V4MAPPED | AI_ADDRCONFIG);

  if (retVal = getaddrinfo(argv[1], argv[2],
                           &hints, &addressInfo) != 0)
  {
    printf("getaddrinfo failed, retVal = %d\n", retVal);
    return 1;
  }

  serverSocket = socket(addressInfo->ai_family,
                        addressInfo->ai_socktype,
                        addressInfo->ai_protocol);
  if (serverSocket < 0)
  {
    printf("error creating serverSocket, errno = %d\n", errno);
    return 1;
  }
  printf("created serverSocket %d\n", serverSocket);

  optval = 1;
  if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
  {
    printf("error setting SO_REUSEADDR errno = %d\n", errno);
    return 1;
  }

  retVal = bind(serverSocket,
                addressInfo->ai_addr,
                addressInfo->ai_addrlen);
  if (retVal < 0)
  {
    printf("bind failed, errno = %d\n", errno);
    return 1;
  }

  if (listen(serverSocket, SOMAXCONN) < 0)
  {
    printf("listen failed, errno = %d\n", errno);
    return 1;
  }

  memset(&events, 1, sizeof(events));
  retVal = setsockopt(serverSocket, SOL_SCTP, SCTP_EVENTS, &events, sizeof(events));
  if (retVal < 0)
  {
    printf("setsockopt(SCTP_EVENTS) error errno = %d\n", errno);
    return 1;
  }

  printLocalAddresses(serverSocket);

  while (1)
  {
    flags = 0;
    clientAddressSize = sizeof(clientAddress);
    printf("before sctp_recvmsg\n");
    retVal = sctp_recvmsg(serverSocket, buffer, 2000, 
                          (struct sockaddr*)&clientAddress, &clientAddressSize,
                          &sndrcvinfo, &flags);
    printf("sctp_recvmsg returns %d\n", retVal);
    if (retVal < 0)
    {
      printf("errno = %d\n", errno);
    }
    else if (flags & MSG_NOTIFICATION)
    {
      printf("got MSG_NOTIFICATION\n");
      printSctpNotification((union sctp_notification*)buffer);
    }
    else if (flags & MSG_EOR)
    {
      printf("got MSG_EOR\n");

      if (addressToNameAndPort(
            (struct sockaddr*)&clientAddress, clientAddressSize, &clientAddrPortStrings) < 0)
      {
        continue;
      }

      printf("received message length %d from client %s:%s\n",
             retVal,
             clientAddrPortStrings.addrString, clientAddrPortStrings.portString);

      printf("call sctp_sendmsg\n");
      retVal = sctp_sendmsg(serverSocket, buffer, retVal, 
                            (struct sockaddr*)&clientAddress, 
                            clientAddressSize, 0, 0, 0, 0, 0);
      printf("sctp_sendmsg returns %d\n", retVal);
    }
  }

  return 0;
}
