/**
 * $Id$
 * Copyright (C) 2008 - 2011 Nils Asmussen
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

#include <esc/common.h>
#include <esc/driver/video.h>
#include <esc/driver/vterm.h>
#include <esc/io.h>
#include <esc/conf.h>
#include <esc/messages.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "display.h"
#include "bar.h"
#include "object.h"
#include "objlist.h"
#include "game.h"

#define SCORE_WIDTH			10
#define SCORE_HEIGHT		4

#define XYCHAR(x,y)			((y) * WIDTH * 2 + (x) * 2)
#define XYCOL(x,y)			((y) * WIDTH * 2 + (x) * 2 + 1)

static void displ_drawScore(void);
static void displ_drawObjects(void);
static void displ_drawBar(void);
static void displ_restoreBackup(void);
static void displ_setBackup(void);

static const uchar airplane[AIRPLANE_WIDTH * AIRPLANE_HEIGHT * 2] = {
	0xDA, 0x07, 0xC4, 0x07, 0xBF, 0x07,
	0xB3, 0x07, 0xDB, 0x07, 0xB3, 0x07,
	0xD4, 0x07, 0xCD, 0x07, 0xBE, 0x07
};

static const uchar explo1[AIRPLANE_WIDTH * AIRPLANE_HEIGHT * 2] = {
	0xDA, 0x07, 0xC4, 0x07, 0xBF, 0x07,
	0xB3, 0x07, 0xB2, 0x0E, 0xB3, 0x07,
	0xD4, 0x07, 0xCD, 0x07, 0xBE, 0x07
};

static const uchar explo2[AIRPLANE_WIDTH * AIRPLANE_HEIGHT * 2] = {
	0xB0, 0x0E, 0xB0, 0x0E, 0xB0, 0x0E,
	0xB0, 0x0E, 0xB0, 0x0E, 0xB0, 0x0E,
	0xB0, 0x0E, 0xB0, 0x0E, 0xB0, 0x0E
};

static const uchar explo3[AIRPLANE_WIDTH * AIRPLANE_HEIGHT * 2] = {
	0xB1, 0x06, 0xB1, 0x06, 0xB1, 0x06,
	0xB1, 0x06, 0xB1, 0x06, 0xB1, 0x06,
	0xB1, 0x06, 0xB1, 0x06, 0xB1, 0x06
};

static const uchar explo4[AIRPLANE_WIDTH * AIRPLANE_HEIGHT * 2] = {
	0xB0, 0x08, 0xB0, 0x08, 0xB0, 0x08,
	0xB0, 0x08, 0xB0, 0x08, 0xB0, 0x08,
	0xB0, 0x08, 0xB0, 0x08, 0xB0, 0x08
};

static const uchar bullet[BULLET_WIDTH * BULLET_HEIGHT * 2] = {
	0x04, 0x04
};

sVTSize ssize;
static int video;
static uchar *buffer = NULL;
static uchar *backup = NULL;

bool displ_init(void) {
	/* get current device */
	char device[MAX_PATH_LEN];
	int res = vterm_getDevice(STDIN_FILENO,device,sizeof(device));
	if(res < 0) {
		fprintf(stderr,"Unable to get video-device: %s\n",strerror(-res));
		return false;
	}

	/* open it */
	video = open(device,IO_WRITE | IO_MSGS);
	if(video < 0) {
		fprintf(stderr,"Unable to open video-device '%s': %s\n",device,strerror(-video));
		return false;
	}

	/* get screen size */
	if((res = video_getSize(video,&ssize)) < 0) {
		fprintf(stderr,"Unable to get screensize: %s\n",strerror(-res));
		return false;
	}

	/* first line is the title */
	HEIGHT--;
	buffer = (uchar*)malloc(WIDTH * HEIGHT * 2);
	if(!buffer) {
		printe("Unable to alloc mem for buffer");
		return false;
	}
	backup = (uchar*)malloc(WIDTH * HEIGHT * 2);
	if(!backup) {
		printe("Unable to alloc mem for backup");
		return false;
	}
	displ_setBackup();
	return true;
}

void displ_destroy(void) {
	close(video);
	free(buffer);
	free(backup);
}

void displ_update(void) {
	displ_restoreBackup();
	displ_drawBar();
	displ_drawObjects();
	displ_drawScore();
	if(seek(video,WIDTH * 2,SEEK_SET) < 0)
		printe("Seek to %d failed",WIDTH * 2);
	if(write(video,buffer,WIDTH * HEIGHT * 2) < 0)
		printe("Write to video-device failed");
}

