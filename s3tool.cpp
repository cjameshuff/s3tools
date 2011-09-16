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

#include "aws_s3.h"
#include "aws_s3_misc.h"
#include "aws_s3_acl.h"
#include "mime_types.h"
#include "multidict.h"
#include "commandline.h"

#include <cmath>

#include <exception>
#include <stdexcept>

#include <set>
#include <string>

#include <unistd.h>
#include <pwd.h>

using namespace std;

// Thrown due to connection failure, etc...operation was valid, may be retried.
struct Recoverable_Error: public std::runtime_error {
	Recoverable_Error(const std::string & msg = ""): std::runtime_error(msg) {}
};

// Fatal error, can not be retried.
struct Fatal_Error: public std::runtime_error {
	Fatal_Error(const std::string & msg = ""): std::runtime_error(msg) {}
};


int Command_s3genidx(size_t wordc, CommandLine & cmds, AWS & aws);


void InitCommands();
void PrintUsage();
void LoadCredFile(const string & path, string & keyID, string & secret, string & name);

void ParseMetadata(AWS_IO & io, const CommandLine & cmdln);
void ParseObjPath(int & idx, const CommandLine & cmds, string & bucket, string & object);

void PrintObject(const AWS_S3_Object & object, bool longFormat = false);
void PrintBucket(const AWS_S3_Bucket & bucket, bool bucketName = false);

typedef int (*Command)(size_t wordc, CommandLine & cmds, AWS & aws);
static std::map<string, Command> commands;

static int verbosity = 1;
static map<string, string> aliases;


//******************************************************************************
int main(int argc, char * argv[])
{
    InitMimeTypes();
    InitCommands();
    
    CommandLine cmds;
    cmds.flagParams.insert("-v");// verbosity level
    cmds.flagParams.insert("-c");// credentials file
    cmds.flagParams.insert("-p");// permissions (canned ACL)
    cmds.flagParams.insert("-t");// type (Content-Type)
    cmds.flagParams.insert("-m");// metadata
    cmds.Parse(argc, argv);
    size_t wordc = cmds.words.size();
    
    string keyID, secret, name;
    
    if(cmds.FlagSet("-v")) {
        verbosity = cmds.opts.GetWithDefault("-v", 2);
        if(verbosity > 0)
            cout << "Verbose output level " << verbosity << endl;
    }
    
    if(cmds.FlagSet("-c")) {
        string credFile = cmds.opts.GetWithDefault("-c", "");
        LoadCredFile(credFile, keyID, secret, name);
    }
    else {
        // credentials not specified. Try standard locations
        char pwd[PATH_MAX];
        getcwd(pwd, PATH_MAX);
        string localCredFilePath(string(pwd) + "/.s3_credentials");
        
        struct passwd * ent = getpwnam(getlogin());
        string userCredFilePath(string(ent->pw_dir) + "/.s3_credentials");
        
        ifstream cred;
        
        // Try ${PWD}/.credentials first
        cred.open(localCredFilePath.c_str());
        if(cred) {
            cred.close();
            LoadCredFile(localCredFilePath, keyID, secret, name);
        }
        else {// Try ~/.s3_credentials
            cred.clear();
            cred.open(userCredFilePath.c_str());
            if(cred) {
                cred.close();
                LoadCredFile(userCredFilePath, keyID, secret, name);
            }
            else {
                cerr << "Could not open credentials file" << endl;
                return EXIT_FAILURE;
            }
        }
        
    }
    
    // Create and configure AWS instance
    AWS aws(keyID, secret);
    aws.SetVerbosity(verbosity);
    
    // Remove executable name if called directly with commands, otherwise show usage
    // If symlinked, use the executable name to determine the desired operation
    // Trim to just command name
    cmds.words[0] = cmds.words[0].substr(cmds.words[0].find_last_of('/') + 1);
    if(cmds.words[0] == "s3tool") {
        if(cmds.words.size() < 2) {
            PrintUsage();
            return EXIT_SUCCESS;
        }
        cmds.words.erase(cmds.words.begin());
        --wordc;
    }
    
    // First string in cmds.words[] is command name, following strings are parameter strings
    
    // Perform command
    int result = EXIT_SUCCESS;
    if(commands.find(cmds.words[0]) != commands.end())
    {
        try {
            result = commands[cmds.words[0]](cmds.words.size(), cmds, aws);
        }
        //catch(Recoverable_Error & err) {
        catch(std::runtime_error & err) {
            cerr << "ERROR: " << err.what() << endl;
            return EXIT_FAILURE;
        }
        
        // Regenerate index file?
        // TODO: add more checking here...or copy to all the commands that need it.
        // Currently assumes first string option is a bucket.
        if(cmds.words.size() >= 2 && cmds.FlagSet("-i")) {
            Command_s3genidx(wordc, cmds, aws);
        }
    }
    else {
        cerr << "Did not understand command \"" << cmds.words[0] << "\"" << endl;
        return EXIT_FAILURE;
    }
    
    return result;
}

