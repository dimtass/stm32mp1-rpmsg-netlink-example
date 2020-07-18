#include <sys/socket.h>
#include <linux/netlink.h>
#include "log.h"

class NetlinkClient {
public:
    NetlinkClient();
    ~NetlinkClient();
    size_t send(const uint8_t* buffer, size_t buffer_len, uint8_t* resp);

private:
    int m_sock_fd;
    struct sockaddr_nl m_src_addr;
    struct sockaddr_nl m_dest_addr;
    struct nlmsghdr *m_nlh;
};