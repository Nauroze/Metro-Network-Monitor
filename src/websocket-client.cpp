#include "websocket-client.hpp"
#include <boost/beast.hpp>

using tcp = boost::asio::ip::tcp;
using namespace boost::beast;

namespace NetworkMonitor {

WebSocketClient::WebSocketClient(        
        const std::string& url,
        const std::string& endpoint,
        const std::string& port,
        boost::asio::io_context& ioc,
        boost::asio::ssl::context& ctx) 
        : m_url(url),
        m_endpoint(endpoint),
        m_port(port),
        m_closed(true),
        m_ioc(&ioc),
        m_resolver(boost::asio::make_strand(ioc)),
        m_ws(boost::asio::make_strand(ioc), ctx)
{ 
}

WebSocketClient::~WebSocketClient() = default;

void WebSocketClient::Connect(
    std::function<void (boost::system::error_code)> onConnect,
    std::function<void (boost::system::error_code,
                        std::string&&)> onMessage,
    std::function<void (boost::system::error_code)> onDisconnect)
{
    m_onConnect = onConnect;
    m_onMessage = onMessage;
    m_onDisconnect = onDisconnect;
    m_closed = false;

    auto resolverIt = m_resolver.resolve(m_url, m_port, m_ec);

    if (m_ec) 
    {
        std::cerr << "Error: " << m_ec.message() << "\n";
        return;
    } 

    m_ws.next_layer().next_layer().async_connect(*resolverIt, 
                                    [this](auto ec)
                                    {
                                        OnConnect(ec);
                                    }
    );
}

void WebSocketClient::Send(
        const std::string& message,
        std::function<void (boost::system::error_code)> onSend)
{
    m_ws.async_write(boost::asio::buffer(message), 
                    [onSend](auto ec, auto size)
                    {
                        onSend(ec);
                    }
    );
}

void WebSocketClient::Close(
    std::function<void (boost::system::error_code)> onClose
)
{
    m_closed = true;
    m_ws.async_close(
        websocket::close_code::none,
        [onClose](auto ec)
        {
            if(onClose)
            {
                onClose(ec);
            }
        }
    );
}

void WebSocketClient::OnConnect(
    const boost::system::error_code& ec
)
{
    if (ec)
        std::cerr << "OnConnect Error: " << ec.message() << "\n";

    if (ec)
    {
        if (m_onConnect)
        {
            m_onConnect(ec);
        }
    }

    m_ws.next_layer().async_handshake(boost::asio::ssl::stream_base::client,
                                      [this](auto ec)
                                      {
                                          OnTLSHandshake(ec);
                                      }
                                    ); 
}

void WebSocketClient::OnTLSHandshake(
    const boost::system::error_code& ec
)
{
    if (ec)
        std::cerr << "OnTLSHandshake Error: " << ec.message() << "\n";
    
    if (ec)
    {
        if (m_onConnect)
        {
            m_onConnect(ec);
        }
    }
    
    m_ws.async_handshake(m_url, m_endpoint,
                        [this](auto ec)
                        {
                            OnHandshake(ec);
                        }
    );
}     

void WebSocketClient::OnHandshake(
    const boost::system::error_code& ec
)
{
    if (ec)
        std::cerr << "OnHandshake Error: " << ec.message() << "\n";

    if (ec)
    {
        if (m_onConnect)
        {
            m_onConnect(ec);
        }
    }

    // All message exchanges in text
    m_ws.text(true);

    ReadMessage(ec);

    if(m_onConnect)
    {
        m_onConnect(ec);
    }
}

void WebSocketClient::OnRead(
    const boost::system::error_code& ec,
    size_t size
)
{
    if(ec)
    {
        return;
    }

    std::string message(boost::beast::buffers_to_string(m_readBuffer.data()));
    m_readBuffer.consume(size);
    if (m_onMessage)
    {
        m_onMessage(ec, std::move(message));
    }
}

void WebSocketClient::ReadMessage(
    const boost::system::error_code& ec
)
{
    if (ec == boost::asio::error::operation_aborted)
    {
        if(m_onDisconnect && !m_closed)
        {
            m_onDisconnect(ec);
        }
        return;
    }

    if (ec)
        std::cerr << "ReadMessage Error: " << ec.message() << "\n";
    
    m_ws.async_read(m_readBuffer, 
        [this](auto ec, auto size)
        {
            OnRead(ec, size);
            ReadMessage(ec);
        }
    );
}
} // namespace NetworkMonitor