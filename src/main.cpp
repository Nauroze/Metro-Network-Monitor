#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/beast.hpp>

#include <iostream>

using tcp = boost::asio::ip::tcp;
using namespace boost::beast;

void OnRead(const boost::system::error_code& ec, std::size_t size )
{
    std::cerr << (ec ? "OnRead Error: " : "OnRead OK")
    << (ec ? ec.message() : "")
    << std::endl;
}

void OnSend(websocket::stream<tcp_stream> &ws,
            boost::beast::flat_buffer& rBuffer,
            const boost::system::error_code& ec 
            )
{
    std::cerr << (ec ? "OnSend Error: " : "OnSend OK")
    << (ec ? ec.message() : "")
    << std::endl;

    ws.async_read(rBuffer, OnRead);
}

void OnHandshake(websocket::stream<tcp_stream> &ws,
                 const boost::asio::const_buffer& wBuffer,
                 boost::beast::flat_buffer& rBuffer,
                 const boost::system::error_code& ec )
{
        std::cerr << (ec ? "OnHandShake Error: " : "OnHandShake OK")
        << (ec ? ec.message() : "")
        << std::endl;

        ws.async_write(wBuffer, 
            [&ws, &rBuffer](auto ec, std::size_t size)
            {
                OnSend(ws, rBuffer, ec);
            }
        );
}

void OnConnect(websocket::stream<tcp_stream> &ws, 
               const std::string& url,
               const std::string& endpoint,
               const boost::asio::const_buffer& wBuffer,
               boost::beast::flat_buffer& rBuffer,
               const boost::system::error_code& ec 
            )
{
    std::cerr << (ec ? "OnConnect Error: " : "OnConnect OK")
    << (ec ? ec.message() : "")
    << std::endl;
    
    // async handshake and lambda callback
    ws.async_handshake(url, endpoint, 
        [&ws, &wBuffer, &rBuffer](auto ec)
        {
            OnHandshake(ws, wBuffer, rBuffer, ec);
        }
    );
}

int main()
{
    // I/O context object.
    boost::asio::io_context ioc {};

    // To store the error value of the eventual tcp connection
    boost::system::error_code ec {};
    tcp::resolver resolver(ioc);

    const std::string url = "ltnm.learncppthroughprojects.com";
    const std::string endpoint = "/echo";
    const std::string port = "80";
    
    // Resolve URL
    auto resolverIt = resolver.resolve(url, port, ec);

    if (ec) {
        std::cerr << "Error: " << ec.message() << std::endl;
        return -1;
    } else {
        std::cout << "OK" << std::endl;
    }
    
    // TCP I/O object
    tcp::socket socket(ioc);
    
    // message
    const char* msg = "Hello You";
    
    // create websocket
    websocket::stream<tcp_stream> ws(std::move(socket));

    // create write buffer
    boost::asio::const_buffer wBuffer(msg, strlen(msg));

    // create read buffer
    boost::beast::flat_buffer rBuffer ;

    // connect to resolved endpoint
    ws.next_layer().async_connect(*resolverIt,
                            [&url, &endpoint, &ws, &wBuffer, &rBuffer](auto ec)
                            {
                                OnConnect(ws, url, endpoint, wBuffer, rBuffer, ec);
                            }
                        );

    // Run all context work and close
    ioc.run();

    // Print the echoed message
    std::cout << boost::beast::buffers_to_string(rBuffer.data());
}