//******************************************************************************
// Load file containing credentials for a given S3 account, and other account-
// specific info such as bucket aliases, etc.
// TODO: It's possible several S3 accounts will have identical bucket setups.
// Perhaps allow a separate aliases file or named credential sets?
//******************************************************************************
void LoadCredFile(const string & path, string & keyID, string & secret, string & name)
{
    ifstream cred(path.c_str());
    
    if(cred) {
//        getline(cred, keyID);
//        getline(cred, secret);
//        getline(cred, name);
        while(cred) {
            string cmd;
            cred >> cmd;
            if(cmd == "keyID")
                cred >> keyID;
            else if(cmd == "secret")
                cred >> secret;
            else if(cmd == "name")
                cred >> name;
            else if(cmd == "alias") {
                string alias, bucket;
                cred >> alias >> bucket;
                aliases[alias] = bucket;
            }
        }
        if(verbosity >= 2)
            cout << "using credentials from " << path << ", name: " << name << endl;
    }
    else {
        cerr << "Error: Could not load credentials file from " << path << "." << endl;
        exit(EXIT_FAILURE);
    }
}

//******************************************************************************
//TODO: get metadata from standard input
void ParseMetadata(AWS_IO & io, const CommandLine & cmdln)
{
    std::pair<AWS_MultiDict::const_iterator, AWS_MultiDict::const_iterator> range;
    AWS_MultiDict::const_iterator md;
    range = cmdln.opts.equal_range("-m");
    for(md = range.first; md != range.second; ++md)
    {
        string entry = md->second;
        string::size_type colon = entry.find_first_of(':');
        string::size_type valueStart = entry.find_first_not_of(' ', colon + 1);
        if(colon != string::npos) {
            string header(entry.substr(0, colon));
            string data(entry.substr(valueStart, entry.length() - valueStart));
            io.sendHeaders.Set(header, data);
            if(verbosity >= 2)
                cout << header << ": " << data << endl;
        }
        else {
            throw Fatal_Error("Bad metadata format");
        }
    }
}

//******************************************************************************
// Parse bucket name and object key from command line. The two may be sequential
// strings, the bucket name coming first, or they may be combined in a path.
// If the object key isn't specified, an empty string is returned.
// OBJECT_KEY may contain / characters, allowing imitation of a directory structure
// BUCKET_NAME OBJECT_KEY
// BUCKET_NAME/OBJECT_KEY
// BUCKET_NAME:OBJECT_KEY
//
// Omitting BUCKET_NAME from the latter two forms returns an empty string for the
// bucket name, which can be used to select a default bucket.
void ParseObjPath(int & idx, const CommandLine & cmds, string & bucket, string & object)
{
    // If first string contains a '/' or ':', treat as bucket and key. Else treat as bucket.
    string::size_type crsr = cmds.words[idx].find('/');
    if(crsr == string::npos)
        crsr = cmds.words[idx].find(':');
    
    if(crsr != string::npos)
    {
        // String is compound path, containing bucket and object key
        bucket = cmds.words[idx].substr(0, crsr);
        object = cmds.words[idx].substr(crsr + 1);
        ++idx;
    }
    else {
        // String is bucket name, next string is object key
        bucket = cmds.words[idx];
        ++idx;
        if(idx < (int)cmds.words.size()) {
            object = cmds.words[idx];
            ++idx;
        }
        else {
            object = "";
        }
    }
    
    // Transform bucket name: check for any aliases
    map<string, string>::iterator alias = aliases.find(bucket);
    if(alias != aliases.end())
        bucket = alias->second;
    
//    cout << "parsed bucket name: \"" << bucket << "\", " << bucket.size() << " B" << endl;
//    cout << "parsed object key: \"" << object << "\", " << object.size() << " B" << endl;
//    exit(-1);
}

//******************************************************************************
void PrintObject(const AWS_S3_Object & object, bool longFormat)
{
    if(longFormat)
    {
        cout << object.key << endl;
        cout << "  Last modified: " << object.lastModified << endl;
        cout << "  eTag: " << object.eTag << endl;
        cout << "  Size: " << HumanSize(object.GetSize()) << endl;
        cout << "  OwnerID: " << object.ownerID << endl;
        cout << "  OwnerName: " << object.ownerDisplayName << endl;
        cout << "  Storage class: " << object.storageClass << endl;
    }
    else
    {
        cout << object.key;
        cout << " " << object.lastModified;
        cout << ", " << HumanSize(object.GetSize());
        cout << " " << object.ownerDisplayName;
        cout << " " << object.storageClass;
        cout << " " << object.eTag;
    }
}

//******************************************************************************
void PrintBucket(const AWS_S3_Bucket & bucket, bool bucketName)
{
	if(bucketName)
		cout << bucket.name << endl;
    list<AWS_S3_Object>::const_iterator obj;
    for(obj = bucket.objects.begin(); obj != bucket.objects.end(); ++obj) {
        if(bucketName) cout << "  ";
        PrintObject(*obj);
        cout << endl;
    }
}

