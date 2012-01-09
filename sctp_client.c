/*
 * Compile command: gcc sctp_client.c -o sctp_client -lsctp
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
    printf("SCTP_ASSOC_CHANGE\n");
    break;
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
  int clientSocket;
  struct sockaddr_storage clientAddress;
  socklen_t clientAddressSize;
  struct AddrPortStrings clientAddrPortStrings;
  struct sctp_event_subscribe events;
  char buffer[2000];
  bool connected = false;
  struct sctp_sndrcvinfo sndrcvinfo;
  int flags;

  printf("sizeof(union sctp_notification) = %d\n", sizeof(union sctp_notification));
  if (argc != 3)
  {
    printf("Usage: %s <address> <port>\n", argv[0]);
    return 1;
  }

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_SCTP;
  hints.ai_flags = (AI_V4MAPPED | AI_ADDRCONFIG);

  if (retVal = getaddrinfo(argv[1], argv[2],
                           &hints, &addressInfo) != 0)
  {
    printf("getaddrinfo failed, retVal = %d\n", retVal);
    return 1;
  }

  while (true)
  {
    clientSocket = socket(addressInfo->ai_family,
                          addressInfo->ai_socktype,
                          addressInfo->ai_protocol);
    if (clientSocket < 0)
    {
      printf("error creating clientSocket, errno = %d\n", errno);
      return 1;
    }
    printf("created clientSocket %d\n", clientSocket);

    if (connect(clientSocket,
                addressInfo->ai_addr,
                addressInfo->ai_addrlen) < 0)
    {
      printf("connect error, errno = %d %s\n", errno, strerror(errno));
      close(clientSocket);
      sleep(1);
      continue;
    }

    memset(&events, 1, sizeof(events));
    retVal = setsockopt(clientSocket, SOL_SCTP, SCTP_EVENTS, &events, sizeof(events));
    if (retVal < 0)
    {
      printf("setsockopt(SCTP_EVENTS) error errno = %d\n", errno);
      return 1;
    }

    printLocalAddresses(clientSocket);
    printPeerAddresses(clientSocket);

    connected = true;

    memset(buffer, 0, 2000);
    for (i = 0; i < 10; ++i)
    {
      buffer[i] = '0' + i;
    }
    retVal = sctp_sendmsg(clientSocket, buffer, 10, NULL, 0, 0, 0, 0, 0, 0);
    if (retVal < 0)
    {
      printf("sctp_sendmsg error errno = %d\n", errno);
      connected = false;
    }

    while (connected)
    {
      memset(buffer, 0, 2000);
      flags = 0;
      printf("call sctp_recvmsg\n");
      retVal = sctp_recvmsg(clientSocket, buffer, 2000, NULL, NULL, &sndrcvinfo, &flags);
      printf("sctp_recvmsg returned %d\n", retVal);
      if (retVal <= 0)
      {
        printf("errno = %d\n", errno);
        connected = false;
      }
      else if (flags & MSG_NOTIFICATION)
      {
        printf("got MSG_NOTIFICATION\n");
        printSctpNotification((union sctp_notification*)buffer);
      }
      else if (flags & MSG_EOR)
      {
        printf("got MSG_EOR\n");
        for (i = 0; (i < retVal) && (i < 2000); ++i)
        {
          printf("buffer[%d] = %d '%c'\n", i, buffer[i], buffer[i]);
        }
      }
    }

    close(clientSocket);
  }

  return 0;
}
