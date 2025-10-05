#include "TCPClient.hpp"
#include "protocol.hpp"

#include <cstring>
#include <cerrno>

using namespace protocol;
using namespace protocol::sql_protocol;

namespace tcpclient {

ClientTCP::ClientTCP(const std::string& ip, int port, std::atomic<bool>* stopAllPtr)
    : server_ip(ip), server_port(port), stop_all(stopAllPtr) {}

ClientTCP::~ClientTCP() {
    stop();
	std::cout << "ClientTCP is stoped!" << std::endl;
}

bool ClientTCP::start() {

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(server_port);
    inet_pton(AF_INET, server_ip.c_str(), &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        return false;
    }
	server_on.store(true);  // 將 atomic bool 設成 true

    set_nonblocking(sock);

    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        perror("epoll_create1");
        return false;
    }

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = sock;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock, &ev);

    notify_fd = eventfd(0, EFD_NONBLOCK);
    if (notify_fd < 0) {
        perror("eventfd");
        return false;
    }

    epoll_event ev_notify{};
    ev_notify.events = EPOLLIN;
    ev_notify.data.fd = notify_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, notify_fd, &ev_notify);

    running = true;

    recv_thread = std::thread(&ClientTCP::recv_loop, this);

    return true;
}

void ClientTCP::stop() {
	if(close_all.load()) return;
	close_all.store(true);
    running = false;

    if (notify_fd >= 0) {
        uint64_t one = 1;
        write(notify_fd, &one, sizeof(one));
    }

    if (recv_thread.joinable())
        recv_thread.join();

    if (sock >= 0)
        close(sock);

    if (notify_fd >= 0)
        close(notify_fd);

    if (epoll_fd >= 0)
        close(epoll_fd);
}

std::string ClientTCP::get_server_ip() const {
    return server_ip;
}

int ClientTCP::get_server_port() const {
    return server_port;
}

// 取得 ClientTCP IP
std::string ClientTCP::get_ip() const {
    sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    if (getsockname(sock, (sockaddr*)&addr, &len) == 0) {
        char buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr.sin_addr, buf, sizeof(buf));
        return std::string(buf);
    }
    return "";
}

// 取得 ClientTCP Port
int ClientTCP::get_port() const {
    sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    if (getsockname(sock, (sockaddr*)&addr, &len) == 0)
        return ntohs(addr.sin_port);
    return -1;
}

void ClientTCP::send_msg(const std::string& msg) {
    if (sock >= 0)
        send(sock, msg.c_str(), msg.size(), 0);
}

void ClientTCP::set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void ClientTCP::recv_loop() {

    const int MAX_EVENTS = 10;
    epoll_event events[MAX_EVENTS];

    while (running && (!stop_all || !*stop_all)) {
			
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        if (n < 0) {

            if (errno == EINTR)
                continue;

            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; ++i) {

            if (events[i].data.fd == sock) {

                char buf[MAX_BUFFERSIZE];
                ssize_t r = read(sock, buf, sizeof(buf));

                if (r > 0) {
                    std::string msg(buf, r);
					size_t pos = get_start_payload(msg);
					if(pos == -1) continue;
					const char* p = msg.c_str();
                    std::cout << "Received: " << p + pos << std::endl;
                } else if (r == 0) {
                    std::cout << "Server closed connection" << std::endl;
					server_on.store(false);  // 將 atomic bool 設成 true
                    running = false;
                } else { //r < 0
					if (errno == EINTR) continue;     // 被 signal 打斷
					if (errno == EAGAIN || errno == EWOULDBLOCK) continue; // 暫時沒資料
					perror("read");
					running = false;
				}

            } else if (events[i].data.fd == notify_fd) {
                uint64_t val;
                read(notify_fd, &val, sizeof(val));
				std::cout <<  "Notify stop recv_loop" <<std::endl;
                //running = false; //這裡不需要，因為我是被通知關掉!
            }
        }
    }		
}

bool ClientTCP::client_get_server_stats(){
	return server_on.load();
}

//目前先讀payload 而已
size_t ClientTCP::get_start_payload(const std::string& data) {
	const size_t min_pkg_size = 1 + 1 + 4 + 1; // StartByte + Type + nLen + EndByte
    if (data.size() < min_pkg_size) return -1;

    // 從頭掃描 StartByte
    size_t start_pos = std::string::npos;
    for (size_t i = 0; i <= data.size() - min_pkg_size; ++i) {
        if (data[i] == protocol::PACKAGE_START_BYTE) {
            start_pos = i;
            break;
        }
    }
    if (start_pos == std::string::npos) 
        return -1; // 沒找到 StartByte

    // 至少要能讀到 Type + nLen + EndByte (start_pos + 1 + 4 = PACKAGE_END_BYTE 位置)
    if (start_pos + 1 + 4 >= data.size()) 
        return -1;
    size_t offset = start_pos + 2; //len的位置

    // 讀 nLen (統一 big-endian)
    int32_t nLen = (static_cast<uint8_t>(data[offset])   << 24) |
				   (static_cast<uint8_t>(data[offset+1]) << 16) |
				   (static_cast<uint8_t>(data[offset+2]) << 8)  |
				   (static_cast<uint8_t>(data[offset+3]));
	
	if (nLen > MAX_BUFFERSIZE) 
        return -1; // 超過緩衝區長度

	size_t end_pos = start_pos + 1 + 4 + nLen + 1;
	if (end_pos >= data.size() || data[end_pos] != protocol::PACKAGE_END_BYTE) 
        return -1;

    return start_pos + 1 + 4 + 1;
}
} // namespace tcpclient
