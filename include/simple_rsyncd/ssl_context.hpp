#pragma once

#include <string>
#include <memory>

namespace simple_rsyncd {

class SSLContext {
public:
    SSLContext();
    ~SSLContext();
    
    bool initialize(const std::string& cert_file, const std::string& key_file, 
                   const std::string& ca_file = "");
    bool isInitialized() const;
    void cleanup();

private:
    bool initialized_;
    // SSL context implementation would go here
};

} // namespace simple_rsyncd
