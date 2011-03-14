/*
+-----------------------------------------------------------------------------------+
|  zerolog, network log daemon                                                      |
|  Copyright (c) 2011, Mikko Koppanen <mikko@kuut.io>                               |
|  All rights reserved.                                                             |
+-----------------------------------------------------------------------------------+
|  Redistribution and use in source and binary forms, with or without               |
|  modification, are permitted provided that the following conditions are met:      |
|     * Redistributions of source code must retain the above copyright              |
|       notice, this list of conditions and the following disclaimer.               |
|     * Redistributions in binary form must reproduce the above copyright           |
|       notice, this list of conditions and the following disclaimer in the         |
|       documentation and/or other materials provided with the distribution.        |
|     * Neither the name of the copyright holder nor the                            |
|       names of its contributors may be used to endorse or promote products        |
|       derived from this software without specific prior written permission.       |
+-----------------------------------------------------------------------------------+
|  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND  |
|  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED    |
|  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           |
|  DISCLAIMED. IN NO EVENT SHALL KUUTIO LIMITED BE LIABLE FOR ANY                   |
|  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES       |
|  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;     |
|  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND      |
|  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT       |
|  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS    |
|  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                     |
+-----------------------------------------------------------------------------------+
*/

#include "pipe.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <boost/filesystem.hpp>
#include <exception>
#include <stdexcept>
#include <zmq.hpp>
#include <limits.h>

#ifndef DEFAULT_DSN
# define DEFAULT_DSN "tcp://127.0.0.1:5656"
#endif

namespace fs = boost::filesystem;

int main (int argc, char *argv []) 
{
    // parse args
    bool bound = false;
    int c;

    std::vector<zerolog::pipe_t *> pipes;

    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_PUB);

    opterr = 0;
    while ((c = getopt(argc, argv, "b:d:hp:")) != -1) {
        switch (c) {

            case 'b':
                socket.bind (optarg);
                bound = true;
                break;

            case 'd':
            {
                // Iterate files in directory and find pipes
                fs::path p (optarg);

                try {
                    fs::directory_iterator dir_end;

                    for (fs::directory_iterator it (p); it != dir_end; ++it) {
                        std::string name;
                        std::stringstream ss;
                        ss << *it;
                        ss >> name;

                        if (fs::is_other (*it) && zerolog::pipe_t::is_pipe (name)) {
                            pipes.push_back (new zerolog::pipe_t (name, 0755));
                        }
                    }
                } catch (const fs::filesystem_error& e) {
                    std::cerr << "Error: " << e.what () << std::endl;
                }
            }
                break;

            case 'p':
                std::string a = optarg;
                pipes.push_back (new zerolog::pipe_t (a, 0755));
                break;
        }
    }

    if (!bound) {
        socket.bind (DEFAULT_DSN);
    }

    if (!pipes.size ()) {
        std::cerr << "Not listening on any pipes.. exiting.."
                  << std::endl;
        return 1;
    }

    zmq::pollitem_t *items = new zmq::pollitem_t [pipes.size ()];

    for (size_t i = 0; i < pipes.size (); i++) {
        items [i].socket  = 0;
        items [i].fd      = pipes[i]->get_fd ();
        items [i].events  = ZMQ_POLLIN;
        items [i].revents = 0;
    }

    while (true) {
        int rc = zmq::poll (&items [0], pipes.size (), -1);
        assert (rc >= 0);

        for (size_t i = 0; i < pipes.size (); i++) {
            if (items [i].revents & ZMQ_POLLIN) {
                size_t buffer_size = PIPE_BUF;
                unsigned char buffer [PIPE_BUF];

                if (pipes [i]->read (buffer, &buffer_size)) {
                    const char *filename = pipes [i]->get_filename ();

                    zmq::message_t msg (strlen (filename) + 1 + buffer_size);

                    memcpy ((char *) msg.data (), filename, strlen (filename));
                    memcpy ((char *) msg.data () + strlen (filename), "|", 1);
                    memcpy ((char *) msg.data () + strlen (filename) + 1, buffer, buffer_size);

                    socket.send (msg, 0);
                }
            }
            if (items [i].revents & ZMQ_POLLERR) {
                std::cerr << "Error polling " << pipes [i]->get_filename () << ": "
                          << zmq_strerror (errno) << std::endl;
            }
        }
    }
    for (size_t i = 0; i < pipes.size (); i++) {
        delete pipes [i];
    }
    delete[] items;
    return 0;
}
