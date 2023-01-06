#ifndef WEBSOCKET_CLIENT_HPP
#define WEBSOCKET_CLIENT_HPP

#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>

namespace NetworkMonitor {

/*! \brief Client to connect to a WebSocket server over TLS.
 *
 *  \tparam Resolver        The class to resolve the URL to an IP address. It
 *                          must support the same interface of
 *                          boost::asio::ip::tcp::resolver.
 *  \tparam WebSocketStream The WebSocket stream class. It must support the
 *                          same interface of boost::beast::websocket::stream.
 */
template <
    typename Resolver,
    typename WebSocketStream
>
class WebSocketClient {
public:
    /*! \brief Construct a WebSocket client.
     *
     *  \note This constructor does not initiate a connection.
     *
     *  \param url      The URL of the server.
     *  \param endpoint The endpoint on the server to connect to.
     *                  Example: ltnm.learncppthroughprojects.com/<endpoint>
     *  \param port     The port on the server.
     *  \param ioc      The io_context object. The user takes care of calling
     *                  ioc.run().
     */
    WebSocketClient(
        const std::string& url,
        const std::string& endpoint,
        const std::string& port,
        boost::asio::io_context& ioc,
        boost::asio::ssl::context& ctx
    ) : m_url {url},
        m_endpoint {endpoint},
        m_port {port},
        m_resolver {boost::asio::make_strand(ioc)},
        m_ws {boost::asio::make_strand(ioc), ctx}
    {
    }

    /*! \brief Destructor.
     */
    ~WebSocketClient() = default;

    /*! \brief Connect to the server.
     *
     *  \param onConnect     Called when the connection fails or succeeds.
     *  \param onMessage     Called only when a message is successfully
     *                       received. The message is an rvalue reference;
     *                       ownership is passed to the receiver.
     *  \param onDisconnect  Called when the connection is closed by the server
     *                       or due to a connection error.
     */
    void Connect(
        std::function<void (boost::system::error_code)> onConnect = nullptr,
        std::function<void (boost::system::error_code,
                            std::string&&)> onMessage = nullptr,
        std::function<void (boost::system::error_code)> onDisconnect = nullptr
    )
    {
        // Save the user callbacks for later use.
        m_onConnect = onConnect;
        m_onMessage = onMessage;
        m_onDisconnect = onDisconnect;

        // Start the chain of asynchronous callbacks.
        m_closed = false;
        m_resolver.async_resolve(m_url, m_port,
            [this](auto ec, auto resolverIt) {
                OnResolve(ec, resolverIt);
            }
        );
    }

    /*! \brief Send a text message to the WebSocket server.
     *
     *  \param message The message to send. The caller must ensure that this
     *                 string stays in scope until the onSend handler is called.
     *  \param onSend  Called when a message is sent successfully or if it
     *                 failed to send.
     */
    void Send(
        const std::string& message,
        std::function<void (boost::system::error_code)> onSend = nullptr
    )
    {
        m_ws.async_write(boost::asio::buffer(message),
            [onSend](auto ec, auto) {
                if (onSend) {
                    onSend(ec);
                }
            }
        );
    }

    /*! \brief Close the WebSocket connection.
     *
     *  \param onClose Called when the connection is closed, successfully or
     *                 not.
     */
    void Close(
        std::function<void (boost::system::error_code)> onClose = nullptr
    )
    {
        m_closed = true;
        m_ws.async_close(
            boost::beast::websocket::close_code::none,
            [onClose](auto ec) {
                if (onClose) {
                    onClose(ec);
                }
            }
        );
    }

    private:
    std::string m_url;
    std::string m_endpoint;
    std::string m_port;
    bool m_closed;
    boost::asio::io_context* m_ioc;
    boost::asio::ssl::context* m_ctx;
    boost::system::error_code m_ec;
    WebSocketStream m_ws;
    Resolver m_resolver;
    boost::beast::flat_buffer m_readBuffer;

    private:
    std::function<void (boost::system::error_code)> m_onConnect {nullptr};
    std::function<void (boost::system::error_code,
                        std::string&&)> m_onMessage {nullptr};
    std::function<void (boost::system::error_code)> m_onDisconnect {nullptr};

    void OnResolve(
        const boost::system::error_code& ec,
        boost::asio::ip::tcp::resolver::iterator resolverIt
    )
    {
        if (ec) {
            if (m_onConnect) {
                m_onConnect(ec);
            }
            return;
        }

        // The following timeout only matters for the purpose of connecting to
        // the TCP socket. We will reset the timeout to a sensible default
        // after we are connected.
        // Note: The TCP layer is the lowest layer (WebSocket -> TLS -> TCP).
        boost::beast::get_lowest_layer(m_ws).expires_after(
            std::chrono::seconds(5)
        );

        // Connect to the TCP socket.
        // Note: The TCP layer is the lowest layer (WebSocket -> TLS -> TCP).
        boost::beast::get_lowest_layer(m_ws).async_connect(*resolverIt,
            [this](auto ec) {
                OnConnect(ec);
            }
        );
    }

    void OnConnect(
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
    
    void OnTLSHandshake(
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
    
    void OnHandshake(
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

    void OnRead(
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

    void ReadMessage(
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
};

using BoostWebSocketClient = WebSocketClient<
    boost::asio::ip::tcp::resolver,
    boost::beast::websocket::stream<
        boost::beast::ssl_stream<boost::beast::tcp_stream>
    >
>;

} // namespace NetworkMonitor

#endif // WEBSOCKET_CLIENT_HPP