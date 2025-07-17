#include <iostream>
#include <string>
#include <boost/asio.hpp>

int main() {
    printf("This is a placeholder for the GPS test code.\n");
    using namespace boost::asio;

    io_service io;
    serial_port serial(io);

    try {
        printf("It isn't going well.\n");
        // Change this line based on your OS
        serial.open("COM5"); // e.g., "/dev/ttyUSB0" on Pi, "COM3" on Windows
        printf("I will keep trying.\n");
        serial.set_option(serial_port_base::baud_rate(115200));

        std::string line;
        char c;

        std::cout << "Reading NMEA data...\n";

        printf("It'll work eventually.\n");

        while (true) {
            read(serial, buffer(&c, 1));
            if (c == '\n') {
                std::cout << line << std::endl;
                line.clear();
            } else if (c != '\r') {
                line += c;
            }
        }

    } catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }

    return 0;
}
