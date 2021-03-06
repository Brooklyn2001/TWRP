
/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fs.h>

#define CRYPT_FOOTER_OFFSET 0x4000

char* userdata_path = "/dev/block/bootdevice/by-name/userdata";
char* footer = "/tmp/footer";

static unsigned int get_blkdev_size(int fd) {
	unsigned long nr_sec;

	if ( (ioctl(fd, BLKGETSIZE, &nr_sec)) == -1) {
		nr_sec = 0;
	}

	return (unsigned int) nr_sec;
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("I:dump_footer: Usage: dump_footer backup|restore\n");
		return 0;
	}

	int rfd, wfd;
	unsigned int nr_sec;
	off64_t offset;
	char buffer[16 * 1024];
	char* arg = argv[1];

	if (strncmp(arg, "backup", strlen("backup")) == 0) {
		rfd = open(userdata_path, O_RDONLY);
		if (rfd < 0) {
			printf("E:dump_footer: Cannot open '%s': %s\n", userdata_path, strerror(errno));
			return -1;
		}
		if ((nr_sec = get_blkdev_size(rfd))) {
			offset = ((off64_t)nr_sec * 512) - CRYPT_FOOTER_OFFSET;
		} else {
			printf("E:dump_footer: Failed to get offset\n");
			close(rfd);
			return -1;
		}
		if (lseek64(rfd, offset, SEEK_SET) == -1) {
			printf("E:dump_footer: Failed to lseek64\n");
			close(rfd);
			return -1;
		}
		if (read(rfd, buffer, sizeof(buffer)) != sizeof(buffer)) {
			printf("E:dump_footer: Failed to read: %s\n", strerror(errno));
			close(rfd);
			return -1;
		}
		close(rfd);
		wfd = open(footer, O_WRONLY | O_CREAT, 0644);
		if (wfd < 0) {
			printf("E:dump_footer: Cannot open '%s': %s\n", footer, strerror(errno));
			return -1;
		}
		if (write(wfd, buffer, sizeof(buffer)) != sizeof(buffer)) {
			printf("E:dump_footer: Failed to write: %s\n", strerror(errno));
			close(wfd);
			return -1;
		}
		close(wfd);

		printf("I:dump_footer: userdata crypto footer backed up to %s\n", footer);
	} else if (strncmp(arg, "restore", strlen("restore")) == 0) {
		rfd = open(footer, O_RDONLY);
		if (rfd < 0) {
			printf("E:dump_footer: Cannot open backup '%s': %s\n", footer, strerror(errno));
			return -1;
		}
		if (read(rfd, buffer, sizeof(buffer)) != sizeof(buffer)) {
			printf("E:dump_footer: Failed to read: %s\n", strerror(errno));
			close(rfd);
			return -1;
		}
		close(rfd);

		wfd = open(userdata_path, O_WRONLY);
		if ((nr_sec = get_blkdev_size(wfd))) {
			offset = ((off64_t)nr_sec * 512) - CRYPT_FOOTER_OFFSET;
		} else {
			printf("E:dump_footer: Failed to get offset\n");
			close(wfd);
			return -1;
		}
		if (lseek64(wfd, offset, SEEK_SET) == -1) {
			printf("E:dump_footer: Failed to lseek64\n");
			close(wfd);
			return -1;
		}
		if (write(wfd, buffer, sizeof(buffer)) != sizeof(buffer)) {
			printf("E:dump_footer: Failed to write: %s\n", strerror(errno));
			close(wfd);
			return -1;
		}
		close(wfd);
		if( remove(footer) != 0 ) {
			printf("E:dump_footer: Error deleting backup after restore\n");
		}

		printf("I:dump_footer: userdata crypto footer restored from %s\n", footer);
	} else {
		printf("I:dump_footer: Usage: dump_footer backup|restore\n");
	}
	return 0;
}
