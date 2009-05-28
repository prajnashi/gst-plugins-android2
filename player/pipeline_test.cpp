/* GStreamer
 * Copyright (C) <2009> Prajnashi S <prajnashi@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "GstPlayerPipeline.h"
#include "GstLog.h"

using namespace android;

struct App
{
    int fd;
};

static void *test_fd_thread_func(void *vptr_args)
{
    App* app = (App*)vptr_args;
    off_t offset = 0;
    char buffer[128];

    printf("test_fd_thread_func(), enter thread %d, fd: %d\n", gettid(), app->fd);

    offset = lseek(app->fd, 128, SEEK_SET);
    printf("test_fd_thread_func(), seek to: %d\n", (int)offset);

    offset = read(app->fd, buffer, 128);
    printf("test_fd_thread_func(), read: %d\n", (int)offset);
 
    return NULL;
}


static void test_fd()
{
    App* app = new App;
    pthread_t thread;
    off_t offset = 0;


    app->fd = open("/sdcard/1.m4a",O_RDONLY);
    printf("test_fd(), fd: %d\n", app->fd);
    
    offset = lseek(app->fd, 64, SEEK_SET);
    printf("test_fd(), seek to: %d\n", (int)offset);
    
    printf("test_fd(), current thread %d, start new thread\n", gettid());
    if (pthread_create(&thread, NULL, test_fd_thread_func, app) != 0)
    {
        printf("test_fd(), ERROR, cannot create thread!\n");
        goto EXIT;
    }

    printf("test_fd(), wait for thread completed\n");
    pthread_join(thread, NULL);


EXIT:
    if(app->fd)
        close(app->fd);
    if(app)
        delete app;
    printf("test_fd(), quit\n");
}

static void test_setDataSource_fd()
{
    const char* file = "/sdcard/1.m4a";
    int fd = open(file, O_RDONLY);
    GstPlayerPipeline *pipeline = new GstPlayerPipeline(NULL);
    printf("setDataSource\n");
    pipeline->setDataSource(fd, 0, 0);
    printf("prepare\n");
    pipeline->prepare();
    sleep(5);
    printf("start\n");
    pipeline->start();
    sleep(5);

    printf("delete pipeline\n");

    delete pipeline;
}

static void test_setDataSource_url()
{
    //GstPlayer *pipeline= new GstPlayer();
    //sp<MediaPlayerService::AudioOutput> audio_output = new
    //MediaPlayerService::AudioOutput();

    GstPlayerPipeline *pipeline = new GstPlayerPipeline(NULL);
    pipeline->setDataSource ("file:///sdcard/1.m4a");
    printf ("prepare\n");
    pipeline->prepare();
    sleep (2);

    printf ("start\n");
    pipeline->start();
    sleep(2);

    // FIXME: stop() -> start() seg fault!
#if 0
    printf ("stop\n");
    pipeline->stop();
    pipeline->setDataSource ("file:///sdcard/1.m4a");
    sleep(2);    

    printf ("prepare\n");
    pipeline->prepare();
    sleep(2);
#endif
    
    //
    //
    printf ("pause\n");
    pipeline->pause();
    sleep(2);

    printf ("start\n");
    pipeline->start();
    sleep(2);

#if 0
    printf ("stop\n");
    pipeline->stop();
    pipeline->setDataSource ("file:///sdcard/1.m4a");
    sleep(2);

    printf ("start\n");
    pipeline->start();
    sleep(2);

    printf ("stop\n");
    pipeline->stop();    
#endif

    delete pipeline;
    printf ("delete pipeline\n");
}


int main (int argc, char **argv[])
{
    test_setDataSource_fd();
    // test_setDataSource_url(); test_fd();
    return 0;



} 
