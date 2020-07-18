#include <cstring>
#include <sys/types.h>
#include <unistd.h>
#include "netlink_client.h"

#define NETLINK_USER 31     // Same with the kernel module
#define MAX_BLOCK_SIZE 2048

NetlinkClient::NetlinkClient()
{
    m_sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
    if(m_sock_fd < 0) {
        L_(lerror) << "Couldn't create a socket";
        exit(-1);
    }

    std::memset(&m_src_addr, 0, sizeof(m_src_addr));
    m_src_addr.nl_family = AF_NETLINK;
    m_src_addr.nl_pid = getpid(); /* self pid */

    bind(m_sock_fd, (struct sockaddr*)&m_src_addr, sizeof(m_src_addr));

    std::memset(&m_dest_addr, 0, sizeof(m_dest_addr));
    std::memset(&m_dest_addr, 0, sizeof(m_dest_addr));
    m_dest_addr.nl_family = AF_NETLINK;
    m_dest_addr.nl_pid = 0; /* For Linux Kernel */
    m_dest_addr.nl_groups = 0; /* unicast */
}

NetlinkClient::~NetlinkClient()
{
    if (m_sock_fd)
        close(m_sock_fd);
}


size_t NetlinkClient::send(const uint8_t* buffer, size_t buffer_len, uint8_t* resp)
{
    size_t tx_len = 0;

    int page_sz = getpagesize();
    m_nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(page_sz));
    L_(ldebug) << "Max netlink payload size: " << page_sz;
    if (!m_nlh) {
        L_(lerror) << "Failed to allocate nlmsg space";
        exit(-1);
    }

    struct msghdr msg;
    struct sockaddr_nl kernel;
    struct iovec iov;

    do {
        int n_tx = buffer_len < MAX_BLOCK_SIZE ?  buffer_len : MAX_BLOCK_SIZE;
        buffer_len -= n_tx;

        memset(&kernel, 0, sizeof(kernel));
        kernel.nl_family = AF_NETLINK;
        kernel.nl_groups = 0;

        memset(&iov, 0, sizeof(iov));
        iov.iov_base = (void *)m_nlh;
        iov.iov_len = n_tx;
        
        std::memset(m_nlh, 0, NLMSG_SPACE(n_tx));
        m_nlh->nlmsg_len = NLMSG_SPACE(n_tx);
        m_nlh->nlmsg_pid = getpid();
        m_nlh->nlmsg_flags = 0;

        std::memcpy(NLMSG_DATA(m_nlh), buffer, n_tx);

        memset(&msg, 0, sizeof(msg));
        msg.msg_name = &kernel;
        msg.msg_namelen = sizeof(kernel);
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;

        L_(ldebug) << "Sending " << n_tx << "/" << buffer_len;
        int err = sendmsg(m_sock_fd, &msg, 0);
        if (err < 0) {
            L_(lerror) << "Failed to send netlink message: " <<  err;
            return(0);
        }

    } while(buffer_len);



    /* Read message from kernel */
    L_(ldebug) << "Waiting for message from kernel";
    tx_len = recvmsg(m_sock_fd, &msg, 0);
    L_(ldebug) << "Received %d bytes: " << std::dec << tx_len;


    if (m_sock_fd)
        close(m_sock_fd);

    return(tx_len);
}