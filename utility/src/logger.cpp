#include "logger.hpp"

void log(const std::string& component, const std::string& message) {
    std::cout << "[" << component << "] " << message << std::endl; 
}