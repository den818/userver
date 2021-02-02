#include <utest/simple_server.hpp>

#include <engine/async.hpp>
#include <engine/io/socket.hpp>
#include <engine/mutex.hpp>
#include <engine/sleep.hpp>
#include <engine/task/cancel.hpp>
#include <logging/log.hpp>

#include <utest/utest.hpp>

namespace testing {

namespace {

class Client final {
 public:
  static void Run(engine::io::Socket socket, SimpleServer::OnRequest f);

 private:
  Client(engine::io::Socket socket, SimpleServer::OnRequest f)
      : socket_{std::move(socket)}, callback_{std::move(f)} {}

  [[nodiscard]] bool NeedsMoreReading() const {
    return (resp_.command == SimpleServer::Response::kTryReadMore);
  }

  [[nodiscard]] bool NeedsNewRequest() const {
    return (resp_.command == SimpleServer::Response::kWriteAndContinue);
  }

  void StartNewRequest() { incomming_data_.clear(); }

  std::size_t ReadSome();

  void WriteResponse();

  static constexpr std::size_t kReadBufferChunkSize = 1024 * 1024 * 1;

  engine::io::Socket socket_;
  SimpleServer::OnRequest callback_;

  SimpleServer::Request incomming_data_{};
  std::size_t previously_received_{0};
  SimpleServer::Response resp_{};
};

void Client::Run(engine::io::Socket socket, SimpleServer::OnRequest f) {
  LOG_TRACE() << "New client";
  Client c{std::move(socket), std::move(f)};
  do {
    const auto deadline = engine::Deadline::FromDuration(kMaxTestWaitTime);
    c.StartNewRequest();

    do {
      if (!c.ReadSome()) {
        return;
      }

      if (engine::current_task::IsCancelRequested()) {
        return;
      }

      if (deadline.IsReached()) {
        ADD_FAILURE() << "Shutting down slow request";
        return;
      }
    } while (c.NeedsMoreReading());

    c.WriteResponse();

  } while (c.NeedsNewRequest() && !engine::current_task::IsCancelRequested());
}

std::size_t Client::ReadSome() {
  previously_received_ = incomming_data_.size();
  incomming_data_.resize(previously_received_ + kReadBufferChunkSize);

  auto received =
      socket_.RecvSome(&incomming_data_[0] + previously_received_,
                       incomming_data_.size() - previously_received_, {});

  if (!received) {
    LOG_TRACE() << "Remote peer shut down the connection";
    return 0;
  }

  incomming_data_.resize(previously_received_ + received);
  resp_ = callback_(incomming_data_);

  return received;
}

void Client::WriteResponse() {
  [[maybe_unused]] size_t sent =
      socket_.SendAll(resp_.data_to_send.data(), resp_.data_to_send.size(), {});
}

}  // namespace

class SimpleServer::Impl {
 public:
  Impl(OnRequest callback, Protocol protocol);

  [[nodiscard]] Port GetPort() const { return port_; };
  [[nodiscard]] Protocol GetProtocol() const { return protocol_; };

 private:
  OnRequest callback_;
  Protocol protocol_;
  Port port_{};

  template <class F>
  void PushTask(F f) {
    engine::impl::Async(std::move(f)).Detach();
  }

  [[nodiscard]] engine::io::Addr MakeLoopbackAddress() const;
  void StartPortListening();
};

SimpleServer::Impl::Impl(OnRequest callback, Protocol protocol)
    : callback_{std::move(callback)}, protocol_{protocol} {
  EXPECT_TRUE(callback_)
      << "SimpleServer must be started with a request callback";

  StartPortListening();
}

engine::io::Addr SimpleServer::Impl::MakeLoopbackAddress() const {
  engine::io::AddrStorage addr_storage;

  switch (protocol_) {
    case kTcpIpV4: {
      auto* sa = addr_storage.As<struct sockaddr_in>();
      sa->sin_family = AF_INET;
      // NOLINTNEXTLINE(hicpp-no-assembler)
      sa->sin_port = 0;
      // NOLINTNEXTLINE(hicpp-no-assembler)
      sa->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      return engine::io::Addr(addr_storage, SOCK_STREAM, 0);
    }
    case kTcpIpV6: {
      auto* sa = addr_storage.As<struct sockaddr_in6>();
      sa->sin6_family = AF_INET6;
      // NOLINTNEXTLINE(hicpp-no-assembler)
      sa->sin6_port = 0;
      sa->sin6_addr = in6addr_loopback;
      return engine::io::Addr(addr_storage, SOCK_STREAM, 0);
    }
  }
}

void SimpleServer::Impl::StartPortListening() {
  engine::io::Addr addr = MakeLoopbackAddress();

  // Starting acceptor in this coro to avoid errors when acceptor has not
  // started listing yet and someone is already connecting...
  engine::io::Socket acceptor = engine::io::Listen(addr);
  switch (protocol_) {
    case Protocol::kTcpIpV4:
      port_ = ntohs(acceptor.Getsockname().As<sockaddr_in>()->sin_port);
      break;
    case Protocol::kTcpIpV6:
      port_ = ntohs(acceptor.Getsockname().As<sockaddr_in6>()->sin6_port);
      break;
  }

  PushTask([this, cb = callback_, acceptor = std::move(acceptor)]() mutable {
    while (!engine::current_task::IsCancelRequested()) {
      auto socket = acceptor.Accept({});

      LOG_TRACE() << "SimpleServer accepted socket";

      PushTask([cb = cb, s = std::move(socket)]() mutable {
        Client::Run(std::move(s), cb);
      });
    }
  });
}

SimpleServer::SimpleServer(OnRequest callback, Protocol protocol)
    : pimpl_{std::make_unique<Impl>(std::move(callback), protocol)} {}

SimpleServer::~SimpleServer() = default;

unsigned short SimpleServer::GetPort() const { return pimpl_->GetPort(); }

std::string SimpleServer::GetBaseUrl(Schema type) const {
  std::string url = type == Schema::kHttp ? "http://" : "https://";
  switch (pimpl_->GetProtocol()) {
    case kTcpIpV4:
      url += "127.0.0.1:";
      break;
    case kTcpIpV6:
      url += "[::1]:";
      break;
  }

  url += std::to_string(GetPort());

  return url;
}

}  // namespace testing