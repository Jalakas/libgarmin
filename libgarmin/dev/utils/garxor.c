#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include "../src/GarminTypedef.h"

static int gar_xor_file(char *in, char *out)
{
	char buf[4096];
	int fd, fd1;
	int first = 1;
	char xorb = 0;
	int ret = -1;
	int rc,i;
	fd = open(in, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Can not open:%s\n", in);
		return -1;
	}
	fd1 = open(out, O_RDWR|O_CREAT|O_TRUNC, 0660);
	if (fd1 < 0) {
		fprintf(stderr, "Can not open:%s\n", out);
		return -1;
	}
	while ((rc = read(fd, buf, sizeof(buf))) > 0) {
		i = 0;
		if (first) {
			xorb = buf[0];
		}
		for (;i<rc; i++) {
			buf[i] ^= xorb;
		}
		if (first) {
			struct hdr_img_t *img = (struct hdr_img_t *)buf;
			if (strncmp(img->signature,"DSKIMG",6)) {
				fprintf(stderr, "%s is not garmin image - invalid signature\n",
					in);
				goto out;
			}
			if (strncmp(img->identifier,"GARMIN",6)) {
				fprintf(stderr, "%s is not garmin image - invalid identifier\n",
					in);
				goto out;
			}
			first = 0;
		}
		if (write(fd1, buf, rc) != rc) {
			fprintf(stderr, "Error writing %d bytes\n", rc);
			ret = -1;
			goto out;
		}
	}
	ret = 0;
out:
	close(fd);
	close(fd1);
	if (ret < 0)
		unlink(out);
	return ret;
}

static int gar_xor_dir(char *path)
{
	struct dirent **namelist;
	int n, rc;
	int cnt = 0;
	char in[4096];
	char out[4096];
	char ren[4096];
	n = scandir(path, &namelist, 0, NULL);
	if (n < 0) {
		fprintf(stderr, "Bad directory path\n");
		return -1;
	}
	while(n--) {
		if ( *namelist[n]->d_name == '.') {
			free(namelist[n]);
			continue;
		}
		fprintf(stderr, "Processing: [%s]\n", namelist[n]->d_name);
		sprintf(in,"%s/%s", path, namelist[n]->d_name);
		sprintf(out,"%s/%s.tmp", path, namelist[n]->d_name);
		rc = gar_xor_file(in, out);
		if (rc == 0) {
			sprintf(ren,"%s/%s.orig", path, namelist[n]->d_name);
			if (!rename(in, ren)) {
				if (!rename(out, in)) {
					fprintf(stdout, "Converted [%s]\n",
							namelist[n]->d_name);
					unlink(ren);
					cnt++;
				} else {
					fprintf(stderr, "Error renaming [%s] to [%s]\n",
						out, in);
					// Can't help if that fails
					rename(ren, in);
				}
			} else {
				fprintf(stderr, "Error renaming [%s] to [%s]\n",
					in, ren);
				unlink(out);
			}
			cnt++;
		} else {
			fprintf(stderr, "Error processing: [%s]\n", 
					namelist[n]->d_name);
		}
		free(namelist[n]);
	}
	free(namelist);
	return cnt;
}

int main(int argc, char **argv)
{
	if (argc != 3) {
		fprintf(stderr, "usage: %s infile outfile | -d directory\n", argv[0]);
		return -1;
	}
	if (!strcmp(argv[1], "-d")) {
		gar_xor_dir(argv[2]);
	} else {
		if (strcmp(argv[1], argv[2])) {
			gar_xor_file(argv[1], argv[2]);
		} else {
			char buf[4096];
			sprintf(buf, "%s.tmp", argv[1]);
			if (gar_xor_file(argv[1], buf) > -1) {
				unlink(argv[1]);
				rename(buf, argv[1]);
			}
		}
	}
	return 0;
}
