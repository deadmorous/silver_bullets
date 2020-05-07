#include "sb_os/get_program_dir.hpp"

#include <stdexcept>
#include <vector>

#include <boost/assert.hpp>
#include <boost/lexical_cast.hpp>

#ifdef _WIN32
#include <windows.h>
#else // _WIN32
#include <sys/types.h>
#include <unistd.h>
#endif // _WIN32

using namespace std;

std::string get_program_dir()
{
    unsigned int sz_buf = 256;
    const unsigned int sz_max = 16384;

#ifdef _WIN32
    string path;
    while( 1 ) {
        path.resize( sz_buf );
        unsigned int length = GetModuleFileName( 0, &path[0], sz_buf );
        if( length == 0 ) {
            auto code = boost::lexical_cast<string>(GetLastError());
            throw runtime_error(
                        string( "get_program_dir() failed: "
                                "GetModuleFileNameA() returned 0, GetLastError() returned " ) +
                        code );
        }
        if( length <= sz_buf ) {
            path.erase( path.begin()+(length-1), path.end() );
            break;
        }
        sz_buf += sz_buf;
        if( sz_buf > sz_max )
            throw runtime_error( "get_program_dir() failed: application filename is too long" );
    }
    constexpr char PathDelimiter = '\\';

#else // _WIN32
    // Obtain full pathname for the executable
    auto pid = boost::lexical_cast<string>(getpid());
    string linkname = string("/proc/") + pid + "/exe";
    vector< char > buf( sz_buf + 1 );
    while( 1 ) {
        ssize_t sz = readlink( linkname.c_str(), &buf[0], sz_buf );
        if( sz == -1 ) {
            BOOST_ASSERT( false );
            throw runtime_error( "get_program_dir() failed: readlink() returned -1" );
        }
        else if( sz >= sz_buf ) {
            if( sz_buf >= sz_max ) {
                BOOST_ASSERT( false );
                throw runtime_error( "get_program_dir() failed: link content is too long" );
            }
            sz_buf += sz_buf;
            buf.resize( sz_buf + 1 );
        }
        else {
            BOOST_ASSERT( sz > 0 );
            buf[static_cast<size_t>(sz)] = 0;
            break;
        }
    }
    string path = buf.data();
    constexpr char PathDelimiter = '/';

#endif // _WIN32
    string::size_type pos = path.find_last_of( PathDelimiter );
    BOOST_ASSERT( pos != string::npos );
    return path.substr( 0, pos+1 );
}
