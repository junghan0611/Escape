/**
 * $Id$
 * Copyright (C) 2008 - 2014 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <sys/common.h>
#include <sys/io.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/test.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_fs(void);
static void test_basics(void);
static void test_perms(void);
static void test_rename(void);
static void test_largeFile(void);
static void test_symlinks(void);
static void test_assertCan(const char *path,uint mode);
static void test_assertCanNot(const char *path,uint mode,int err);
static void fs_createFile(const char *name,const char *content);
static void fs_readFile(const char *name,const char *content);

sTestModule tModFs = {
	"FS",
	&test_fs
};

static void test_fs(void) {
	struct stat info;
	if(stat("/bin",&info) < 0)
		error("Unable to stat /bin");

	if(getuid() != ROOT_UID) {
		printf("You are not root; skipping the test\n\n");
		return;
	}

	/* don't try that on readonly-filesystems */
	if(!(info.st_mode & S_IWUSR)) {
		printf("WARNING: Detected readonly filesystem; skipping the test\n\n");
		return;
	}

	test_basics();
	test_perms();
	test_rename();
	test_largeFile();
	test_symlinks();
}

static void test_basics(void) {
	struct stat info1;
	struct stat info2;
	test_caseStart("Testing fs");

	test_assertInt(mkdir("/newdir",DIR_DEF_MODE),0);

	fs_createFile("/newdir/file1","foobar");
	fs_readFile("/newdir/file1","foobar");
	test_assertInt(link("/newdir/file1","/newdir/file2"),0);
	test_assertInt(stat("/newdir/file1",&info1),0);
	test_assertInt(stat("/newdir/file2",&info2),0);

	// compare elements individually, because the structs might contain uninitialized padding
	test_assertUInt(info1.st_atime,info2.st_atime);
	test_assertUInt(info1.st_mtime,info2.st_mtime);
	test_assertUInt(info1.st_ctime,info2.st_ctime);
	test_assertUInt(info1.st_blocks,info2.st_blocks);
	test_assertUInt(info1.st_blksize,info2.st_blksize);
	test_assertUInt(info1.st_dev,info2.st_dev);
	test_assertUInt(info1.st_uid,info2.st_uid);
	test_assertUInt(info1.st_gid,info2.st_gid);
	test_assertUInt(info1.st_ino,info2.st_ino);
	test_assertUInt(info1.st_nlink,info2.st_nlink);
	test_assertUInt(info1.st_mode,info2.st_mode);
	test_assertUInt(info1.st_size,info2.st_size);

	test_assertUInt(info1.st_nlink,2);
	test_assertInt(unlink("/newdir/file1"),0);
	test_assertInt(rmdir("/newdir"),-ENOTEMPTY);
	test_assertInt(stat("/newdir/file1",&info1),-ENOENT);
	test_assertInt(stat("/newdir/file2",&info2),0);
	test_assertUInt(info2.st_nlink,1);
	test_assertInt(unlink("/newdir/file2"),0);

	test_assertInt(rmdir("/newdir"),0);
	int fd = open("/",O_RDONLY);
	test_assertTrue(fd >= 0);
	test_assertInt(syncfs(fd),0);
	close(fd);

	test_caseSucceeded();
}

#define RUN_IN_CHILD(code)							\
	if(fork() == 0) {								\
		code										\
		exit(0);									\
	}												\
	else											\
		test_assertInt(waitchild(NULL,-1,0),0);

