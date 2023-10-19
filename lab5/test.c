#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "list.h"

#define MAX_LEN 256
#define MAX_PLAYLIST 87

typedef struct playlist {
    struct list_head *playListHead;
    char name[MAX_LEN];
} playlist_t;

typedef struct song {
    struct list_head list;
    char name[MAX_LEN];
} song_t;

const char libpath[] = "songlib";
playlist_t playlists[MAX_PLAYLIST];
playlist_t curPlaylist;
song_t *curSong;
int playListptr = 0;

void add_playlist(char *name)
{
    memcpy(playlists[playListptr].name, name, strlen(name));
    playlists[playListptr].playListHead = malloc(sizeof(struct list_head));
    INIT_LIST_HEAD(playlists[playListptr].playListHead);
    ++playListptr;
}

void switch_playlist(char *name)
{
    for (int i = 0; i < playListptr; ++i)
        if (!strcmp(playlists[i].name, name))
        {
            curPlaylist = playlists[i];
            return;
        }
}

void list_cur_playlist()
{
    struct list_head *t;
    list_for_each(t, curPlaylist.playListHead)
    {
        song_t *tmps = list_entry(t, song_t, list);
        printf("%s\n", tmps->name);
    }
}

void add_song(char *name, playlist_t playlist)
{
    song_t *tmp = malloc(sizeof(song_t));
    memcpy(tmp->name, name, strlen(name));
    INIT_LIST_HEAD(&tmp->list);
    list_add_tail(&tmp->list, playlist.playListHead);
}

void del_song(char *name, playlist_t playlist)
{
    struct list_head *t;
    list_for_each(t, curPlaylist.playListHead)
    {
        song_t *tmps = list_entry(t, song_t, list);
        if (!strcmp(tmps->name, name))
        {
            list_del(&tmps->list);
            free(tmps);
            return;
        }
    }
}

void play(char *name)
{
    struct list_head *t;
    list_for_each(t, curPlaylist.playListHead)
    {
        song_t *tmps = list_entry(t, song_t, list);
        if (!strcmp(tmps->name, name))
        {
            curSong = tmps;
            char command[MAX_LEN] = {0};
            sprintf(command, "madplay %s/%s &", libpath, name);
            system(command);
            return;
        }
    }
}

void stop()
{
    system("killall -9 madplay");
}

void backward()
{
    curSong = list_entry(curSong->list.prev, song_t, list);
    if (curSong == (song_t *)curPlaylist.playListHead)
        curSong = list_entry(curSong->list.prev, song_t, list);
    stop();
    play(curSong->name);
}

void foward()
{
    curSong = list_entry(curSong->list.next, song_t, list);
    if (curSong == (song_t *)curPlaylist.playListHead)
        curSong = list_entry(curSong->list.next, song_t, list);
    stop();
    play(curSong->name);
}

int playList_size()
{
    int r = 0;
    struct list_head *t;
    list_for_each(t, curPlaylist.playListHead)
    {
        ++r;
    }
    return r;
}

int main(int argc, char *argv[])
{
    add_playlist("default");
    switch_playlist("default");

    char songName[MAX_LEN] = {0};

    while (1)
    {
        char c = getchar();
        switch (c)
        {
            case 'a':
                scanf("%s", songName);
                add_song(songName, curPlaylist);
                if (playList_size() == 1)
                    curSong = list_first_entry(curPlaylist.playListHead, song_t, list);
                break;
            case 'p':
                play(curSong->name);
                break;
            case 's':
                stop();
                break;
            case 'l':
                list_cur_playlist();
                break;
            case 'b':
                backward();
                break;
            case 'f':
                foward();
                break;
        }
    }
}