//******************************************************************************
// MARK: s3install
//******************************************************************************
int Command_s3install(size_t wordc, CommandLine & cmds, AWS & aws)
{
    char pwd[PATH_MAX];
    getcwd(pwd, PATH_MAX);
    std::ostringstream cmdstrm;
    cmdstrm << "ln -s " << pwd << "/s3tool s3ls";
    cmdstrm << " && ln -s " << pwd << "/s3tool s3put";
    cmdstrm << " && ln -s " << pwd << "/s3tool s3wput";
    cmdstrm << " && ln -s " << pwd << "/s3tool s3get";
    cmdstrm << " && ln -s " << pwd << "/s3tool s3getmeta";
    cmdstrm << " && ln -s " << pwd << "/s3tool s3putmeta";
    cmdstrm << " && ln -s " << pwd << "/s3tool s3mv";
    cmdstrm << " && ln -s " << pwd << "/s3tool s3cp";
    cmdstrm << " && ln -s " << pwd << "/s3tool s3rm";
    cmdstrm << " && ln -s " << pwd << "/s3tool s3mkbkt";
    cmdstrm << " && ln -s " << pwd << "/s3tool s3rmbkt";
    cmdstrm << " && ln -s " << pwd << "/s3tool s3setacl";
    cmdstrm << " && ln -s " << pwd << "/s3tool s3getacl";
    cmdstrm << " && ln -s " << pwd << "/s3tool s3genidx";
    cout << cmdstrm.str() << endl;
    return system(cmdstrm.str().c_str());
}


//******************************************************************************
// MARK: ls
//******************************************************************************
void PrintUsage_s3ls() {
    cout << "List all buckets:" << endl;
    cout << "\ts3tool ls" << endl;
    cout << "List all buckets with contents:" << endl;
    cout << "\ts3tool -r ls" << endl;
    cout << "List contents of bucket or object from bucket:" << endl;
    cout << "\ts3tool ls BUCKET_NAME [OBJECT_KEY]" << endl;
    cout << "alias s3ls" << endl;
    cout << endl;
}

int Command_s3ls(size_t wordc, CommandLine & cmds, AWS & aws)
{
    if(wordc == 1)
    {
        // List all buckets
        AWS_Connection * conn = NULL;
        list<AWS_S3_Bucket> & buckets = aws.GetBuckets(false, true, &conn);
        list<AWS_S3_Bucket>::iterator bkt;
        
        if(cmds.FlagSet("-r")) {
            for(bkt = buckets.begin(); bkt != buckets.end(); ++bkt) {
                aws.GetBucketContents(*bkt, &conn);
                PrintBucket(*bkt, true);
            }
        }
        else {
            cout << "Buckets:" << endl;
            for(bkt = buckets.begin(); bkt != buckets.end(); ++bkt)
                cout << bkt->name << endl;
        }
        delete conn;
    }
    else if(wordc == 2 || wordc == 3)
    {
        string bucketName, objectKey;
        int idx = 1;
        ParseObjPath(idx, cmds, bucketName, objectKey);
        
        if(objectKey == "")
        {
            // List specific bucket
            AWS_S3_Bucket bucket(bucketName, "");
            aws.GetBucketContents(bucket);
            PrintBucket(bucket);
        }
        else
        {
            // List specific object in bucket
            AWS_S3_Bucket bucket(bucketName, "");
            aws.GetBucketContents(bucket);
            list<AWS_S3_Object>::const_iterator obj;
            for(obj = bucket.objects.begin(); obj != bucket.objects.end(); ++obj) {
                if(obj->key == objectKey) {
                    PrintObject(*obj, true);// long format, all details
                    cout << endl;
                    break;
                }
            }
        }
    }
    else {
        PrintUsage_s3ls();
    }
    return EXIT_SUCCESS;
}


//******************************************************************************
// MARK: put
//******************************************************************************
void PrintUsage_s3put() {
    cout << "Upload file to S3:" << endl;
    cout << "\ts3tool put BUCKET_NAME OBJECT_KEY [FILE_PATH] [OPTIONS]" << endl;
    cout << "\ts3tool put BUCKET_NAME/OBJECT_KEY | BUCKET_NAME:OBJECT_KEY [FILE_PATH]" << endl;
    cout << "\tOBJECT_KEY may contain / characters, allowing imitation of a directory structure" << endl;
    cout << "[OPTIONS] = [-pPERMISSION] [-tTYPE] [-mMETADATA]" << endl;
    cout << "PERMISSION: a canned ACL:" << endl;
    cout << "\tprivate, public-read, public-read-write, or authenticated-read" << endl;
    cout << "TYPE: a MIME content-type" << endl;
    cout << "METADATA: a HTML header and data string, multiple metadata may be specified" << endl;
    cout << "\"s3wput\" can be used as a shortcut for \"s3put -ppublic-read\"" << endl;
    cout << endl;
}