static void test_perms(void) {
	size_t i;
	struct stat info;
	struct {
		const char *dir;
		const char *file;
	} paths[] = {
		{"/newfile","/newfile/test"},
		{"/sys/newfile","/sys/newfile/test"}
	};
	test_caseStart("Testing permissions");

	for(i = 0; i < ARRAY_SIZE(paths); i++) {
		/* create new file */
		fs_createFile(paths[i].dir,"foobar");
		test_assertInt(chmod(paths[i].dir,0600),0);
		test_assertInt(chown(paths[i].dir,1,1),0);

		/* I'm the owner */
		RUN_IN_CHILD(
			test_assertInt(setgid(1),0);
			test_assertInt(setuid(1),0);

			test_assertCan(paths[i].dir,O_READ);
			test_assertCan(paths[i].dir,O_WRITE);
		);

		/* I'm NOT the owner */
		RUN_IN_CHILD(
			test_assertInt(setgid(1),0);
			test_assertInt(setuid(2),0);

			test_assertCanNot(paths[i].dir,O_READ,-EACCES);
			test_assertCanNot(paths[i].dir,O_WRITE,-EACCES);
		);

		/* give group read-perm */
		test_assertInt(chmod(paths[i].dir,0640),0);

		RUN_IN_CHILD(
			test_assertInt(setgid(1),0);
			test_assertInt(setuid(2),0);

			test_assertCan(paths[i].dir,O_READ);
			test_assertCanNot(paths[i].dir,O_WRITE,-EACCES);
		);

		/* neither owner nor group */
		RUN_IN_CHILD(
			test_assertInt(setgid(2),0);
			test_assertInt(setuid(2),0);

			test_assertCanNot(paths[i].dir,O_READ,-EACCES);
			test_assertCanNot(paths[i].dir,O_WRITE,-EACCES);
		);

		/* give others read+write perm */
		test_assertInt(chmod(paths[i].dir,0646),0);

		RUN_IN_CHILD(
			test_assertInt(setgid(2),0);
			test_assertInt(setuid(2),0);

			test_assertCan(paths[i].dir,O_READ);
			test_assertCan(paths[i].dir,O_WRITE);
		);

		/* delete it */
		test_assertInt(unlink(paths[i].dir),0);


		/* create new folder */
		test_assertInt(mkdir(paths[i].dir,DIR_DEF_MODE),0);
		test_assertInt(chmod(paths[i].dir,0700),0);
		test_assertInt(chown(paths[i].dir,1,1),0);

		/* I'm the owner */
		RUN_IN_CHILD(
			test_assertInt(setgid(1),0);
			test_assertInt(setuid(1),0);

			test_assertCan(paths[i].dir,O_READ);
			test_assertCan(paths[i].dir,O_WRITE);
			fs_createFile(paths[i].file,"foo");
			test_assertInt(stat(paths[i].file,&info),0);
		);

		/* I'm NOT the owner */
		RUN_IN_CHILD(
			test_assertInt(setgid(1),0);
			test_assertInt(setuid(2),0);

			test_assertCanNot(paths[i].dir,O_READ,-EACCES);
			test_assertCanNot(paths[i].dir,O_WRITE,-EACCES);
			test_assertInt(stat(paths[i].file,&info),-EACCES);
		);

		/* give group read-perm */
		test_assertInt(chmod(paths[i].dir,0740),0);

		RUN_IN_CHILD(
			test_assertInt(setgid(1),0);
			test_assertInt(setuid(2),0);

			test_assertCan(paths[i].dir,O_READ);
			test_assertCanNot(paths[i].dir,O_WRITE,-EACCES);
			test_assertInt(stat(paths[i].file,&info),-EACCES);
		);

		/* neither owner nor group */
		RUN_IN_CHILD(
			test_assertInt(setgid(2),0);
			test_assertInt(setuid(2),0);

			test_assertCanNot(paths[i].dir,O_READ,-EACCES);
			test_assertCanNot(paths[i].dir,O_WRITE,-EACCES);
			test_assertInt(stat(paths[i].file,&info),-EACCES);
		);

		/* give others read+write perm */
		test_assertInt(chmod(paths[i].dir,0747),0);

		RUN_IN_CHILD(
			test_assertInt(setgid(2),0);
			test_assertInt(setuid(2),0);

			test_assertCan(paths[i].dir,O_READ);
			test_assertCan(paths[i].dir,O_WRITE);
			test_assertInt(stat(paths[i].file,&info),0);
		);

		/* delete it */
		test_assertInt(unlink(paths[i].file),0);
		test_assertInt(rmdir(paths[i].dir),0);
	}

	test_caseSucceeded();
}

static void test_rename(void) {
	test_caseStart("Testing rename()");

	fs_createFile("/newfile","test!");
	test_assertCan("/newfile",O_READ);
	test_assertInt(rename("/newfile","/newerfile"),0);
	test_assertCanNot("/newfile",O_READ,-ENOENT);
	test_assertInt(unlink("/newerfile"),0);
	test_assertCanNot("/newerfile",O_READ,-ENOENT);

	test_caseSucceeded();
}

