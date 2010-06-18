#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "strings.h"
#include "line.h"
#include "ui/term.h"

#define LINE_START_SIZE	 5


int rline(char **line, int *linesize)
{
	int c, linepos = 0, loop = 1;

	if(*line == NULL){
		*line = malloc(LINE_START_SIZE);
		if(!*line)
			return 2;

		*linesize = LINE_START_SIZE;
	}
	memset(*line, '\0', *linesize);

	do{
		c = getchar();
		switch(c){
			case '\n':
				(*line)[linepos++] = '\0';
				loop = 0;
				break;

			case EOF:
				if(linepos == 0){
					(*line)[0] = EOF;
					(*line)[1] = '\0';
					loop = 0;
					break;
				}
				/* fall through - auto complete */

				/*
			case '\t':
				if(linepos)
					switch(autocomplete(line, linesize, &linepos)){
						case COMP_MEMERR:
							return 2;
						case COMP_BADINPUT:
						case COMP_SUCCESS:
							break;
					}
				break;
				*/

				/*
				 * TODO FIXME: unprintable chars processing, aka ^G, ^W
				case ^W:
				 */

			default:
				if(addtostring(c, &linepos, linesize, line))
					return 2;
		}
	}while(loop);

	return 0;
}