int Command_s3put(size_t wordc, CommandLine & cmds, AWS & aws)
{
    if(wordc > 1) {
        AWS_IO io;
        string bucketName, objectKey;
        int idx = 1;
        ParseObjPath(idx, cmds, bucketName, objectKey);
        
        // If there's a remaining word after the bucket/key, it's a file path
        string filePath = (idx < (int)cmds.words.size())? cmds.words[idx] : objectKey;
        cout << "filePath: " << filePath << endl;
        string acl;
        
        if((cmds.words[0] == "wput") || (cmds.words[0] == "s3wput"))
            acl = "public-read";
        
        ParseMetadata(io, cmds);
        cout << "Metadata loaded" << endl;
        
        cmds.opts.Get("-p", acl);// get acl if specified
        
        if(cmds.opts.Exists("-t"))
            io.sendHeaders.Set("Content-Type", cmds.opts.GetWithDefault("-t", ""));
        else {
            string inferredType = MatchMimeType(filePath);
            if(inferredType != "")
                io.sendHeaders.Set("Content-Type", inferredType);
        }
        
        cout << "Headers set" << endl;
        
        io.ostrm = &cout;
        io.printProgress = true;
        aws.PutObject(bucketName, objectKey, acl, filePath, io);
        if(io.Failure()) {
            cerr << "ERROR: failed to put object" << endl;
            cerr << "response:\n" << io << endl;
            cerr << "response body:\n" << io.response.str() << endl;
            return EXIT_FAILURE;
        }
    }
    else {
        PrintUsage_s3put();
    }
    return EXIT_SUCCESS;
}


//******************************************************************************
// MARK: get
//******************************************************************************
void PrintUsage_s3get() {
    cout << "Download file from S3:" << endl;
    cout << "\ts3tool get BUCKET_NAME OBJECT_KEY [FILE_PATH]" << endl;
    cout << endl;
}

int Command_s3get(size_t wordc, CommandLine & cmds, AWS & aws)
{
    if(wordc > 1) {
        string bucketName, objectKey;
        int idx = 1;
        ParseObjPath(idx, cmds, bucketName, objectKey);
        
        // If there's a remaining word after the bucket/key, it's a local file/directory path
        string filePath = (idx < (int)cmds.words.size())? cmds.words[idx] : objectKey;
        ofstream fout(filePath.c_str());
        
        // TODO: if objectKey is null, get all objects in bucket, treating filePath as path prefix.
        
        AWS_IO objinfo_io;
        aws.GetObjectMData(bucketName, objectKey, objinfo_io);
        
        AWS_IO io(NULL, &fout);
        io.printProgress = (verbosity >= 1);
        io.bytesToGet = objinfo_io.headers.GetWithDefault("Content-Length", 0);
        aws.GetObject(bucketName, objectKey, io);
        if(io.Failure()) {
            cerr << "ERROR: failed to get object" << endl;
            cerr << "response:\n" << io << endl;
            // TODO: grab response body, delete incorrect local file,
            // or retry
            //cerr << "response body:\n" << io.response.str() << endl;
            return EXIT_FAILURE;
        }
    }
    else {
        PrintUsage_s3get();
    }
    return EXIT_SUCCESS;
}

//******************************************************************************
// MARK: getmeta
//******************************************************************************
void PrintUsage_s3getmeta() {
    cout << "Get object metadata:" << endl;
    cout << "\ts3tool getmeta BUCKET_NAME OBJECT_KEY" << endl;
    cout << endl;
}

int Command_s3getmeta(size_t wordc, CommandLine & cmds, AWS & aws)
{
    if(wordc == 3) {
        string bucketName, objectKey;
        int idx = 1;
        ParseObjPath(idx, cmds, bucketName, objectKey);
        
        AWS_IO io(NULL, NULL);
        aws.GetObjectMData(bucketName, objectKey, io);
        AWS_MultiDict::iterator i;
        for(i = io.headers.begin(); i != io.headers.end(); ++i)
            cout << i->first << ": " << i->second << endl;
    }
    else {
        PrintUsage_s3getmeta();
    }
    return EXIT_SUCCESS;
}

//******************************************************************************
// MARK: putmeta
//******************************************************************************
void PrintUsage_s3putmeta() {
    cout << "Set object metadata:" << endl;
    cout << "\ts3tool putmeta BUCKET_NAME OBJECT_KEY -tTYPE -mMETA..." << endl;
    cout << endl;
}