static void test_largeFile(void) {
	/* ensure that the blocksize is not a multiple of this array size */
	uint8_t pattern[62];
	uint8_t buf[62];
	/* reach some double indirect blocks */
	const size_t size = 12 * 1024 + 256 * 1024 + 256 * 1024;
	test_caseStart("Creating a large file and reading it back");

	for(size_t i = 0; i < ARRAY_SIZE(pattern); ++i)
		pattern[i] = i;

	/* write it */
	{
		FILE *f = fopen("/largefile","w");
		test_assertTrue(f != NULL);

		size_t rem = size;
		while(rem > 0) {
			size_t amount = MIN(rem,ARRAY_SIZE(pattern));
			test_assertSize(fwrite(pattern,1,amount,f),amount);
			rem -= amount;
		}

		/* flush buffer cache */
		test_assertInt(syncfs(fileno(f)),0);

		fclose(f);
	}

	/* read it back */
	{
		FILE *f = fopen("/largefile","r");
		test_assertTrue(f != NULL);

		size_t rem = size;
		while(rem > 0) {
			size_t amount = MIN(rem,ARRAY_SIZE(pattern));
			test_assertSize(fread(buf,1,amount,f),amount);
			for(size_t i = 0; i < amount; ++i)
				test_assertInt(buf[i],pattern[i]);
			rem -= amount;
		}
		fclose(f);
	}

	test_assertInt(unlink("/largefile"),0);

	test_caseSucceeded();
}

static void test_symlinks(void) {
	struct stat info1;
	struct stat info2;
	test_caseStart("Testing symbolic links");

	/* in ext2 */
	test_assertInt(symlink("/bin","/newlink"),0);
	test_assertInt(lstat("/newlink",&info1),0);
	test_assertTrue(S_ISLNK(info1.st_mode));
	test_assertInt(stat("/newlink",&info1),0);
	test_assertInt(stat("/bin",&info2),0);
	test_assertUInt(info1.st_ino,info2.st_ino);
	test_assertUInt(info1.st_dev,info2.st_dev);
	test_assertInt(unlink("/newlink"),0);

	/* ext2 to vfs */
	test_assertInt(symlink("/sys","/newlink"),0);
	test_assertInt(lstat("/newlink",&info1),0);
	test_assertTrue(S_ISLNK(info1.st_mode));
	test_assertInt(stat("/newlink",&info1),0);
	test_assertInt(stat("/sys",&info2),0);
	test_assertUInt(info1.st_ino,info2.st_ino);
	test_assertUInt(info1.st_dev,info2.st_dev);

	/* symlink to symlink */
	test_assertInt(symlink("/newlink","/newlink2"),0);
	test_assertInt(stat("/newlink2",&info1),0);
	test_assertUInt(info1.st_ino,info2.st_ino);
	test_assertUInt(info1.st_dev,info2.st_dev);
	test_assertInt(unlink("/newlink"),0);
	test_assertInt(stat("/newlink2",&info1),-ENOENT);
	test_assertInt(lstat("/newlink2",&info1),0);
	test_assertInt(unlink("/newlink2"),0);

	/* symlink to itself */
	test_assertInt(symlink("/newlink1","/newlink2"),0);
	test_assertInt(symlink("/newlink2","/newlink1"),0);
	test_assertInt(stat("/newlink1",&info1),-ELOOP);
	test_assertInt(lstat("/newlink1",&info1),0);
	test_assertTrue(S_ISLNK(info1.st_mode));
	test_assertInt(lstat("/newlink2",&info1),0);
	test_assertTrue(S_ISLNK(info1.st_mode));
	test_assertInt(unlink("/newlink2"),0);
	test_assertInt(unlink("/newlink1"),0);

	/* relative symlink */
	test_assertInt(symlink("../..","/bin/newlink"),0);
	test_assertInt(stat("/bin/newlink",&info1),0);
	test_assertInt(stat("/",&info2),0);
	test_assertUInt(info1.st_ino,info2.st_ino);
	test_assertUInt(info1.st_dev,info2.st_dev);
	test_assertInt(unlink("/bin/newlink"),0);

	test_caseSucceeded();
}

static void test_assertCan(const char *path,uint mode) {
	int fd = open(path,mode);
	test_assertTrue(fd >= 0);
	close(fd);
}

static void test_assertCanNot(const char *path,uint mode,int err) {
	test_assertInt(open(path,mode),err);
}

static void fs_createFile(const char *name,const char *content) {
	FILE *f = fopen(name,"w");
	test_assertTrue(f != NULL);
	if(f != NULL) {
		test_assertInt(fwrite(content,1,strlen(content),f),strlen(content));
		fclose(f);
	}
}

static void fs_readFile(const char *name,const char *content) {
	char buf[100] = {0};
	FILE *f = fopen(name,"r");
	test_assertTrue(f != NULL);
	if(f != NULL) {
		test_assertInt(fseek(f,0,SEEK_END),0);
		test_assertInt(ftell(f),strlen(content));
		rewind(f);
		test_assertInt(fread(buf,1,strlen(content),f),strlen(content));
		test_assertStr(buf,content);
		fclose(f);
	}
}
