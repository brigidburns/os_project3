/*
fractal.c - Sample Mandelbrot Fractal Display
Starting code for CSE 30341 Project 3.
*/

#include "gfx.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <complex.h>
#include <pthread.h>

/*
Compute the number of iterations at point x, y
in the complex space, up to a maximum of maxiter.
Return the number of iterations at that point.

This example computes the Mandelbrot fractal:
z = z^2 + alpha

Where z is initially zero, and alpha is the location x + iy
in the complex plane.  Note that we are using the "complex"
numeric type in C, which has the special functions cabs()
and cpow() to compute the absolute values and powers of
complex values.
*/
typedef struct {
	int done; 
	int x; 
	int y;  
} task_args; 

typedef struct {
	double xmin; 
	double xmax; 
	double ymin; 
	double ymax; 
	int maxiter; 
	int num_threads;
	task_args **tasks;  
} thread_args;  

task_args **tasks; 
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;  

static int compute_point( double x, double y, int max )
{
	double complex z = 0;
	double complex alpha = x + I*y;

	int iter = 0;

	while( cabs(z)<4 && iter < max ) {
		z = cpow(z,2) + alpha;
		iter++;
	}

	return iter;
}

/*
Compute an entire image, writing each point to the given bitmap.
Scale the image to the range (xmin-xmax,ymin-ymax).
*/

void *compute_image(void *args)
{
	thread_args *thread = (thread_args *) args;
	int i,j;  
	int width = gfx_xsize(); 
	int height = gfx_ysize();
	int x_size = width/20; 
	int y_size = height/20; 
	int x_task, y_task; 

	// For every pixel i,j, in the image...
	while (1) {

		// Lock critical section 
		pthread_mutex_lock(&lock); 
		x_task = -1; 
		y_task = -1; 

		// Find available tasks
		for(i=0; i<y_size; i++) {
			for(j=0; j<x_size; j++) {
				if(thread->tasks[i][j].done==0) {
					x_task = thread->tasks[i][j].x; 
					y_task = thread->tasks[i][j].y;
					thread->tasks[i][j].done = 1; 
					break; 
				} 
			}

			if (x_task >= 0 && y_task >= 0) {
				break; 
			}
		}

		// Unlock critical section
		pthread_mutex_unlock(&lock); 

		// Break if all tasks finished 
		if (x_task < 0 || y_task < 0) {
			break; 
		}

		for(j=0;j<20;j++) {
			for(i=0;i<20;i++) {

				// Scale from pixels i,j to coordinates x,y
				double x = thread->xmin + (i+x_task)*(thread->xmax-thread->xmin)/width;
				double y = thread->ymin + (j+y_task)*(thread->ymax-thread->ymin)/height;

				// Compute the iterations at x,y
				int iter = compute_point(x,y,thread->maxiter);

				// Convert a iteration number to an RGB color.
				// (Change this bit to get more interesting colors.)
				int color = (255 * iter / thread->maxiter)*10;
				int color2 = (255 * iter / thread->maxiter)*20;
				int color3 = (255 * iter / thread->maxiter)*50;

				// Lock the critical section
				pthread_mutex_lock(&lock); 
				gfx_color(color,color2,color3);

				// Plot the point on the screen.
				gfx_point(i+x_task,j+y_task);

				// Unlock the critical section
				pthread_mutex_unlock(&lock); 
			}
		}
	}

	pthread_exit(NULL); 
}

void create_threads(double xmin, double xmax, double ymin, double ymax, int maxiter, int num_threads) {
	int i, j, rc;
	int height = gfx_ysize(); 
	int width = gfx_xsize();
	int x_size = width/20, y_size = height/20;  
	pthread_t p[num_threads];  
	thread_args args[num_threads];
	
	// Allocate memory for tasks array
	tasks = (task_args **) calloc (height, sizeof(task_args *));
	for(i=0; i<height; i++) {
		tasks[i] = (task_args *) calloc (width, sizeof(task_args)); 
	}

	// Initialize tasks
	for (i=0; i<y_size; i++) {
		for (j=0; j<x_size; j++) {
			tasks[i][j].x = j*20; 
			tasks[i][j].y = i*20; 
			tasks[i][j].done = 0; 
		}
	}


	for (i = 0; i < num_threads; i++) {
		args[i].xmin = xmin;
		args[i].xmax = xmax;
		args[i].ymin = ymin;
		args[i].ymax = ymax;
		args[i].maxiter = maxiter;
		args[i].num_threads = num_threads;
		args[i].tasks = tasks; 
		rc = pthread_create(&p[i], NULL, compute_image, &args[i]); 
		if (rc < 0) {
			exit(1); 
		}
	}

	for (i=0; i < num_threads; i++) {
		pthread_join(p[i], NULL); 
	}

	// Free tasks
	for(i=0; i<height; i++) {
		free(tasks[i]); 
	}
	free(tasks); 
}

// Move up function
void move_up(double xmin, double xmax, double ymin, double ymax, int maxiter, int num_threads) {
	double yrange = ymax-ymin; 
	ymin -= yrange/4;
	ymax -= yrange/4;  	
	create_threads(xmin, xmax, ymin, ymax, maxiter, num_threads); 
	return; 
}

// Move down function
void move_down(double xmin, double xmax, double ymin, double ymax, int maxiter, int num_threads) {
	double yrange = ymax-ymin; 
	ymin += yrange/4; 
	ymax += yrange/4; 
	create_threads(xmin, xmax, ymin, ymax, maxiter, num_threads); 
	return; 
}

// Move left function
void move_left(double xmin, double xmax, double ymin, double ymax, int maxiter, int num_threads) {
	double xrange = xmax-xmin; 
	xmin -= xrange/4; 
	xmax -= xrange/4; 
	create_threads(xmin, xmax, ymin, ymax, maxiter, num_threads); 
	return; 
}

