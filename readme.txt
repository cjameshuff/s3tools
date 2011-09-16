For questions or suggestions, please contact me at cjameshuff@gmail.com.

Requirements:
An Amazon S3 account:
http://aws.amazon.com/s3/

libcurl: http://curl.haxx.se/
libcurlpp: http://curlpp.org/
openssl: http://www.openssl.org/

If you're on Mac OS X, you already have libcurl and openssl. libcurlpp is the C++ wrapper for libcurl, and is available from: http://curlpp.org/


TODO:
sync directory with bucket
globbing for get, put, rm, ls, setacl, putmeta, genidx...
s3put: generate URL
Store file manifest in S3...listing buckets is slow.
"s3_noindex" file for genidx to exclude files from index.
"don't copy metadata" option for cp
force bucket delete...clear out contents, then delete
bucket clear (or glob support for rm)
better error handling, retries, etc.
More and better tests
More and better documentation
--version, --help
versioning support: http://docs.amazonwebservices.com/AmazonS3/latest/dev/index.html?Versioning.html

s3put: When key ends in /, append file name.

CHANGELOG:
----------------------------------------------------------------
Version 0.x:

Made s3ls output more useful for machine processing
Removed &quot;'s from eTag
Command_s3get() opens output files in binary format to avoid corruption. (need to check file usage elsewhere)

Version 0.2:
Features:
Made s3ls list buckets by default, only doing deep listing with -r option.
Added bucket aliases, local names for buckets. For example, you can define a "files" alias for a "files.domain.com" bucket.
Added more flexible object paths: "bucket object",  "bucket:object", "bucket/object"

Other:
Cleaned up some of the code.
Added some upper case file extensions to detected list. *.JPG files are recognized now.
Fixed license.

----------------------------------------------------------------
Version 0.1:
Fixed incorrect usage help call that kept s3tool from compiling.
Added version number
Added genidx command


Installation:

Compiling:
Build the program:
make

Copy the program to its destination, pick whichever you prefer, cd there, and run install:

cp s3tool ~/bin/s3tool
cd ~/bin/

or:

cp s3tool /usr/local/bin/s3tool
cd /usr/local/bin/

and then:

./s3tool install

This will set up symlinks for the various s3 tools. The tool is usable without doing this step, but the commands will have to be accessed as "s3tool ls" instead of "s3ls", "s3tool put" instead of "s3put", etc.

You will also need to set up a credentials file. This is a plain text file with at least three lines specifying a key ID, secret key, and a name identifying the credentials set. This file may also specify bucket aliases (these aliases are only seen and handled by s3tool, and do not exist on S3 itself).

For example (non-working example credentials):
keyID 0PN5J17HBGZHT7JJ3X82
secret uV3F3YluFJax1cknvbcGwgjvx4QpvB+leU8dUj2o
name johnsmith
alias tmp tmp.johnsmith.org

The file must be named .s3credentials, and may be either in the current working directory or in the user directory. If one exists in the current working directory, it will take precedence over the one in the user directory. Alternatively, you could use a credentials file located anywhere, with any name, using the -c flag.

Whatever you do, keep this file safe! The key information contained in it gives full access to the associated Amazon S3 account.


Usage:
General options:
For commands where a bucket is specified, -i will cause the index.html object to be generated or updated.
Object paths may be specified as a bucket name followed by an object key, or as a combined path string:
BUCKET_NAME OBJECT_KEY
BUCKET_NAME/OBJECT_KEY
BUCKET_NAME:OBJECT_KEY

The / and : characters are treated identically, the latter is merely allowed as a notational convenience to make it clear what the bucket name is. When a bucket name or object key can be determined from context (a move between buckets without rename, or between names within a bucket, for example), the combined path notation allows either to be omitted without ambiguity:
BUCKET_NAME:
:OBJECT_KEY


Commands:
----------------------------------------------------------------
List all buckets:
	s3ls
List contents of bucket or object from bucket:
	s3ls BUCKET_NAME
	s3ls OBJECT_PATH
alias s3ls

----------------------------------------------------------------
Upload file to S3:
	s3put OBJECT_PATH [FILE_PATH] [-pPERMISSION] [-tTYPE] [-mMETADATA]
PERMISSION: a canned ACL:
	private
	public-read
	public-read-write
	authenticated-read
TYPE: a MIME content-type
METADATA: a HTML header and data string, multiple metadata may be specified
"s3wput" can be used as a shortcut for "s3put -ppublic-read"

----------------------------------------------------------------
Get object from S3:
	s3get OBJECT_PATH [FILE_PATH]

----------------------------------------------------------------
Get object metadata:
	s3getmeta OBJECT_PATH

----------------------------------------------------------------
Move S3 object:
	s3mv SRC_OBJECT_PATH DST_OBJECT_PATH
    
    If bucket is not specified in DST_OBJECT_PATH, it is assumed to be the same
bucket as that specified in SRC_OBJECT_PATH.

----------------------------------------------------------------
Copy S3 object:
	s3cp SRC_OBJECT_PATH DST_OBJECT_PATH
    
    If bucket is not specified in DST_OBJECT_PATH, it is assumed to be the same
bucket as that specified in SRC_OBJECT_PATH.

----------------------------------------------------------------
Remove object:
	s3 rm OBJECT_PATH

----------------------------------------------------------------
Make bucket:
	s3mkbkt BUCKET_NAME

----------------------------------------------------------------
Remove bucket:
	s3rmbkt BUCKET_NAME

----------------------------------------------------------------
Set access to bucket or object with canned ACL:
	s3setbktacl BUCKET_NAME PERMISSION
	s3setacl OBJECT_PATH PERMISSION
where PERMISSION is a canned ACL:
	private
	public-read
	public-read-write
	authenticated-read

----------------------------------------------------------------
Set access to bucket or object with full ACL:
	s3setbktacl BUCKET_NAME
	s3setacl OBJECT_PATH
	With ACL definition piped to STDIN.

----------------------------------------------------------------
Get ACL for bucket or object:
	s3getacl [BUCKET_NAME] | [OBJECT_PATH]

----------------------------------------------------------------
Generate index.html for bucket. All publically-readable objects are included in
the index:
	s3genidx BUCKET_NAME