int Command_s3putmeta(size_t wordc, CommandLine & cmds, AWS & aws)
{
    // putmeta: copy to temp, delete, copy to original with overrides
    // TODO: this is a long and complex sequence of operations for what it does. S3 may
    // by now support a more direct method.
    // TODO: read metadata and only override specified items
    if(wordc == 3)
    {
        string bucketName, objectKey;
        int idx = 1;
        ParseObjPath(idx, cmds, bucketName, objectKey);
        string tmpObjectKey = objectKey + "_putmetatmp";
        AWS_IO io;
        
        // Make copy to temp object, discarding metadata
        aws.CopyObject(bucketName, objectKey, bucketName, tmpObjectKey, false, io);
        if(io.Failure()) {
            cerr << "ERROR: putmeta: failed to make temp copy of object" << endl;
            cerr << "response:\n" << io << endl;
            cerr << "response body:\n" << io.response.str() << endl;
            return EXIT_FAILURE;
        }
        
        // Get ACL of original
        io.Reset();
        string acl = aws.GetACL(bucketName, objectKey, io);
        if(io.Failure()) {
            cerr << "ERROR: putmeta: failed to get ACL." << endl;
            cerr << "response:\n" << io << endl;
            cerr << "response body:\n" << io.response.str() << endl;
            return EXIT_FAILURE;
        }
        
        // Delete original
        io.Reset();
        aws.DeleteObject(bucketName, objectKey, io);
        if(io.Failure()) {
            cerr << "ERROR: putmeta: failed to delete original copy of object" << endl;
            cerr << "response:\n" << io << endl;
            cerr << "response body:\n" << io.response.str() << endl;
            return EXIT_FAILURE;
        }
        
        // Copy temp object back to original, setting metadata
        io.Reset();
        if(cmds.FlagSet("-t"))
            io.sendHeaders.Set("Content-Type", cmds.opts.GetWithDefault("-t", ""));
        ParseMetadata(io, cmds);
        aws.CopyObject(bucketName, tmpObjectKey, bucketName, objectKey, false, io);
        if(io.Failure()) {
            cerr << "ERROR: putmeta: failed to make new copy of object" << endl;
            cerr << "response:\n" << io << endl;
            cerr << "response body:\n" << io.response.str() << endl;
            return EXIT_FAILURE;
        }
        
        // Restore ACL of object
        io.Reset();
        aws.SetACL(bucketName, objectKey, acl, io);
        if(io.Failure()) {
            cerr << "ERROR: putmeta: failed to set ACL" << endl;
            cerr << "response:\n" << io << endl;
            cerr << "response body:\n" << io.response.str() << endl;
            return EXIT_FAILURE;
        }
        
        // Delete copy
        io.Reset();
        aws.DeleteObject(bucketName, tmpObjectKey, io);
        if(io.Failure()) {
            cerr << "ERROR: putmeta: failed to delete temporary copy of object" << endl;
            cerr << "response:\n" << io << endl;
            cerr << "response body:\n" << io.response.str() << endl;
            return EXIT_FAILURE;
        }
    }
    else {
        PrintUsage_s3putmeta();
    }
    return EXIT_SUCCESS;
}

//******************************************************************************
// MARK: cp
//******************************************************************************
void PrintUsage_s3cp() {
    cout << "Copy S3 object:" << endl;
    cout << "\ts3tool cp SRC_BUCKET_NAME SRC_OBJECT_KEY DST_OBJECT_KEY" << endl;
    cout << "\ts3tool cp SRC_BUCKET_NAME SRC_OBJECT_KEY DST_BUCKET_NAME DST_OBJECT_KEY" << endl;
    cout << endl;
}

int Command_s3cp(size_t wordc, CommandLine & cmds, AWS & aws)
{
    if(wordc > 1) {
        string srcBucketName, srcObjectKey;
        string dstBucketName, dstObjectKey;
        int idx = 1;
        ParseObjPath(idx, cmds, srcBucketName, srcObjectKey);
        ParseObjPath(idx, cmds, dstBucketName, dstObjectKey);
        if(dstBucketName == "")
            dstBucketName = srcBucketName;
        if(dstObjectKey == "")
            dstObjectKey = srcObjectKey;
        
        AWS_IO io;
        
        bool copyMD = true;
        if(cmds.opts.Exists("-t")) {
            io.sendHeaders.Set("Content-Type", cmds.opts.GetWithDefault("-t", ""));
            copyMD = false;
        }
        ParseMetadata(io, cmds);
        aws.CopyObject(srcBucketName, srcObjectKey, dstBucketName, dstObjectKey, copyMD, io);
        if(io.Failure()) {
            cerr << "ERROR: failed to copy object" << endl;
            cerr << "response:\n" << io << endl;
            cerr << "response body:\n" << io.response.str() << endl;
            return EXIT_FAILURE;
        }
        
        io.Reset();
        string acl = aws.GetACL(srcBucketName, srcObjectKey, io);
        if(io.Failure()) {
            cerr << "ERROR: failed to get ACL. Object was copied, and has default ACL." << endl;
            cerr << "response:\n" << io << endl;
            cerr << "response body:\n" << io.response.str() << endl;
            return EXIT_FAILURE;
        }
        
        io.Reset();
        aws.SetACL(dstBucketName, dstObjectKey, acl, io);
        if(io.Failure()) {
            cerr << "ERROR: failed to set ACL" << endl;
            cerr << "response:\n" << io << endl;
            cerr << "response body:\n" << io.response.str() << endl;
            return EXIT_FAILURE;
        }
    }
    else {
        PrintUsage_s3cp();
    }
    return EXIT_SUCCESS;
}

//******************************************************************************
// MARK: mv
//******************************************************************************
void PrintUsage_s3mv() {
    cout << "Move S3 object:" << endl;
    cout << "\ts3tool mv SRC_BUCKET_NAME SRC_OBJECT_KEY DST_OBJECT_KEY" << endl;
    cout << "\ts3tool mv SRC_BUCKET_NAME SRC_OBJECT_KEY DST_BUCKET_NAME DST_OBJECT_KEY" << endl;
    cout << endl;
}

