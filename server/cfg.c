#define _XOPEN_SOURCE
#include <stdio.h>
#include <string.h>

#include "cfg.h"
#include "restrict.h"
#include "../config.h"

#define MAX_LINE 128

/*
 * config file:
 *
 * ALLOW IPv4
 * ALLOW IPv6
 * ALLOW Hostname
 * (all other hosts are denied)
 *
 * PORT portno
 * PASS rootpass
 * DESC server desc
 */

extern char glob_port[MAX_PORT_LEN];
extern char glob_pass[MAX_PASS_LEN];
extern char glob_desc[MAX_DESC_LEN];

int cfg_read(FILE *f)
{
	char line[MAX_LINE];
	int n = 1;

	while(fgets(line, sizeof line, f)){
		char *p = strchr(line, '\n');
		if(p)
			*p = '\0';
		p = strchr(line, '#');
		if(p)
			*p = '\0';

		if(!*line)
			continue;

		if(!strncmp(line, "ALLOW ", 6)){
			if(restrict_addaddr(line+6))
				return 1;
		}else if(!strncmp(line, "PORT ", 5)){
			strncpy(glob_port, line+5, MAX_PORT_LEN);

		}else if(!strncmp(line, "PASS ", 5)){
			strncpy(glob_pass, line+5, MAX_PASS_LEN);

		}else if(!strncmp(line, "DESC ", 5)){
			strncpy(glob_desc, line+5, MAX_DESC_LEN);

		}else{
			fprintf(stderr, "Invalid config file syntax: line %d: \"%s\"\n",
					n, line);
			return 1;
		}

		n++; /* used above */
	}

	if(ferror(f)){
		perror("fgets()");
		return 1;
	}

	return 0;
}

void cfg_end()
{
	restrict_end();
}