// Move right function
void move_right(double xmin, double xmax, double ymin, double ymax, int maxiter, int num_threads) {
	double xrange = xmax-xmin; 
	xmin += xrange/4; 
	xmax += xrange/4; 
	create_threads(xmin, xmax, ymin, ymax, maxiter, num_threads); 
	return; 
}

// Zoom in function
void zoom_in(double xmin, double xmax, double ymin, double ymax, int maxiter, int num_threads) {
	double xrange = xmax-xmin; 
	double yrange = ymax-ymin; 
	xmin -= xrange/4; 
	xmax -= xrange/4; 
	ymin -= yrange/4; 
	ymax -= yrange/4; 
	create_threads(xmin, xmax, ymin, ymax, maxiter, num_threads); 
	return; 
}

// Zoom out function
void zoom_out(double xmin, double xmax, double ymin, double ymax, int maxiter, int num_threads) {
	double xrange = xmax-xmin; 
	double yrange = ymax-ymin; 
	xmin += xrange/4; 
	xmax += xrange/4; 
	ymin += yrange/4; 
	ymax += yrange/4; 
	create_threads(xmin, xmax, ymin, ymax, maxiter, num_threads); 
	return; 
}

// Recenter function
void recenter(double xmin, double xmax, double ymin, double ymax, int maxiter, int num_threads) {
	int width = gfx_xsize();
	int height = gfx_ysize(); 
	int xpos = gfx_xpos(); 
	int ypos = gfx_ypos(); 

	double xrange = xmax-xmin; 
	double yrange = ymax-ymin;

	double xcenter = xmin + xrange * xpos / width; 
	double ycenter = ymin + yrange * ypos / height;  

	xmax = xcenter + xrange; 
	xmin = xcenter - xrange; 
	ymax = ycenter + yrange; 
	ymin = ycenter - yrange; 

	create_threads(xmin, xmax, ymin, ymax, maxiter, num_threads); 
 
	return; 
}

// Change maxiter
void change_maxiter(double xmin, double xmax, double ymin, double ymax, int maxiter, int num_threads) {
	maxiter *= 5; 
	create_threads(xmin, xmax, ymin, ymax, maxiter, num_threads); 
	return; 
}

int main( int argc, char *argv[] )
{
	// The initial boundaries of the fractal image in x,y space.

	double xmin=-1.5;
	double xmax= 0.5;
	double ymin=-1.0;
	double ymax= 1.0;
	int num_threads = 1; 

	// Maximum number of iterations to compute.
	// Higher values take longer but have more detail.
	int maxiter=500;

	// Open a new window.
	gfx_open(640,480,"Mandelbrot Fractal");

	// Show the configuration, just in case you want to recreate it.
	printf("coordinates: %lf %lf %lf %lf\n",xmin,xmax,ymin,ymax);

	// Fill it with a dark blue initially.
	gfx_clear_color(0,0,255);
	gfx_clear();

	// Display the fractal image
	create_threads(xmin,xmax,ymin,ymax,maxiter,num_threads);
	gfx_flush();
	int c = 0;
	while(1) {
		// Wait for a key or mouse click.
		if (gfx_event_waiting()) {
		c = gfx_wait();

		switch (c) {
			case ('r'):
				// Move right
				move_right(xmin, xmax, ymin, ymax, maxiter, num_threads); 
				break;  
			case ('l'):
				// Move left
				move_left(xmin, xmax, ymin, ymax, maxiter, num_threads); 
				break;
			case ('u'):
				// Move up
				move_up(xmin, xmax, ymin, ymax, maxiter, num_threads); 
				break;  
			case ('d'):
				// Move down
				move_down(xmin, xmax, ymin, ymax, maxiter, num_threads); 
				break; 
			case ('q'):
				// Quit if q is pressed
				pthread_mutex_destroy(&lock); 
				exit(0); 
			case ('i'): 
				// Zoom in
				zoom_in(xmin, xmax, ymin, ymax, maxiter, num_threads); 
				break;
			case ('+'):
				// Zoom in
				zoom_in(xmin, xmax, ymin, ymax, maxiter, num_threads); 
				break; 
			case ('o'):
				// Zoom out
				zoom_out(xmin, xmax, ymin, ymax, maxiter, num_threads); 
				break;
			case ('-'):
				// Zoom out
				zoom_out(xmin, xmax, ymin, ymax, maxiter, num_threads); 
				break;
			case (1):
				// Recenter
				recenter(xmin, xmax, ymin, ymax, maxiter, num_threads); 
				break;  
			case (2):
				// Recenter
				recenter(xmin, xmax, ymin, ymax, maxiter, num_threads); 
				break; 
			case (3):
				// Recenter
				recenter(xmin, xmax, ymin, ymax, maxiter, num_threads); 
				break; 
			case ('m'):
				// Change maxiter
				change_maxiter(xmin, xmax, ymin, ymax, maxiter, num_threads); 
				break; 
			case ('1'):
				// Run with 1 thread
				num_threads = 1;
				break; 
			case ('2'):
				// Run with 2 threads
				num_threads = 2; 
				break; 
			case ('3'):
				// Run with 3 threads
				num_threads = 3; 
				break; 
			case ('4'):
				// Run with 4 threads
				num_threads = 4; 
				break; 
			case ('5'):
				// Run with 5 threads
				num_threads = 5; 
				break; 
			case ('6'):
				// Run with 6 threads
				num_threads = 6; 
				break; 
			case ('7'):
				// Run with 7 threads
				num_threads = 7;   
				break; 
			case ('8'):
				// Run with 8 threads
				num_threads = 8;   
				break; 
			default:
				break; 
		}
		} 
	}
	return 0;
}
