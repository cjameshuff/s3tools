//    Copyright (c) 2010, Christopher James Huff
//    All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//  * Redistributions of source code must retain the above copyright
//  notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//  notice, this list of conditions and the following disclaimer in the
//  documentation and/or other materials provided with the distribution.
//  * Neither the name of the copyright holders nor the names of contributors
//  may be used to endorse or promote products derived from this software
//  without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//******************************************************************************


#include "mime_types.h"
#include <iostream>
#include <sstream>
#include <string>
#include <map>

using namespace std;

// An extremely minimal content type inference system
// TODO: add more types from http://www.iana.org/assignments/media-types/
// Or find a decent MIME type inference library.
static map<string, string> mimeTypes;
void InitMimeTypes()
{
    mimeTypes[".txt"] = "text/plain";
    mimeTypes[".pov"] = "text/plain";
    mimeTypes[".inc"] = "text/plain";
    mimeTypes[".sh"] = "text/plain";
    mimeTypes[".rb"] = "text/plain";
    mimeTypes[".erb"] = "text/plain";
    mimeTypes[".h"] = "text/plain";
    mimeTypes[".hh"] = "text/plain";
    mimeTypes[".hpp"] = "text/plain";
    mimeTypes[".H"] = "text/plain";
    mimeTypes[".cpp"] = "text/plain";
    mimeTypes[".c"] = "text/plain";
    mimeTypes[".C"] = "text/plain";
    mimeTypes[".mak"] = "text/plain";
    mimeTypes["Makefile"] = "text/plain";
    
    mimeTypes[".css"] = "text/css";
    mimeTypes[".csv"] = "text/csv";
    mimeTypes[".htm"] = "text/html";
    mimeTypes[".html"] = "text/html";
    mimeTypes[".xml"] = "text/xml";
    
    mimeTypes[".png"] = "image/png";
    mimeTypes[".PNG"] = "image/png";
    mimeTypes[".gif"] = "image/gif";
    mimeTypes[".GIF"] = "image/gif";
    mimeTypes[".jpg"] = "image/jpeg";
    mimeTypes[".JPG"] = "image/jpeg";
    mimeTypes[".jpeg"] = "image/jpeg";
    mimeTypes[".JPEG"] = "image/jpeg";
    mimeTypes[".tiff"] = "image/tiff";
    mimeTypes[".TIFF"] = "image/tiff";
    mimeTypes[".svg"] = "image/svg+xml";
    mimeTypes[".SVG"] = "image/svg+xml";
    mimeTypes[".tga"] = "image";
    mimeTypes[".TGA"] = "image";
    
    mimeTypes[".mp3"] = "audio/mp3";
    mimeTypes[".MP3"] = "audio/mp3";
    
    mimeTypes[".mp4"] = "video/mp4";//video/vnd.objectvideo
    mimeTypes[".MP4"] = "video/mp4";
    mimeTypes[".mpg"] = "video/mpeg";
    mimeTypes[".MPG"] = "video/mpeg";
    mimeTypes[".mpeg"] = "video/mpeg";
    mimeTypes[".MPEG"] = "video/mpeg";
    mimeTypes[".mov"] = "video/quicktime";
    mimeTypes[".MOV"] = "video/quicktime";
    
    mimeTypes[".tex"] = "application/x-latex";
    mimeTypes[".pdf"] = "application/pdf";
    mimeTypes[".PDF"] = "application/pdf";
    
    mimeTypes[".tar"] = "application/x-tar";
    mimeTypes[".gz"] = "application/octet-stream";
    mimeTypes[".zip"] = "application/zip";
    
    mimeTypes[".js"] = "application/js";
}

string MatchMimeType(const string & fname)
{
    std::string::size_type crsr = fname.find_last_of('.');
    if(crsr != string::npos) {// File name has extension
        string extension(fname.substr(crsr));
        if(mimeTypes.find(extension) != mimeTypes.end())
            return mimeTypes[extension];
    }
    
    // File name has no extension, or extension isn't known. Check for special file name.
    if(mimeTypes.find(fname) != mimeTypes.end())
        return mimeTypes[fname];
    
    // Could not find a match
    return string("");
}
