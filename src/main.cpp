#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/beast.hpp>

#include <iostream>

using tcp = boost::asio::ip::tcp;
using namespace boost::beast;
void OnConnect(boost::system::error_code ec)
{
    std::cerr << (ec ? "Error: " : "OK")
            << (ec ? ec.message() : "")
            << std::endl;
}

int main()
{
    // I/O context object.
    boost::asio::io_context ioc {};

    // To store the error value of the eventual tcp connection
    boost::system::error_code ec {};
    tcp::resolver resolver(ioc);

    // Resolve URL
    auto resolverIt = resolver.resolve("ltnm.learncppthroughprojects.com", "80", ec);

    if (ec) {
        std::cerr << "Error: " << ec.message() << std::endl;
        return -1;
    } else {
        std::cout << "OK" << std::endl;
    }
    
    // TCP I/O object
    tcp::socket socket(ioc);

    // connect to resolved endpoint
    socket.connect(*resolverIt, ec);
    
    // create websocket and perform handshake
    websocket::stream<tcp_stream> ws(std::move(socket));
    ws.handshake("ltnm.learncppthroughprojects.com", "/echo", ec);

    if (ec) {
        std::cerr << "Error: " << ec.message() << std::endl;
        return -1;
    } else {
        std::cout << "OK" << std::endl;
    }

    // Success handshake, send message
    const char* msg = "Hello You";
    boost::asio::const_buffer writemessage(msg, strlen(msg));
    ws.write(writemessage, ec);

    // Wait for reply
    boost::beast::flat_buffer readmessage;
    ws.read(readmessage);
    std::cout << "Message echoed is: " << boost::beast::make_printable(readmessage.data());
    
    // Run all context work and close
    ioc.run();
}