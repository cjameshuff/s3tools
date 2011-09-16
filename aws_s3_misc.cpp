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


#include "aws_s3_misc.h"
#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <cmath>


using namespace std;

// Encode binary data in ASCII form using base 64
std::string EncodeB64(uint8_t * data, size_t dataLen)
{
    // http://www.ioncannon.net/programming/34/howto-base64-encode-with-cc-and-openssl/
    BIO * b64 = BIO_new(BIO_f_base64());
    BIO * bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_write(b64, data, dataLen);
    BIO_ctrl(b64, BIO_CTRL_FLUSH, 0, NULL);//BIO_flush(b64);
    
    BUF_MEM * bptr;
    BIO_get_mem_ptr(b64, &bptr);
    
    string b64string(bptr->data, bptr->length-1);
//    cout << "b64string: \"" << b64string << "\"" << endl;
    BIO_free_all(b64);
	return b64string;
}

// Compute a MD5 checksum of a given data stream as a binary string
// openssl dgst -md5 -binary FILE | openssl enc -base64
const streamsize kMD5_ChunkSize = 16384;
const char * hexchars = "0123456789abcdef";
size_t ComputeMD5(uint8_t md5[EVP_MAX_MD_SIZE], std::istream & istrm)
{
    EVP_MD_CTX ctx;
    EVP_DigestInit(&ctx, EVP_md5());
    
    uint8_t * buf = new uint8_t[kMD5_ChunkSize];
    while(istrm) {
        istrm.read((char*)buf, kMD5_ChunkSize);
        streamsize count = istrm.gcount();
        EVP_DigestUpdate(&ctx, buf, count);
    }
    delete[] buf;
    
    unsigned int mdLen;
    EVP_DigestFinal_ex(&ctx, md5, &mdLen);
    EVP_MD_CTX_cleanup(&ctx);
    return mdLen;
}

// Compute a MD5 checksum of a given data stream as a hex-encoded ASCII string
std::string ComputeMD5(std::istream & istrm)
{
    uint8_t md5[EVP_MAX_MD_SIZE];
    size_t mdLen = ComputeMD5(md5, istrm);
    std::ostringstream md5strm;
    for(size_t j = 0; j < mdLen; ++j)
        md5strm << hexchars[(md5[j] >> 4) & 0x0F] << hexchars[md5[j] & 0x0F];
    
    return md5strm.str();
}

// Generate a signature from a message and secret.
string GenerateSignature(const string & secret, const string & stringToSign)
{
    HMAC_CTX ctx;
    uint8_t md[EVP_MAX_MD_SIZE];
    unsigned int mdLength = 0;
    HMAC_CTX_init(&ctx);
    HMAC_Init(&ctx, secret.c_str(), secret.length(), EVP_sha1());
    HMAC_Update(&ctx, (uint8_t*)stringToSign.c_str(), stringToSign.length());
    HMAC_Final(&ctx, md, &mdLength);
    HMAC_CTX_cleanup(&ctx);
    return EncodeB64(md, mdLength);
}

bool ExtractXML(string & data, string::size_type & crsr, const string & tag, const string & xml)
{
    string startTag = string("<") + tag + ">";
    string endTag = string("</") + tag + ">";
    crsr = xml.find(startTag, crsr);
    if(crsr != string::npos) {
        crsr += startTag.size();
        string::size_type crsr2 = xml.find(endTag, crsr);
        data = string(xml, crsr, crsr2 - crsr);
        crsr = crsr2 + endTag.size();
        return true;
    }
    data = "";
    return false;
}

// Generates a string with the current time formatted in the way required by the HTTP protocol.
string HTTP_Date()
{
    time_t t = time(NULL);
    tm gmt;
    gmtime_r(&t, &gmt);
    char bfr[256];
    size_t n = strftime(bfr, 256, "%a, %d %b %Y %H:%M:%S GMT", &gmt);
    bfr[n] = '\0';
    return bfr;
}


string HumanSize(size_t size)
{
    std::ostringstream strm;
    if(size > 1024*1024*1024) {
        double s = (double)size/(1024*1024*1024);
        //floor(log10(s)) + 1 is number of digits
        strm.precision(floor(log10(s)) + 3);
        strm << s << " GB";
    }
    else if(size > 1024) {
        double s = (double)size/(1024*1024);
        strm.precision(floor(log10(s)) + 3);
        strm << s << " MB";
    }
    else if(size > 1024) {
        double s = (double)size/(1024);
        strm.precision(floor(log10(s)) + 3);
        strm << s << " KB";
    }
    else strm << size << " B";
    return strm.str();
}

//******************************************************************************
