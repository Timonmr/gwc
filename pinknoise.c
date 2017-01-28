/*****************************************************************************
*   Gnome Wave Cleaner Version 0.19
*   Copyright (C) 2001 Jeffrey J. Welty
*   
*   This program is free software; you can redistribute it and/or
*   modify it under the terms of the GNU General Public License
*   as published by the Free Software Foundation; either version 2
*   of the License, or (at your option) any later version.
*   
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*   
*   You should have received a copy of the GNU General Public License
*   along with this program; if not, write to the Free Software
*   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*******************************************************************************/

/* amplify.c */
#include <stdlib.h>
#include <gnome.h>
#include "gwc.h"

#define BUFSIZE 10000

static gfloat amount_pink = .01 ;
static gfloat amount_white = .01 ;
static int n_pn_rows = 32 ;
static int feather_width = 0 ;

/*
	patest_pink.c

	Generate Pink Noise using Gardner method.
	Optimization suggested by James McCartney uses a tree
	to select which random value to replace.

    x x x x x x x x x x x x x x x x 
     x   x   x   x   x   x   x   x   
       x       x       x       x       
           x               x               
                   x                               

    Tree is generated by counting trailing zeros in an increasing index.
	When the index is zero, no random number is selected.

    This program uses the Portable Audio library which is under development.
	For more information see:   http://www.audiomulch.com/portaudio/

    Author: Phil Burk, http://www.softsynth.com
	
	Revision History:

    Copyleft 1999 Phil Burk - No rights reserved.
*/

#include <stdio.h>
#include <math.h>
#ifdef WIN32
#include <windows.h>
#endif

/************************************************************/
/* Calculate pseudo-random 32 bit number based on linear congruential method. */
static unsigned long GenerateRandomNumber( void )
{
	static unsigned long randSeed = 22222;  /* Change this for different random sequences. */
	randSeed = (randSeed * 196314165) + 907633515;
	return randSeed;
}

#define PINK_MAX_RANDOM_ROWS   (60)
#define PINK_RANDOM_BITS       (24)
#define PINK_RANDOM_SHIFT      ((sizeof(long)*8)-PINK_RANDOM_BITS)

typedef struct
{
	long      pink_Rows[PINK_MAX_RANDOM_ROWS];
	long      pink_RunningSum;   /* Used to optimize summing of generators. */
	int       pink_Index;        /* Incremented each sample. */
	int       pink_IndexMask;    /* Index wrapped by ANDing with this mask. */
	float     pink_Scalar;       /* Used to scale within range of -1.0 to +1.0 */
} PinkNoise;

/* Setup PinkNoise structure for N rows of generators. */
void InitializePinkNoise( PinkNoise *pink, int numRows )
{
	int i;
	long pmax;
	if(numRows > PINK_MAX_RANDOM_ROWS) numRows = PINK_MAX_RANDOM_ROWS ;
	pink->pink_Index = 0;
	pink->pink_IndexMask = (1<<numRows) - 1;
/* Calculate maximum possible signed random value. Extra 1 for white noise always added. */
	pmax = (numRows + 1) * (1<<(PINK_RANDOM_BITS-1));
	pink->pink_Scalar = 1.0f / pmax;
/* Initialize rows. */
	for( i=0; i<numRows; i++ ) pink->pink_Rows[i] = 0;
	pink->pink_RunningSum = 0;
}

#define PINK_MEASURE
#ifdef PINK_MEASURE
	float pinkMax = -999.0;
	float pinkMin =  999.0;
#endif

/* Generate white noise values between -1.0 and +1.0 */
float GenerateWhiteNoise( PinkNoise *pink )
{
    long newRandom = ((long)GenerateRandomNumber()) >> PINK_RANDOM_SHIFT;
    return (float) newRandom / (float) (1<<(PINK_RANDOM_BITS-1)) ;
}

/* Generate Pink noise values between -1.0 and +1.0 */
float GeneratePinkNoise( PinkNoise *pink )
{
	long newRandom;
	long sum;
	float output;

/* Increment and mask index. */
	pink->pink_Index = (pink->pink_Index + 1) & pink->pink_IndexMask;

/* If index is zero, don't update any random values. */
	if( pink->pink_Index != 0 )
	{
	/* Determine how many trailing zeros in PinkIndex. */
	/* This algorithm will hang if n==0 so test first. */
		int numZeros = 0;
		int n = pink->pink_Index;
		while( (n & 1) == 0 )
		{
			n = n >> 1;
			numZeros++;
		}

	/* Replace the indexed ROWS random value.
	 * Subtract and add back to RunningSum instead of adding all the random
	 * values together. Only one changes each time.
	 */
		pink->pink_RunningSum -= pink->pink_Rows[numZeros];
		newRandom = ((long)GenerateRandomNumber()) >> PINK_RANDOM_SHIFT;
		pink->pink_RunningSum += newRandom;
		pink->pink_Rows[numZeros] = newRandom;
	}
	
/* Add extra white noise value. */
	newRandom = ((long)GenerateRandomNumber()) >> PINK_RANDOM_SHIFT;
	sum = pink->pink_RunningSum + newRandom;

/* Scale to range of -1.0 to 0.9999. */
	output = pink->pink_Scalar * sum;

#ifdef PINK_MEASURE
/* Check Min/Max */
	if( output > pinkMax ) pinkMax = output;
	else if( output < pinkMin ) pinkMin = output;
#endif

	return output;
}

