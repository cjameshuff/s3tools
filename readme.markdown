----------------------------------------------------------------
Requirements:
----------------------------------------------------------------
An Amazon S3 account: http://aws.amazon.com/s3/

libcurl: http://curl.haxx.se/
libcurlpp: http://curlpp.org/
openssl: http://www.openssl.org/

If you're on Mac OS X, you already have libcurl and openssl. libcurlpp is the C++ wrapper for libcurl, and is available from http://curlpp.org/.

If you're on Windows, you may find the following link to be of use: http://www.gamelogicdesign.com/index.php?cID=97

----------------------------------------------------------------
Installation:
----------------------------------------------------------------

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

----------------------------------------------------------------
Usage:
----------------------------------------------------------------
General options:
For commands where a bucket is specified, -i will cause the index.html object to be generated or updated.
Object paths may be specified as a bucket name followed by an object key, or as a combined path string:

	BUCKET_NAME OBJECT_KEY
	BUCKET_NAME/OBJECT_KEY
	BUCKET_NAME:OBJECT_KEY

The / and : characters are treated identically, the latter is merely allowed as a notational convenience to make it clear what the bucket name is. When a bucket name or object key can be determined from context (a move between buckets without rename, or between names within a bucket, for example), the combined path notation allows either to be omitted without ambiguity:

	BUCKET_NAME:
	:OBJECT_KEY


----------------------------------------------------------------
Commands:
----------------------------------------------------------------
List all buckets:

	s3ls

List contents of bucket or object from bucket:

	s3ls BUCKET_NAME
	s3ls OBJECT_PATH

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

If bucket is not specified in DST\_OBJECT\_PATH, it is assumed to be the same
bucket as that specified in SRC\_OBJECT\_PATH.

----------------------------------------------------------------
Copy S3 object:

	s3cp SRC_OBJECT_PATH DST_OBJECT_PATH

If bucket is not specified in DST\_OBJECT\_PATH, it is assumed to be the same
bucket as that specified in SRC\_OBJECT\_PATH.

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
Generate index.html for bucket. All publicly-readable objects are included in
the index:

	s3genidx BUCKET_NAME


----------------------------------------------------------------
Contact
----------------------------------------------------------------
For questions or suggestions, please contact me at cjameshuff@gmail.com.

