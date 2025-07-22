#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <boost/asio.hpp>

int main() {
    printf("Starting GPS code...\n");
    using namespace boost::asio;

    io_service io;
    serial_port serial(io);

    try {
        printf("Opening serial port...\n");
        serial.open("/dev/ttyACM0");  // Adjust if needed for your system
        printf("Serial port opened.\n");
        serial.set_option(serial_port_base::baud_rate(115200));

        std::ofstream gps_output("gps_log.txt");

        std::string line;
        char c;

        std::cout << "Reading NMEA data...\n";

        while (true) {
            read(serial, buffer(&c, 1));
            if (c == '\n') {
                if (line.find("$GNGGA") == 0) {
                    // Timestamp when line is complete
                    auto now = std::chrono::system_clock::now();
                    auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(
                                      now.time_since_epoch()).count();

                    std::stringstream ss(line);
                    std::string field;
                    std::vector<std::string> fields;

                    while (std::getline(ss, field, ',')) {
                        fields.push_back(field);
                    }

                    if (fields.size() >= 10) {
                        auto convertToDecimal = [](const std::string& nmeaCoord, const std::string& dir) {
                            if (nmeaCoord.empty()) return 0.0;
                            double raw = std::stod(nmeaCoord);
                            int degrees = static_cast<int>(raw / 100);
                            double minutes = raw - (degrees * 100);
                            double decimal = degrees + minutes / 60.0;
                            if (dir == "S" || dir == "W") decimal = -decimal;
                            return decimal;
                        };

                        double latitude = convertToDecimal(fields[2], fields[3]);
                        double longitude = convertToDecimal(fields[4], fields[5]);
                        double altitude = std::stod(fields[9]);

                        if (gps_output.is_open()) {
                            gps_output << now_us << "," << latitude << "," << longitude << "," << altitude << std::endl;
                        }

                        //FOR READABILITY ONLY!!! Remove for actual use
                        std::cout << std::fixed << std::setprecision(10);
                        std::cout << "Timestamp: " << now_us << " us, ";
                        std::cout << "Lat: " << latitude
                                  << ", Lon: " << longitude
                                  << ", Alt: " << altitude << " m\n";
                    }
                }
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