void pinknoise(struct sound_prefs *p, long first, long last, int channel_mask)
{
    long left[BUFSIZE], right[BUFSIZE] ;
    long current, i ;
    int loops = 0 ;
    PinkNoise   leftPink;
    PinkNoise   rightPink;

/* Initialize two pink noise signals with different numbers of rows. */
    InitializePinkNoise( &leftPink, n_pn_rows );
    InitializePinkNoise( &rightPink, n_pn_rows );

/* Look at a few values. */
/*      {  */
/*  	int i;  */
/*  	float pink;  */
/*  	for( i=0; i<20; i++ )  */
/*  	{  */
/*  		pink = GeneratePinkNoise( &leftPink );  */
/*  		printf("Pink = %f\n", pink );  */
/*  	}  */
/*      }  */

    current = first ;

    push_status_text("Generating Pink Noise") ;
    update_status_bar(0.0,STATUS_UPDATE_INTERVAL,TRUE) ;

    {
	long max_allowed = INT_MAX-1 ;
	g_print("max_allowed=%ld\n", max_allowed) ;

	while(current <= last) {
	    long n = MIN(last - current + 1, BUFSIZE) ;
	    long tmplast = current + n - 1 ;
	    gfloat prog = (gfloat)(current-first)/(last-first+1) ;

	    n = read_wavefile_data(left, right, current, tmplast) ;

	    update_status_bar(prog,STATUS_UPDATE_INTERVAL,FALSE) ;

	    for(i = 0 ; i < n ; i++) {
		long icurrent = current + i ;
		double feather_p = 1.0 ;
		double wet_left, wet_right ;

		if(icurrent - first < feather_width)
			feather_p = (double)(icurrent-first)/(feather_width) ;

		if(last - icurrent < feather_width)
			feather_p = (double)(last - icurrent)/(feather_width) ;

		if(channel_mask & 0x01) {
		    wet_left = amount_pink*GeneratePinkNoise( &leftPink ) * (double)max_allowed ;
		    wet_left += amount_white*GenerateWhiteNoise( &leftPink ) * (double)max_allowed ;
		    left[i] = lrint(left[i]*(1.0-feather_p) + wet_left*feather_p) ;
		}

		if(channel_mask & 0x02) {
		    wet_right = amount_pink*GeneratePinkNoise( &rightPink ) * (double)max_allowed ;
		    wet_right += amount_white*GenerateWhiteNoise( &rightPink ) * (double)max_allowed ;
		    right[i] = lrint(right[i]*(1.0-feather_p) + wet_right*feather_p) ;

		}
	    }

	    write_wavefile_data(left, right, current, tmplast) ;

	    current += n ;

	    if(last - current < 10) loops++ ;

	    if(loops > 5) {
		warning("infinite loop in amplify_audio, programming error\n") ;
	    }
	}

	resample_audio_data(p, first, last) ;
	save_sample_block_data(p) ;
    }

    update_status_bar(0.0,STATUS_UPDATE_INTERVAL,TRUE) ;
    pop_status_text() ;

    main_redraw(FALSE, TRUE) ;
}

int pinknoise_dialog(struct sound_prefs current, struct view *v)
{
    GtkWidget *dlg, *dialog_table, *n_rows_entry  ;
    GtkWidget *amount_pink_entry ;
    GtkWidget *amount_white_entry ;
    GtkWidget *feather_width_entry ;
    int row = 0 ;
    int dres ;

    dialog_table = gtk_table_new(5,2,0) ;

    gtk_table_set_row_spacings(GTK_TABLE(dialog_table), 4) ;
    gtk_table_set_col_spacings(GTK_TABLE(dialog_table), 6) ;
    gtk_widget_show (dialog_table);

    dlg = gtk_dialog_new_with_buttons("Pink Noise",
			NULL, GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK, GTK_RESPONSE_OK,
			 GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL, NULL);

    amount_pink_entry = add_number_entry_with_label_double(amount_pink, "Amount Pink Noise(0-1)", dialog_table, row++) ;
    amount_white_entry = add_number_entry_with_label_double(amount_white, "Amount White Noise(0-1)", dialog_table, row++) ;
    feather_width_entry = add_number_entry_with_label_int(feather_width, "Feather width", dialog_table, row++) ;
    n_rows_entry = add_number_entry_with_label_int(n_pn_rows, "# rows in noise generator (1-60)", dialog_table, row++) ;

    gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dlg)->vbox), dialog_table, TRUE, TRUE, 0);

    dres = gwc_dialog_run(GTK_DIALOG(dlg)) ;

    if(dres == 0) {
	amount_pink = atof(gtk_entry_get_text((GtkEntry *)amount_pink_entry)) ;
	amount_white = atof(gtk_entry_get_text((GtkEntry *)amount_white_entry)) ;
	feather_width = atoi(gtk_entry_get_text((GtkEntry *)feather_width_entry)) ;
	n_pn_rows = atoi(gtk_entry_get_text((GtkEntry *)n_rows_entry)) ;
    }

    gtk_widget_destroy(dlg) ;

    if(dres == 0)
	return 1 ;

    return 0 ;
}