int Command_s3mv(size_t wordc, CommandLine & cmds, AWS & aws)
{
    if(wordc > 1) {
        string srcBucketName, srcObjectKey;
        string dstBucketName, dstObjectKey;
        int idx = 1;
        ParseObjPath(idx, cmds, srcBucketName, srcObjectKey);
        ParseObjPath(idx, cmds, dstBucketName, dstObjectKey);
        if(dstBucketName == "")
            dstBucketName = srcBucketName;
        if(dstObjectKey == "")
            dstObjectKey = srcObjectKey;
        
        AWS_IO io;
        
        bool copyMD = true;
        if(cmds.FlagSet("-t")) {
            io.sendHeaders.Set("Content-Type", cmds.opts.GetWithDefault("-t", ""));
            copyMD = false;
        }
        ParseMetadata(io, cmds);
        aws.CopyObject(srcBucketName, srcObjectKey, dstBucketName, dstObjectKey, copyMD, io);
        if(io.Failure()) {
            cerr << "ERROR: mv: failed to copy object" << endl;
            cerr << "response:\n" << io << endl;
            cerr << "response body:\n" << io.response.str() << endl;
            return EXIT_FAILURE;
        }
        
        io.Reset();
        string acl = aws.GetACL(srcBucketName, srcObjectKey, io);
        if(io.Failure()) {
            cerr << "ERROR: s3mv: failed to get ACL. Object was copied, and has default ACL." << endl;
            cerr << "response:\n" << io << endl;
            cerr << "response body:\n" << io.response.str() << endl;
            return EXIT_FAILURE;
        }
        
        io.Reset();
        aws.SetACL(dstBucketName, dstObjectKey, acl, io);
        if(io.Failure()) {
            cerr << "ERROR: s3mv: failed to set ACL" << endl;
            cerr << "response:\n" << io << endl;
            cerr << "response body:\n" << io.response.str() << endl;
            return EXIT_FAILURE;
        }
        
        io.Reset();
        aws.DeleteObject(srcBucketName, srcObjectKey, io);
        if(io.Failure()) {
            cerr << "ERROR: s3mv: failed to delete old copy of object" << endl;
            cerr << "response:\n" << io << endl;
            cerr << "response body:\n" << io.response.str() << endl;
            return EXIT_FAILURE;
        }
    }
    else {
        PrintUsage_s3mv();
    }
    return EXIT_SUCCESS;
}

//******************************************************************************
// MARK: rm
//******************************************************************************
void PrintUsage_s3rm() {
    cout << "Remove object:" << endl;
    cout << "\ts3 rm BUCKET_NAME OBJECT_KEY" << endl;
    cout << endl;
}

int Command_s3rm(size_t wordc, CommandLine & cmds, AWS & aws)
{
    if(wordc > 1) {
        string bucketName, objectKey;
        int idx = 1;
        ParseObjPath(idx, cmds, bucketName, objectKey);
        // TODO: rm bucket when only bucket specified?
        AWS_IO io;
        aws.DeleteObject(bucketName, objectKey, io);
        if(io.Failure()) {
            cerr << "ERROR: s3rm: failed to delete object\n" << io << endl;
            cerr << "response:\n" << io << endl;
            cerr << "response body:\n" << io.response.str() << endl;
            return EXIT_FAILURE;
        }
    }
    else {
        PrintUsage_s3rm();
    }
    return EXIT_SUCCESS;
}

//******************************************************************************
// MARK: mkbkt
//******************************************************************************
void PrintUsage_s3mkbkt() {
    cout << "Create bucket:" << endl;
    cout << "\ts3tool mkbkt BUCKET_NAME" << endl;
    cout << endl;
}

int Command_s3mkbkt(size_t wordc, CommandLine & cmds, AWS & aws)
{
    if(wordc == 2) {
        string bucketName = cmds.words[1];
        AWS_IO io;
        ParseMetadata(io, cmds);
        aws.CreateBucket(bucketName, io);
        if(io.Failure()) {
            cerr << "ERROR: failed to create bucket" << endl;
            cerr << "response:\n" << io << endl;
            cerr << "response body:\n" << io.response.str() << endl;
            return EXIT_FAILURE;
        }
    }
    else {
        PrintUsage_s3mkbkt();
    }
    return EXIT_SUCCESS;
}

//******************************************************************************
// MARK: rmbkt
//******************************************************************************
void PrintUsage_s3rmbkt() {
    cout << "Remove bucket:" << endl;
    cout << "\ts3tool rmbkt BUCKET_NAME" << endl;
    cout << endl;
}

int Command_s3rmbkt(size_t wordc, CommandLine & cmds, AWS & aws)
{
    if(wordc == 2) {
        string bucketName = cmds.words[1];
        AWS_IO io;
        aws.DeleteBucket(bucketName, io);
        if(io.Failure()) {
            cerr << "ERROR: failed to delete object" << endl;
            cerr << "response:\n" << io << endl;
            cerr << "response body:\n" << io.response.str() << endl;
            return EXIT_FAILURE;
        }
    }
    else {
        PrintUsage_s3rmbkt();
    }
    return EXIT_SUCCESS;
}