static void displ_drawScore(void) {
	size_t x,i;
	char scoreStr[SCORE_WIDTH];
	snprintf(scoreStr,sizeof(scoreStr),"%*u",SCORE_WIDTH - 2,game_getScore());
	for(i = 0, x = WIDTH - SCORE_WIDTH + 1; scoreStr[i]; i++, x++)
		backup[XYCHAR(x,3)] = scoreStr[i];
}

static void displ_drawObjects(void) {
	int y;
	sSLNode *n;
	sObject *o;
	const uchar *src;
	sSLList *objects = objlist_get();
	for(n = sll_begin(objects); n != NULL; n = n->next) {
		o = (sObject*)n->data;
		if((size_t)(o->x + PADDING + o->width) >= (size_t)(WIDTH - SCORE_WIDTH) &&
				(o->y + PADDING) <= SCORE_HEIGHT) {
			/* don't draw objects over the score-area */
			continue;
		}

		switch(o->type) {
			case TYPE_AIRPLANE:
				src = airplane;
				break;
			case TYPE_BULLET:
				src = bullet;
				break;
			case TYPE_EXPLO1:
				src = explo1;
				break;
			case TYPE_EXPLO2:
				src = explo2;
				break;
			case TYPE_EXPLO3:
				src = explo3;
				break;
			case TYPE_EXPLO4:
			default:
				src = explo4;
				break;
		}

		for(y = o->y + PADDING; y < o->y + PADDING + o->height; y++) {
			memcpy(buffer + XYCHAR(o->x + PADDING,y),src,o->width * 2);
			src += o->width * 2;
		}
	}
}

static void displ_drawBar(void) {
	size_t x,start,end;
	bar_getDim(&start,&end);
	for(x = start + PADDING; x <= end; x++) {
		buffer[XYCHAR(x,HEIGHT - 2)] = 0xDB;
		buffer[XYCOL(x,HEIGHT - 2)] = 0x07;
	}
}

static void displ_restoreBackup(void) {
	memcpy(buffer,backup,WIDTH * HEIGHT * 2);
}

static void displ_setBackup(void) {
	size_t i,x,y;
	const char *title = "Score:";
	/* fill bg */
	for(i = 0; i < WIDTH * HEIGHT * 2; i += 2) {
		backup[i] = ' ';
		backup[i + 1] = 0x07;
	}
	/* top and bottom border */
	for(x = 1; x < WIDTH - 1; x++) {
		backup[XYCHAR(x,0)] = 0xCD;
		backup[XYCOL(x,0)] = 0x07;
		backup[XYCHAR(x,HEIGHT - 1)] = 0xCD;
		backup[XYCOL(x,HEIGHT - 1)] = 0x07;
	}
	/* left and right border */
	for(y = 1; y < HEIGHT - 1; y++) {
		backup[XYCHAR(0,y)] = 0xBA;
		backup[XYCOL(0,y)] = 0x07;
		backup[XYCHAR(WIDTH - 1,y)] = 0xBA;
		backup[XYCOL(WIDTH - 1,y)] = 0x07;
	}
	/* corners */
	backup[XYCHAR(0,0)] = 0xC9;
	backup[XYCOL(0,0)] = 0x07;
	backup[XYCHAR(WIDTH - 1,0)] = 0xBB;
	backup[XYCOL(WIDTH - 1,0)] = 0x07;
	backup[XYCHAR(0,HEIGHT - 1)] = 0xC8;
	backup[XYCOL(0,HEIGHT - 1)] = 0x07;
	backup[XYCHAR(WIDTH - 1,HEIGHT - 1)] = 0xBC;
	backup[XYCOL(WIDTH - 1,HEIGHT - 1)] = 0x07;

	/* score-border */
	backup[XYCHAR(WIDTH - SCORE_WIDTH,0)] = 0xCB;
	for(y = 1; y < SCORE_HEIGHT; y++)
		backup[XYCHAR(WIDTH - SCORE_WIDTH,y)] = 0xBA;
	backup[XYCHAR(WIDTH - SCORE_WIDTH,SCORE_HEIGHT)] = 0xC8;
	for(x = WIDTH - SCORE_WIDTH + 1; x < WIDTH - 1; x++)
		backup[XYCHAR(x,SCORE_HEIGHT)] = 0xCD;
	backup[XYCHAR(WIDTH - 1,SCORE_HEIGHT)] = 0xB9;

	/* "Score:" */
	for(x = WIDTH - SCORE_WIDTH + 1; *title; x++)
		backup[XYCHAR(x,1)] = *title++;
}
