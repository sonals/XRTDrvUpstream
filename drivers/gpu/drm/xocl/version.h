#ifndef _XRT_VERSION_H_
#define _XRT_VERSION_H_

static const char xrt_build_version[] = "2.2.0";

static const char xrt_build_version_branch[] = "master";

static const char xrt_build_version_hash[] = "2de0f3707ba3b3f1a853006bfd8f75a118907021";

static const char xrt_build_version_hash_date[] = "Mon, 4 Mar 2019 13:26:04 -0800";

static const char xrt_build_version_date_rfc[] = "Mon, 04 Mar 2019 19:33:17 -0800";

static const char xrt_build_version_date[] = "2019-03-04 19:33:17";

static const char xrt_modified_files[] = "";

#define XRT_DRIVER_VERSION "2.2.0,2de0f3707ba3b3f1a853006bfd8f75a118907021"

# ifdef __cplusplus
#include <iostream>
#include <string>

namespace xrt { 

class version {
 public:
  static void print(std::ostream & output)
  {
     output << "       XRT Build Version: " << xrt_build_version << std::endl;
     output << "    Build Version Branch: " << xrt_build_version_branch << std::endl;
     output << "      Build Version Hash: " << xrt_build_version_hash << std::endl;
     output << " Build Version Hash Date: " << xrt_build_version_hash_date << std::endl;
     output << "      Build Version Date: " << xrt_build_version_date_rfc << std::endl;
  
     std::string modifiedFiles(xrt_modified_files);
     if ( !modifiedFiles.empty() ) {
        const std::string& delimiters = ",";      // Our delimiter
        std::string::size_type lastPos = 0;
        int runningIndex = 1;
        while(lastPos < modifiedFiles.length() + 1) {
          if (runningIndex == 1) {
             output << "  Current Modified Files: ";
          } else {
             output << "                          ";
          }
          output << runningIndex++ << ") ";
  
          std::string::size_type pos = modifiedFiles.find_first_of(delimiters, lastPos);
  
          if (pos == std::string::npos) {
            pos = modifiedFiles.length();
          }
  
          output << modifiedFiles.substr(lastPos, pos-lastPos) << std::endl;
  
          lastPos = pos + 1;
        }
     }
  }
};
}
#endif

#endif 

