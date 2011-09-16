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

#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include <iostream>
#include <string>
#include <map>
#include <set>
#include "multidict.h"

struct CommandLine {
    AWS_MultiDict opts;
    std::vector<std::string> words;
    
    // flags that have parameter values
    // -X value or -Xvalue, where -x is a flag from this set.
    // Flags that have parameter values must always take that value, defaults are not
    // supported.
    std::set<std::string> flagParams;
    
    bool FlagSet(const std::string & flag) const {return opts.Exists(flag);}
    
    void Parse(int argc, char * argv[])
    {
        int j = 0;
        while(j < argc)
        {
            if(argv[j][0] == '-') {
                std::string flag = std::string(argv[j], 0, 2), value;
                if(flagParams.find(flag) != flagParams.end()) {
                    // is flag with parameter
                    if(strlen(argv[j]) == 2 && argc >= j+1)
                        opts.Insert(flag, argv[++j]);// value is next string, insert and skip
                    else// value is part of string, cut out
                        opts.Insert(flag, std::string(argv[j], 2, strlen(argv[j]) - 2));
                }
                else {
                    // is plain boolean flag
                    opts.Insert(flag, "");
                }
            }
            else {
                words.push_back(argv[j]);
            }
            ++j;
        }
    }
};

//******************************************************************************
#endif // COMMANDLINE_H