//******************************************************************************
// MARK: setacl, setbktacl
//******************************************************************************
void PrintUsage_s3setacl() {
    // Set access to bucket or object
    cout << "Set access to bucket or object with canned ACL:" << endl;
    cout << "\ttool setbktacl BUCKET_NAME PERMISSION" << endl;
    cout << "\ttool setacl BUCKET_NAME OBJECT_KEY PERMISSION" << endl;
    cout << "where PERMISSION is a canned ACL:\n"
         << "\tprivate, public-read, public-read-write, or authenticated-read" << endl;
    cout << "Set access to bucket or object with full ACL:" << endl;
    cout << "\ttool setbktacl BUCKET_NAME" << endl;
    cout << "\ttool setacl BUCKET_NAME OBJECT_KEY\n"
         << "\tWith ACL definition piped to STDIN." << endl;
    cout << endl;
}

int Command_s3setbktacl(size_t wordc, CommandLine & cmds, AWS & aws)
{
    // TODO: Get rid of this. setbktacl should be special case of s3setacl when objectKey is "".
    if(wordc == 2) {// set full acl (via stdin) on bucket
        string bucketName = cmds.words[1];
        istreambuf_iterator<char> begin(cin), end;
        string acl(begin, end);
        AWS_IO io;
        aws.SetACL(bucketName, acl, io);
        if(io.Failure()) {
            cerr << "ERROR: failed to set bucket ACL" << endl;
            cerr << "response:\n" << io << endl;
            cerr << "response body:\n" << io.response.str() << endl;
            return EXIT_FAILURE;
        }
    }
    else if(wordc == 3) {// set canned acl on bucket
        string bucketName = cmds.words[1];
        string acl = cmds.words[2];
        AWS_IO io;
        aws.SetCannedACL(bucketName, acl, io);
        if(io.Failure()) {
            cerr << "ERROR: failed to set bucket ACL" << endl;
            cerr << "response:\n" << io << endl;
            cerr << "response body:\n" << io.response.str() << endl;
            return EXIT_FAILURE;
        }
    }
    else {
        PrintUsage_s3setacl();
    }
    return EXIT_SUCCESS;
}

int Command_s3setacl(size_t wordc, CommandLine & cmds, AWS & aws)
{
    if(wordc > 1)
    {
        string bucketName, objectKey;
        int idx = 1;
        ParseObjPath(idx, cmds, bucketName, objectKey);
        
        // If there's a remaining word after the bucket/key, it's a canned ACL
        string acl;
        if(idx < (int)cmds.words.size()) {
            acl = cmds.words[idx];// set canned acl on object
        }
        else {// set full acl (via stdin) on object
            istreambuf_iterator<char> begin(cin), end;
            acl = string(begin, end);
        }
        
        AWS_IO io;
        aws.SetCannedACL(bucketName, objectKey, acl, io);
        if(io.Failure()) {
            cerr << "ERROR: failed to set object ACL" << endl;
            cerr << "response:\n" << io << endl;
            cerr << "response body:\n" << io.response.str() << endl;
            return EXIT_FAILURE;
        }
    }
    else {
        PrintUsage_s3setacl();
    }
    return EXIT_SUCCESS;
}

//******************************************************************************
// MARK: getacl
//******************************************************************************
void PrintUsage_s3getacl() {
    cout << "Get ACL for bucket or object:" << endl;
    cout << "\ttool getacl BUCKET_NAME [OBJECT_KEY]" << endl;
}

int Command_s3getacl(size_t wordc, CommandLine & cmds, AWS & aws)
{
    if(wordc > 1)
    {
        string bucketName, objectKey;
        int idx = 1;
        ParseObjPath(idx, cmds, bucketName, objectKey);
        
        AWS_IO io;
        if(objectKey == "")
            cout << aws.GetACL(bucketName, io) << endl;
        else
            cout << aws.GetACL(bucketName, objectKey, io) << endl;
    }
    else PrintUsage_s3getacl();
    return EXIT_SUCCESS;
}

//******************************************************************************
// MARK: genidx
//******************************************************************************

void PrintUsage_s3genidx() {
    cout << "Generate index for public-readable items in bucket:" << endl;
    cout << "\ttool genidx BUCKET_NAME" << endl;
}

