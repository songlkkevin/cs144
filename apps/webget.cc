#include "socket.hh"

#include <cstdlib>
#include <iostream>
#include <span>
#include <string>

using namespace std;

void get_URL( const string& host, const string& path )
{
  // Create a TCP socket.
  TCPSocket tcps = TCPSocket();
  // Get the address tcps will connect to.
  Address addr = Address(host, "http");
  // Connect tcps to addr.
  tcps.connect(addr);
  // Send connection request.
  string req = "";
  req += "GET "   + path + " HTTP/1.1\r\n";
  req += "Host: " + host + " \r\n";
  req += "Connection: close\r\n";
  // New line to tell server don't need to wait anymore.
  req += "\r\n";
  // Use the write() function in class FileDescriptor to write request.
  tcps.write(req);
  // Keep listening messages from server until socket reaches EOF.
  string buf;
  while (!tcps.eof())
  {
    tcps.read(buf);
    cout << buf;
  }
  // Close the socket.
  tcps.close();
}

int main( int argc, char* argv[] )
{
  try {
    if ( argc <= 0 ) {
      abort(); // For sticklers: don't try to access argv[0] if argc <= 0.
    }

    auto args = span( argv, argc );

    // The program takes two command-line arguments: the hostname and "path" part of the URL.
    // Print the usage message unless there are these two arguments (plus the program name
    // itself, so arg count = 3 in total).
    if ( argc != 3 ) {
      cerr << "Usage: " << args.front() << " HOST PATH\n";
      cerr << "\tExample: " << args.front() << " stanford.edu /class/cs144\n";
      return EXIT_FAILURE;
    }

    // Get the command-line arguments.
    const string host { args[1] };
    const string path { args[2] };

    // Call the student-written function.
    get_URL( host, path );
  } catch ( const exception& e ) {
    cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
