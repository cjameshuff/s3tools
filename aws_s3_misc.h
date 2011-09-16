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

#ifndef AWS_S3_MISC_H
#define AWS_S3_MISC_H

#include <iostream>
#include <string>
#include <openssl/md5.h>
#include <openssl/buffer.h>
#include <openssl/hmac.h>
#include <openssl/bio.h>

//******************************************************************************

std::string EncodeB64(uint8_t * data, size_t dataLen);

size_t ComputeMD5(uint8_t md5[EVP_MAX_MD_SIZE], std::istream & istrm);
std::string ComputeMD5(std::istream & istrm);

std::string GenerateSignature(const std::string & secret, const std::string & stringToSign);

// A very minimal XML parser. 
// Extract text enclosed between <tag> and </tag>, starting from crsr
// position and leaving crsr at the character index following the end tag.
// Does not handle nested <tag>...</tag> constructs, any nested tags must
// be of a different type.
bool ExtractXML(std::string & data, std::string::size_type & crsr,
                const std::string & tag, const std::string & xml);

// Same as above, but starts from beginning of xml string. Useful when order of tags is unknown.
// Scanning from the beginning every time will be inefficient for large strings and will only
// ever return the first instance of a tag, so use appropriately.
inline bool ExtractXML(std::string & data, const std::string & tag, const std::string & xml) {
    std::string::size_type crsr = 0;
    return ExtractXML(data, crsr, tag, xml);
}

std::string HTTP_Date();


std::string HumanSize(size_t size);

//******************************************************************************
#endif // AWS_S3_MISC_H
