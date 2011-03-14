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

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <assert.h>

zerolog::pipe_t::pipe_t (const std::string &filename, int mode)
{
    if (mkfifo(filename.c_str (), mode) != 0) {
		if (errno != EEXIST) {
            throw std::runtime_error ("Failed to create pipe");
		}
	}

    this->fd = open(filename.c_str (), O_RDWR | O_NONBLOCK);
    assert (this->fd >= 0);

    this->filename = filename;
}

bool zerolog::pipe_t::read (void *buffer, size_t *size) 
{
    *size = ::read(this->fd, buffer, *size);

	if (*size <= 0) {
        return false;
	}
	return true;
}

int zerolog::pipe_t::get_fd ()
{
    return this->fd;
}

const char *zerolog::pipe_t::get_filename ()
{
    return this->filename.c_str ();
}

zerolog::pipe_t::~pipe_t () 
{
    close (this->fd);
}

bool zerolog::pipe_t::is_pipe (const std::string &filename)
{
    struct stat st_buf;
    if (stat (filename.c_str (), &st_buf) == -1) {
        return false;
    }
    return S_ISFIFO(st_buf.st_mode);
}