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

#ifndef AWS_S3_ACL_H
#define AWS_S3_ACL_H

#include <iostream>
#include <string>
#include "aws_s3_misc.h"
//******************************************************************************

// A permissions grant
// Grants can be by canonical user ID, e-mail (TODO), or a group defined
// by URI (currently, Authenticated and All).
struct ACL_Grant {
    std::string granteeDisplayName;
    std::string granteeID;
    std::string granteeURI;
    std::string permission;
};

struct ACL_Perms {
    bool read;
    bool write;
    bool readACP;
    bool writeACP;
    
    ACL_Perms():
        read(false), write(false),
        readACP(false), writeACP(false)
    {}
    ACL_Perms(const std::string & str):
        read(false), write(false),
        readACP(false), writeACP(false)
    {
        // FULL_CONTROL is only supplied for convenience, it simply gives all permissions.
        if(str == "READ")
            read = true;
        else if(str == "WRITE")
            write = true;
        else if(str == "READ_ACP")
            readACP = true;
        else if(str == "WRITE_ACP")
            writeACP = true;
        else if(str == "FULL_CONTROL") {
            read = true;
            write = true;
            readACP = true;
            writeACP = true;
        }
    }
    ACL_Perms(const ACL_Perms & rhs):
        read(rhs.read), write(rhs.write),
        readACP(rhs.readACP), writeACP(rhs.writeACP)
    {}
    ACL_Perms(bool r, bool w, bool racp, bool wacp):
        read(r), write(w),
        readACP(racp), writeACP(wacp)
    {}
    
    friend std::ostream & operator<<(std::ostream & strm, const ACL_Perms & prms) {
        strm << ((prms.read)? 'r' : '-');
        strm << ' ' << ((prms.write)? 'w' : '-');
        strm << ' ' << ((prms.readACP)? "rp" : "--");
        strm << ' ' << ((prms.writeACP)? "rp" : "--");
        return strm;
    }
};

struct S3_ACL {
    // http://docs.amazonwebservices.com/AmazonS3/latest/index.html?S3_ACLs.html
    // TODO: canonical users
    // TODO: users by e-mail
//    string ownerDisplayName;
//    string ownerID;
//    list<ACL_Grant> grants;
    
    ACL_Perms all;
    ACL_Perms auth;
    
    S3_ACL(const std::string & acl);
};

S3_ACL::S3_ACL(const std::string & acl)
{
    std::string::size_type crsr = 0;
    std::string owner;
    std::string tmp;
    crsr = 0;
//    ExtractXML(owner, crsr, "Owner", acl);
    ExtractXML(tmp, crsr, "AccessControlList", acl);
    
//    ExtractXML(ownerID, "ID", owner);
//    ExtractXML(ownerDisplayName, "DisplayName", owner);
    
    crsr = 0;
    std::string grant;
    while(ExtractXML(grant, crsr, "Grant", tmp))
    {
        // XML parser isn't smart enough to parse Grantee tags, but this should work.
        ACL_Grant g;
        //ExtractXML(g.granteeID, "ID", grant);
        //ExtractXML(g.granteeDisplayName, "DisplayName", grant);
        ExtractXML(g.granteeURI, "URI", grant);
        ExtractXML(g.permission, "Permission", grant);
        //grants.push_back(g);
        
        if(g.granteeURI == "http://acs.amazonaws.com/groups/global/AllUsers")
            all = ACL_Perms(g.permission);
        else if(g.granteeURI == "http://acs.amazonaws.com/groups/global/AuthenticatedUsers")
            auth = ACL_Perms(g.permission);
    }
}


//******************************************************************************
#endif // AWS_S3_ACL_H