int Command_s3genidx(size_t wordc, CommandLine & cmds, AWS & aws)
{
    if(wordc == 0)
    {
        PrintUsage_s3genidx();
        return EXIT_SUCCESS;
    }
    string bucketName, objectKey;
    int idx = 1;
    ParseObjPath(idx, cmds, bucketName, objectKey);
    AWS_IO io;
    
    AWS_S3_Bucket bucket(bucketName, "");
    aws.GetBucketContents(bucket);
    cout << "Generating index for bucket:" << bucket.name << endl;
    std::ostringstream strm;
    strm << "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n";
    strm << "<html>\n";
    strm << " <head>\n";
    strm << "  <title>Index of " << bucket.name << "</title>\n";
    strm << " </head>\n";
    strm << " <body>\n";
    strm << "<h1>Index of " << bucket.name << "</h1>\n";
    
    strm << "<table>\n";
    strm << "<tr><th>Name</th><th>Last modified</th><th>Size</th><th>eTag</th></tr>\n";
    strm << "<tr><th colspan=\"4\"><hr></th></tr>\n";
    list<AWS_S3_Object>::const_iterator obj;
    for(obj = bucket.objects.begin(); obj != bucket.objects.end(); ++obj)
    {
        if(obj->key == "index.html")
            continue;
        
        S3_ACL perms(aws.GetACL(bucket.name, obj->key, io));
        if(!perms.all.read) {
            cout << bucket.name << ":" << obj->key << " is not publically readable" << endl;
            continue;
        }
        //cout << bucket.name << ":" << obj->key << endl;
        //cout << "\tAll " << perms.all << endl;
        //cout << "\tAuth " << perms.auth << endl;
        strm << "<tr>";
        strm << "<td><a href=\"http://" << bucket.name << "/" << obj->key << "\">"
             << obj->key << "</a></td>";
        strm << "<td>" << obj->lastModified << "</td>";
        strm << "<td>" << HumanSize(obj->GetSize()) << "</td>";
        strm << "<td>" << obj->eTag << "</td>";
        strm << "</tr>\n";
    }
    strm << "</table>\n";
    strm << "</body>\n";
    strm << "</html>" << endl;
    
    // Upload index
    //cout << strm.str() << endl;
    std::istringstream istrm(strm.str());
    io.ostrm = &cout;
    io.istrm = &istrm;
    io.printProgress = true;
    io.sendHeaders.Set("Content-Type", "text/html");
    aws.PutObject(bucketName, "index.html", "public-read", io);
    if(io.Failure()) {
        cerr << "ERROR: failed to put index object" << endl;
        cerr << "response:\n" << io << endl;
        cerr << "response body:\n" << io.response.str() << endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

//******************************************************************************
// MARK: md5
//******************************************************************************
int Command_s3md5(size_t wordc, CommandLine & cmds, AWS & aws)
{
    ifstream fin(cmds.words[1].c_str(), ios_base::binary | ios_base::in);
    uint8_t md5[EVP_MAX_MD_SIZE];
    size_t mdLen = ComputeMD5(md5, fin);
    cout << "md5: \"" << EncodeB64(md5, mdLen) << "\"" << endl;
    return EXIT_SUCCESS;
}

//******************************************************************************
// MARK: mime
//******************************************************************************
int Command_s3mime(size_t wordc, CommandLine & cmds, AWS & aws)
{
    cout << "Content-Type: \"" << MatchMimeType(cmds.words[1]) << "\"" << endl;
    return EXIT_SUCCESS;
}

//******************************************************************************
void InitCommands()
{
    commands["install"] = Command_s3install;
    
    commands["s3ls"] = Command_s3ls;
    commands["ls"] = Command_s3ls;
    
    commands["s3wput"] = Command_s3put;
    commands["s3put"] = Command_s3put;
    commands["wput"] = Command_s3put;
    commands["put"] = Command_s3put;
    
    commands["s3get"] = Command_s3get;
    commands["get"] = Command_s3get;
    
    commands["s3getmeta"] = Command_s3getmeta;
    commands["getmeta"] = Command_s3getmeta;
    
    commands["s3putmeta"] = Command_s3putmeta;
    commands["putmeta"] = Command_s3putmeta;
    
    commands["s3cp"] = Command_s3cp;
    commands["cp"] = Command_s3cp;
    commands["s3mv"] = Command_s3mv;
    commands["mv"] = Command_s3mv;
    commands["s3rm"] = Command_s3rm;
    commands["rm"] = Command_s3rm;
    
    commands["s3mkbkt"] = Command_s3mkbkt;
    commands["mkbkt"] = Command_s3mkbkt;
    
    commands["s3rmbkt"] = Command_s3rmbkt;
    commands["rmbkt"] = Command_s3rmbkt;
    
    commands["s3setbktacl"] = Command_s3setbktacl;
    commands["setbktacl"] = Command_s3setbktacl;
    
    commands["s3setacl"] = Command_s3setacl;
    commands["setacl"] = Command_s3setacl;
    
    commands["s3getacl"] = Command_s3getacl;
    commands["getacl"] = Command_s3getacl;
    
    commands["s3genidx"] = Command_s3genidx;
    commands["genidx"] = Command_s3genidx;
    
    commands["md5"] = Command_s3md5;
    commands["mime"] = Command_s3mime;
}

void PrintUsage() {
    cout << "Usage:" << endl;
    PrintUsage_s3ls();
    PrintUsage_s3put();
    PrintUsage_s3get();
    PrintUsage_s3getmeta();
    PrintUsage_s3putmeta();
    PrintUsage_s3mv();
    PrintUsage_s3cp();
    PrintUsage_s3rm();
    PrintUsage_s3mkbkt();
    PrintUsage_s3rmbkt();
    PrintUsage_s3setacl();
    PrintUsage_s3getacl();
    PrintUsage_s3genidx();
}
