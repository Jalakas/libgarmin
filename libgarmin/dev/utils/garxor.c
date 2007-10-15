#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


int main(int argc, char **argv)
{
	char buf[4096];
	int i;
	int fd, fd1;
	int rc;
	int first = 1;
	char xorb = 0;
	if (argc != 3) {
		fprintf(stderr, "usage: %s infile outfile\n", argv[0]);
		return -1;
	}
	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Can not open:%s\n", argv[1]);
		return -1;
	}
	fd1 = open(argv[2], O_RDWR|O_CREAT|O_TRUNC, 0660);
	if (fd1 < 0) {
		fprintf(stderr, "Can not open:%s\n", argv[2]);
		return -1;
	}
	while ((rc = read(fd, buf, sizeof(buf))) > 0) {
		i = 0;
		if (first) {
			xorb = buf[0];
			first = 0;
			// i ++;
		}
		for (;i<rc; i++) {
			buf[i] ^= xorb;
		}
		write(fd1, buf, rc);
	}
	close(fd);
	close(fd1);
	return 0;
}
