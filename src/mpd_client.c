/* ympd
   (c) 2013-2014 Andrew Karpow <andy@ndyk.de>
   This project's homepage is: http://www.ympd.org
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
/**
 已经改成新的ws模式，不在本函数里调用mg_ws_send,而是统一在主程序的消息MG_EV_WAKEUP里发送
 需要通过bool mg_wakeup(struct mg_mgr *mgr, unsigned long conn_id, const void *buf,size_t len)发送
  */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <mpd/client.h>
#include <mpd/message.h>

#include "mpd_client.h"
#include "config.h"
#include "json_encode.h"

char dirble_api_token[28];
struct t_mpd mpd;
/* forward declaration */
static void mpd_notify_callback(struct thread_data *p);

const char * mpd_cmd_strs[] = {
    MPD_CMDS(GEN_STR)
};

char * get_arg1 (char *p) {
	return strchr(p, ',') + 1;
}

char * get_arg2 (char *p) {
	return get_arg1(get_arg1(p));
}

static inline enum mpd_cmd_ids get_cmd_id(char *cmd)
{
    for(int i = 0; i < sizeof(mpd_cmd_strs)/sizeof(mpd_cmd_strs[0]); i++)
        if(!strncmp(cmd, mpd_cmd_strs[i], strlen(mpd_cmd_strs[i])))
            return i;

    return -1;
}

/*add by luping */
void mg_ws_send_error(struct thread_data *p,const char *buf)
{
    size_t n;

    if(buf){
        n = snprintf(mpd.buf, MAX_SIZE, "{\"type\":\"error\",\"data\":\"%s\"}", 
            buf);
        mg_wakeup(p->mgr, p->conn_id, mpd.buf, n);
    }
}
/*end of add by luping*/
void callback_mpd(struct mg_connection *c,struct mg_ws_message *wm)
{
    enum mpd_cmd_ids cmd_id = get_cmd_id(wm->data.buf);
    size_t n = 0;
    unsigned int uint_buf, uint_buf_2;
    int int_buf;
    char *p_charbuf = NULL, *token;
    struct thread_data *p;

    if(cmd_id == -1)
        return;

    if(mpd.conn_state != MPD_CONNECTED && cmd_id != MPD_API_SET_MPDHOST &&
        cmd_id != MPD_API_GET_MPDHOST && cmd_id != MPD_API_SET_MPDPASS &&
        cmd_id != MPD_API_GET_DIRBLEAPITOKEN)
        return;

    switch(cmd_id)
    {
        case MPD_API_UPDATE_DB:
            mpd_run_update(mpd.conn, NULL);
            break;
        case MPD_API_SET_PAUSE:
            mpd_run_toggle_pause(mpd.conn);
            break;
        case MPD_API_SET_PREV:
            mpd_run_previous(mpd.conn);
            break;
        case MPD_API_SET_NEXT:
            mpd_run_next(mpd.conn);
            break;
        case MPD_API_SET_PLAY:
            mpd_run_play(mpd.conn);
            break;
        case MPD_API_SET_STOP:
            mpd_run_stop(mpd.conn);
            break;
        case MPD_API_RM_ALL:
            mpd_run_clear(mpd.conn);
            break;
        case MPD_API_RM_TRACK:
            if(sscanf(wm->data.buf, "MPD_API_RM_TRACK,%u", &uint_buf))
                mpd_run_delete_id(mpd.conn, uint_buf);
            break;
        case MPD_API_RM_RANGE:
            if(sscanf(wm->data.buf, "MPD_API_RM_RANGE,%u,%u", &uint_buf, &uint_buf_2))
                mpd_run_delete_range(mpd.conn, uint_buf, uint_buf_2);
            break;
        case MPD_API_MOVE_TRACK:
            if (sscanf(wm->data.buf, "MPD_API_MOVE_TRACK,%u,%u", &uint_buf, &uint_buf_2) == 2)
            {
                uint_buf -= 1;
                uint_buf_2 -= 1;
                mpd_run_move(mpd.conn, uint_buf, uint_buf_2);
            }
            break;
        case MPD_API_PLAY_TRACK:
            if(sscanf(wm->data.buf, "MPD_API_PLAY_TRACK,%u", &uint_buf))
                mpd_run_play_id(mpd.conn, uint_buf);
                //mpd_run_play_pos(mpd.conn, uint_buf);
            break;
        case MPD_API_TOGGLE_RANDOM:
            if(sscanf(wm->data.buf, "MPD_API_TOGGLE_RANDOM,%u", &uint_buf))
                mpd_run_random(mpd.conn, uint_buf);
            break;
        case MPD_API_TOGGLE_REPEAT:
            if(sscanf(wm->data.buf, "MPD_API_TOGGLE_REPEAT,%u", &uint_buf))
                mpd_run_repeat(mpd.conn, uint_buf);
            break;
        case MPD_API_TOGGLE_CONSUME:
            if(sscanf(wm->data.buf, "MPD_API_TOGGLE_CONSUME,%u", &uint_buf))
                mpd_run_consume(mpd.conn, uint_buf);
            break;
        case MPD_API_TOGGLE_SINGLE:
            if(sscanf(wm->data.buf, "MPD_API_TOGGLE_SINGLE,%u", &uint_buf))
                mpd_run_single(mpd.conn, uint_buf);
            break;
        case MPD_API_TOGGLE_CROSSFADE:
            if(sscanf(wm->data.buf, "MPD_API_TOGGLE_CROSSFADE,%u", &uint_buf))
                mpd_run_crossfade(mpd.conn, uint_buf);
            break;
        case MPD_API_GET_OUTPUTS:
            mpd.buf_size = mpd_put_outputs(mpd.buf, 1);
            mpd_notify_callback(c->fn_data);
            break;
        case MPD_API_TOGGLE_OUTPUT:
            if (sscanf(wm->data.buf, "MPD_API_TOGGLE_OUTPUT,%u,%u", &uint_buf, &uint_buf_2)) {
                if (uint_buf_2)
                    mpd_run_enable_output(mpd.conn, uint_buf);
                else
                    mpd_run_disable_output(mpd.conn, uint_buf);
            }
            break;
        case MPD_API_SET_VOLUME:
            if(sscanf(wm->data.buf, "MPD_API_SET_VOLUME,%ud", &uint_buf) && uint_buf <= 100)
                mpd_run_set_volume(mpd.conn, uint_buf);
            break;
        case MPD_API_SET_SEEK:
            if(sscanf(wm->data.buf, "MPD_API_SET_SEEK,%u,%u", &uint_buf, &uint_buf_2))
                mpd_run_seek_id(mpd.conn, uint_buf, uint_buf_2);
            break;
        case MPD_API_GET_QUEUE:
            if(sscanf(wm->data.buf, "MPD_API_GET_QUEUE,%u", &uint_buf))
                n = mpd_put_queue(mpd.buf, uint_buf);
            break;
        case MPD_API_GET_BROWSE:
            p_charbuf = strdup(wm->data.buf);
            if(strcmp(strtok(p_charbuf, ","), "MPD_API_GET_BROWSE"))
                goto out_browse;

            uint_buf = strtoul(strtok(NULL, ","), NULL, 10);
            if((token = strtok(NULL, ",")) == NULL)
                goto out_browse;

			free(p_charbuf);
            p_charbuf = strdup(wm->data.buf);
            n = mpd_put_browse(mpd.buf, get_arg2(p_charbuf), uint_buf);
out_browse:
			free(p_charbuf);
            break;
        case MPD_API_ADD_TRACK:
            p_charbuf = strdup(wm->data.buf);
            if(strcmp(strtok(p_charbuf, ","), "MPD_API_ADD_TRACK"))
                goto out_add_track;

            if((token = strtok(NULL, ",")) == NULL)
                goto out_add_track;

			free(p_charbuf);
            p_charbuf = strdup(wm->data.buf);
            mpd_run_add(mpd.conn, get_arg1(p_charbuf));
out_add_track:
            free(p_charbuf);
            break;           
        case MPD_API_ADD_PLAY_TRACK:
            p_charbuf = strdup(wm->data.buf);
            if(strcmp(strtok(p_charbuf, ","), "MPD_API_ADD_PLAY_TRACK"))
                goto out_play_track;

            if((token = strtok(NULL, ",")) == NULL)
                goto out_play_track;

			free(p_charbuf);
            p_charbuf = strdup(wm->data.buf);
            int_buf = mpd_run_add_id(mpd.conn, get_arg1(p_charbuf));
            if(int_buf != -1)
                mpd_run_play_id(mpd.conn, int_buf);
out_play_track:
            free(p_charbuf);
            break;
        case MPD_API_ADD_PLAYLIST:
            p_charbuf = strdup(wm->data.buf);
            if(strcmp(strtok(p_charbuf, ","), "MPD_API_ADD_PLAYLIST"))
                goto out_playlist;

            if((token = strtok(NULL, ",")) == NULL)
                goto out_playlist;

			free(p_charbuf);
            p_charbuf = strdup(wm->data.buf);
            mpd_run_load(mpd.conn, get_arg1(p_charbuf));
out_playlist:
            free(p_charbuf);
            break;
        case MPD_API_SAVE_QUEUE:
            p_charbuf = strdup(wm->data.buf);
            if(strcmp(strtok(p_charbuf, ","), "MPD_API_SAVE_QUEUE"))
                goto out_save_queue;

            if((token = strtok(NULL, ",")) == NULL)
                goto out_save_queue;

			free(p_charbuf);
            p_charbuf = strdup(wm->data.buf);
            mpd_run_save(mpd.conn, get_arg1(p_charbuf));
out_save_queue:
            free(p_charbuf);
            break;
        case MPD_API_SEARCH:
            p_charbuf = strdup(wm->data.buf);
            if(strcmp(strtok(p_charbuf, ","), "MPD_API_SEARCH"))
				goto out_search;

            if((token = strtok(NULL, ",")) == NULL)
                goto out_search;

			free(p_charbuf);
            p_charbuf = strdup(wm->data.buf);
            n = mpd_search(mpd.buf, get_arg1(p_charbuf));
out_search:
            free(p_charbuf);
            break;
        case MPD_API_SEND_MESSAGE:
            p_charbuf = strdup(wm->data.buf);
            if(strcmp(strtok(p_charbuf, ","), "MPD_API_SEND_MESSAGE"))
				goto out_send_message;

            if((token = strtok(NULL, ",")) == NULL)
                goto out_send_message;

			free(p_charbuf);
            p_charbuf = strdup(get_arg1(wm->data.buf));

            if ( strtok(p_charbuf, ",") == NULL )
                goto out_send_message;

            if ( (token = strtok(NULL, ",")) == NULL )
                goto out_send_message;

			mpd_run_send_message(mpd.conn, p_charbuf, token);
out_send_message:
            free(p_charbuf);
            break;
#ifdef WITH_MPD_HOST_CHANGE
        /* Commands allowed when disconnected from MPD server */
        case MPD_API_SET_MPDHOST:
            int_buf = 0;
            p_charbuf = strdup(wm->data.buf);
            if(strcmp(strtok(p_charbuf, ","), "MPD_API_SET_MPDHOST"))
                goto out_host_change;

            if((int_buf = strtol(strtok(NULL, ","), NULL, 10)) <= 0)
                goto out_host_change;

            if((token = strtok(NULL, ",")) == NULL)
                goto out_host_change;

            strncpy(mpd.host, token, sizeof(mpd.host));
            mpd.port = int_buf;
            mpd.conn_state = MPD_RECONNECT;
            free(p_charbuf);
out_host_change:
            free(p_charbuf);
            break;
        case MPD_API_GET_MPDHOST:
            n = snprintf(mpd.buf, MAX_SIZE, "{\"type\":\"mpdhost\", \"data\": "
                "{\"host\" : \"%s\", \"port\": \"%d\", \"passwort_set\": %s}"
                "}", mpd.host, mpd.port, mpd.password ? "true" : "false");
            break;
        case MPD_API_GET_DIRBLEAPITOKEN:
            n = snprintf(mpd.buf, MAX_SIZE, "{\"type\":\"dirbleapitoken\", \""
                "data\": \"%s\"}", dirble_api_token);
            break;
        case MPD_API_SET_MPDPASS:
            p_charbuf = strdup(wm->data.buf);
            if(strcmp(strtok(p_charbuf, ","), "MPD_API_SET_MPDPASS"))
                goto out_set_pass;

            if((token = strtok(NULL, ",")) == NULL)
                goto out_set_pass;

            if(mpd.password)
                free(mpd.password);

            mpd.password = strdup(token);
            mpd.conn_state = MPD_RECONNECT;
            free(p_charbuf);
out_set_pass:
            free(p_charbuf);
            break;
#endif
    }

    if(mpd.conn_state == MPD_CONNECTED && mpd_connection_get_error(mpd.conn) != MPD_ERROR_SUCCESS)
    {
        n = snprintf(mpd.buf, MAX_SIZE, "{\"type\":\"error\", \"data\": \"%s\"}", 
            mpd_connection_get_error_message(mpd.conn));

        /* Try to recover error */
        if (!mpd_connection_clear_error(mpd.conn))
            mpd.conn_state = MPD_FAILURE;
    }

    if(n > 0){
        //mg_ws_send(c, mpd.buf, n, WEBSOCKET_OP_TEXT);
        p = (struct thread_data *)c->fn_data;
        mg_wakeup(p->mgr, p->conn_id, mpd.buf, n);
    }
    return;
}

int mpd_close_handler(struct mg_connection *c)
{
    /* Cleanup session data */
    if(c->fn_data)
        free(c->fn_data);
    return 0;
}

static void mpd_notify_callback(struct thread_data *p) {
    size_t n;
    
    if(mpd.conn_state != MPD_CONNECTED) {
        n = snprintf(mpd.buf, MAX_SIZE, "{\"type\":\"disconnected\"}");
        mg_wakeup(p->mgr, p->conn_id, mpd.buf, n);  // Send to parent
    }
    else{
        mg_wakeup(p->mgr, p->conn_id, mpd.buf, mpd.buf_size);  //之前mpd命令的response
        if(p->mpd_cs.song_id != mpd.song_id) //song_change
        {
            n = mpd_put_current_song(mpd.buf);
            mg_wakeup(p->mgr, p->conn_id, mpd.buf, n);
            p->mpd_cs.song_id = mpd.song_id;
        }
        if(p->mpd_cs.queue_version != mpd.queue_version) //update_queue
        {
            n = snprintf(mpd.buf, MAX_SIZE, "{\"type\":\"update_queue\"}");
            mg_wakeup(p->mgr, p->conn_id, mpd.buf, n);
            p->mpd_cs.queue_version = mpd.queue_version;
        }        
    }
}

void mpd_poll(struct thread_data *p)
{
    const char * buf;
    switch (mpd.conn_state) {
        case MPD_DISCONNECTED:
            /* Try to connect */
            fprintf(stdout, "MPD Connecting to %s:%d\n", mpd.host, mpd.port);
            mpd.conn = mpd_connection_new(mpd.host, mpd.port, 3000);
            if (mpd.conn == NULL) {
                fprintf(stderr, "Out of memory.");
                mpd.conn_state = MPD_FAILURE;
                return;
            }

            if (mpd_connection_get_error(mpd.conn) != MPD_ERROR_SUCCESS) {
                fprintf(stderr, "MPD connection: %s\n", mpd_connection_get_error_message(mpd.conn));
                buf = mpd_connection_get_error_message(mpd.conn);
                mg_ws_send_error(p,buf);
                mpd.conn_state = MPD_FAILURE;
                return;
            }

            if(mpd.password && !mpd_run_password(mpd.conn, mpd.password))
            {
                fprintf(stderr, "MPD connection: %s\n", mpd_connection_get_error_message(mpd.conn));
                buf = mpd_connection_get_error_message(mpd.conn);
                mg_ws_send_error(p,buf);
                mpd.conn_state = MPD_FAILURE;
                return;
            }

            fprintf(stderr, "MPD connected.\n");
            mpd_connection_set_timeout(mpd.conn, 10000);
            mpd.conn_state = MPD_CONNECTED;
            /* write outputs */
            mpd.buf_size = mpd_put_outputs(mpd.buf, 1);
            mpd_notify_callback(p);
            break;

        case MPD_FAILURE:
            fprintf(stderr, "MPD connection failed.\n");

        case MPD_DISCONNECT:
        case MPD_RECONNECT:
            if(mpd.conn != NULL)
                mpd_connection_free(mpd.conn);
            mpd.conn = NULL;
            mpd.conn_state = MPD_DISCONNECTED;
            break;

        case MPD_CONNECTED:
            mpd.buf_size = mpd_put_state(mpd.buf, &mpd.song_id, &mpd.queue_version);
            mpd_notify_callback(p);
            mpd.buf_size = mpd_put_outputs(mpd.buf, 0);
            mpd_notify_callback(p);
            break;
    }
}

char* mpd_get_title(struct mpd_song const *song)
{
    char *str;

    str = (char *)mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
    if(str == NULL){
        str = basename((char *)mpd_song_get_uri(song));
    }

    return str;
}

char* mpd_get_artist(struct mpd_song const *song)
{
    char *str;

    str = (char *)mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
    if (str == NULL) {
	return "";
    } else {
	return str;
    }
}

char* mpd_get_album(struct mpd_song const *song)
{
    char *str;

    str = (char *)mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
    if (str == NULL) {
	return "";
    } else {
	return str;
    }
}

int mpd_put_state(char *buffer, int *current_song_id, unsigned *queue_version)
{
    struct mpd_status *status;
    int len;

    status = mpd_run_status(mpd.conn);
    if (!status) {
        fprintf(stderr, "MPD mpd_run_status: %s\n", mpd_connection_get_error_message(mpd.conn));
        mpd.conn_state = MPD_FAILURE;
        return 0;
    }

    len = snprintf(buffer, MAX_SIZE,
        "{\"type\":\"state\", \"data\":{"
        " \"state\":%d, \"volume\":%d, \"repeat\":%d,"
        " \"single\":%d, \"crossfade\":%d, \"consume\":%d, \"random\":%d, "
        " \"songpos\": %d, \"elapsedTime\": %d, \"totalTime\":%d, "
        " \"currentsongid\": %d,\"queueLength\": %d,\"queueVersion\": %d"
        "}}", 
        mpd_status_get_state(status),
        mpd_status_get_volume(status), 
        mpd_status_get_repeat(status),
        mpd_status_get_single(status),
        mpd_status_get_crossfade(status),
        mpd_status_get_consume(status),
        mpd_status_get_random(status),
        mpd_status_get_song_pos(status),
        mpd_status_get_elapsed_time(status),
        mpd_status_get_total_time(status),
        mpd_status_get_song_id(status),
        mpd_status_get_queue_length(status),
        mpd_status_get_queue_version(status));

    *current_song_id = mpd_status_get_song_id(status);
    *queue_version = mpd_status_get_queue_version(status);
    mpd_status_free(status);
    return len;
}

int mpd_put_outputs(char *buffer, int names)
{
    struct mpd_output *out;
    int nout;
    char *str, *strend;

    str = buffer;
    strend = buffer+MAX_SIZE;
    str += snprintf(str, strend-str, "{\"type\":\"%s\", \"data\":{",
            names ? "outputnames" : "outputs");

    mpd_send_outputs(mpd.conn);
    nout = 0;
    while ((out = mpd_recv_output(mpd.conn)) != NULL) {
        if (nout++)
            *str++ = ',';
        if (names)
            str += snprintf(str, strend - str, " \"%d\":\"%s\"",
                    mpd_output_get_id(out), mpd_output_get_name(out));
        else
            str += snprintf(str, strend-str, " \"%d\":%d",
                    mpd_output_get_id(out), mpd_output_get_enabled(out));
        mpd_output_free(out);
    }
    if (!mpd_response_finish(mpd.conn)) {
        fprintf(stderr, "MPD outputs: %s\n", mpd_connection_get_error_message(mpd.conn));
        mpd_connection_clear_error(mpd.conn);
        return 0;
    }
    str += snprintf(str, strend-str, " }}");
    return str-buffer;
}

int mpd_put_current_song(char *buffer)
{
    char *cur = buffer;
    const char *end = buffer + MAX_SIZE;
    struct mpd_song *song;

    song = mpd_run_current_song(mpd.conn);
    if(song == NULL)
        return 0;

    cur += json_emit_raw_str(cur, end - cur, "{\"type\": \"song_change\", \"data\":{\"pos\":");
    cur += json_emit_int(cur, end - cur, mpd_song_get_pos(song));
    cur += json_emit_raw_str(cur, end - cur, ",\"title\":");
    cur += json_emit_quoted_str(cur, end - cur, mpd_get_title(song));
    cur += json_emit_raw_str(cur, end - cur, ",\"artist\":");
    cur += json_emit_quoted_str(cur, end - cur, mpd_get_artist(song));
    cur += json_emit_raw_str(cur, end - cur, ",\"album\":");
    cur += json_emit_quoted_str(cur, end - cur, mpd_get_album(song));

    cur += json_emit_raw_str(cur, end - cur, "}}");
    mpd_song_free(song);
    mpd_response_finish(mpd.conn);

    return cur - buffer;
}

int mpd_put_queue(char *buffer, unsigned int offset)
{
    char *cur = buffer;
    const char *end = buffer + MAX_SIZE;
    struct mpd_entity *entity;

    if (!mpd_send_list_queue_range_meta(mpd.conn, offset, offset+MAX_ELEMENTS_PER_PAGE))
        RETURN_ERROR_AND_RECOVER("mpd_send_list_queue_meta");

    cur += json_emit_raw_str(cur, end  - cur, "{\"type\":\"queue\",\"data\":[ ");

    while((entity = mpd_recv_entity(mpd.conn)) != NULL) {
        const struct mpd_song *song;

        if(mpd_entity_get_type(entity) == MPD_ENTITY_TYPE_SONG) {
            song = mpd_entity_get_song(entity);

            cur += json_emit_raw_str(cur, end - cur, "{\"id\":");
            cur += json_emit_int(cur, end - cur, mpd_song_get_id(song));
            cur += json_emit_raw_str(cur, end - cur, ",\"pos\":");
            cur += json_emit_int(cur, end - cur, mpd_song_get_pos(song));
            cur += json_emit_raw_str(cur, end - cur, ",\"duration\":");
            cur += json_emit_int(cur, end - cur, mpd_song_get_duration(song));
            cur += json_emit_raw_str(cur, end - cur, ",\"title\":");
            cur += json_emit_quoted_str(cur, end - cur, mpd_get_title(song));
            cur += json_emit_raw_str(cur, end - cur, ",\"artist\":");
            cur += json_emit_quoted_str(cur, end - cur, mpd_get_artist(song));
            cur += json_emit_raw_str(cur, end - cur, ",\"album\":");
            cur += json_emit_quoted_str(cur, end - cur, mpd_get_album(song));
            cur += json_emit_raw_str(cur, end - cur, "},");
        }
        mpd_entity_free(entity);
    }

    /* remove last ',' */
    cur--;

    cur += json_emit_raw_str(cur, end - cur, "]}");
    return cur - buffer;
}

int mpd_put_browse(char *buffer, char *path, unsigned int offset)
{
    char *cur = buffer;
    const char *end = buffer + MAX_SIZE;
    struct mpd_entity *entity;
    unsigned int entity_count = 0;

    if (!mpd_send_list_meta(mpd.conn, path))
        RETURN_ERROR_AND_RECOVER("mpd_send_list_meta");

    cur += json_emit_raw_str(cur, end  - cur, "{\"type\":\"browse\",\"data\":[ ");

    while((entity = mpd_recv_entity(mpd.conn)) != NULL) {
        const struct mpd_song *song;
        const struct mpd_directory *dir;
        const struct mpd_playlist *pl;

        if(offset > entity_count)
        {
            mpd_entity_free(entity);
            entity_count++;
            continue;
        }
        else if(offset + MAX_ELEMENTS_PER_PAGE - 1 < entity_count)
        {
            mpd_entity_free(entity);
            cur += json_emit_raw_str(cur, end  - cur, "{\"type\":\"wrap\",\"count\":");
            cur += json_emit_int(cur, end - cur, entity_count);
            cur += json_emit_raw_str(cur, end  - cur, "} ");
            break;
        }

        switch (mpd_entity_get_type(entity)) {
            case MPD_ENTITY_TYPE_UNKNOWN:
                break;

            case MPD_ENTITY_TYPE_SONG:
                song = mpd_entity_get_song(entity);
                cur += json_emit_raw_str(cur, end - cur, "{\"type\":\"song\",\"uri\":");
                cur += json_emit_quoted_str(cur, end - cur, mpd_song_get_uri(song));
                cur += json_emit_raw_str(cur, end - cur, ",\"duration\":");
                cur += json_emit_int(cur, end - cur, mpd_song_get_duration(song));
                cur += json_emit_raw_str(cur, end - cur, ",\"title\":");
                cur += json_emit_quoted_str(cur, end - cur, mpd_get_title(song));
                cur += json_emit_raw_str(cur, end - cur, "},");
                break;

            case MPD_ENTITY_TYPE_DIRECTORY:
                dir = mpd_entity_get_directory(entity);

                cur += json_emit_raw_str(cur, end - cur, "{\"type\":\"directory\",\"dir\":");
                cur += json_emit_quoted_str(cur, end - cur, mpd_directory_get_path(dir));
                cur += json_emit_raw_str(cur, end - cur, "},");
                break;

            case MPD_ENTITY_TYPE_PLAYLIST:
                pl = mpd_entity_get_playlist(entity);
                cur += json_emit_raw_str(cur, end - cur, "{\"type\":\"playlist\",\"plist\":");
                cur += json_emit_quoted_str(cur, end - cur, mpd_playlist_get_path(pl));
                cur += json_emit_raw_str(cur, end - cur, "},");
                break;
        }
        mpd_entity_free(entity);
        entity_count++;
    }

    if (mpd_connection_get_error(mpd.conn) != MPD_ERROR_SUCCESS || !mpd_response_finish(mpd.conn)) {
        fprintf(stderr, "MPD mpd_send_list_meta: %s\n", mpd_connection_get_error_message(mpd.conn));
        mpd.conn_state = MPD_FAILURE;
        return 0;
    }

    /* remove last ',' */
    cur--;

    cur += json_emit_raw_str(cur, end - cur, "]}");
    return cur - buffer;
}

int mpd_search(char *buffer, char *searchstr)
{
    int i = 0;
    char *cur = buffer;
    const char *end = buffer + MAX_SIZE;
    struct mpd_song *song;

    if(mpd_search_db_songs(mpd.conn, false) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_db_songs");
    else if(mpd_search_add_any_tag_constraint(mpd.conn, MPD_OPERATOR_DEFAULT, searchstr) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_add_any_tag_constraint");
    else if(mpd_search_commit(mpd.conn) == false)
        RETURN_ERROR_AND_RECOVER("mpd_search_commit");
    else {
        cur += json_emit_raw_str(cur, end - cur, "{\"type\":\"search\",\"data\":[ ");

        while((song = mpd_recv_song(mpd.conn)) != NULL) {
            cur += json_emit_raw_str(cur, end - cur, "{\"type\":\"song\",\"uri\":");
            cur += json_emit_quoted_str(cur, end - cur, mpd_song_get_uri(song));
            cur += json_emit_raw_str(cur, end - cur, ",\"duration\":");
            cur += json_emit_int(cur, end - cur, mpd_song_get_duration(song));
            cur += json_emit_raw_str(cur, end - cur, ",\"title\":");
            cur += json_emit_quoted_str(cur, end - cur, mpd_get_title(song));
            cur += json_emit_raw_str(cur, end - cur, ",\"artist\":");
            cur += json_emit_quoted_str(cur, end - cur, mpd_get_artist(song));
            cur += json_emit_raw_str(cur, end - cur, ",\"album\":");
            cur += json_emit_quoted_str(cur, end - cur, mpd_get_album(song));
            cur += json_emit_raw_str(cur, end - cur, "},");
            mpd_song_free(song);

            /* Maximum results */
            if(i++ >= 300)
            {
                cur += json_emit_raw_str(cur, end - cur, "{\"type\":\"wrap\"},");
                break;
            }
        }

        /* remove last ',' */
        cur--;

        cur += json_emit_raw_str(cur, end - cur, "]}");
    }
    return cur - buffer;
}


void mpd_disconnect()
{
    mpd.conn_state = MPD_DISCONNECT;
    mpd_poll(NULL);
}

void mpd_clear_all()
{
    if (mpd.conn != NULL) {
        // 标记响应接收结束
        //mpd_response_finish(mpd.conn);
        // 停止播放
        if (!mpd_run_stop(mpd.conn)) {
            fprintf(stderr, "Failed to stop playback: %s\n", mpd_connection_get_error_message(mpd.conn));
        }
        // 清空播放队列
        if (!mpd_run_clear(mpd.conn)) {
            fprintf(stderr, "Failed to clear playlist: %s\n", mpd_connection_get_error_message(mpd.conn));
        }
        // 刷新数据库
        if (!mpd_run_update(mpd.conn, NULL)) {
            fprintf(stderr, "Failed to update database: %s\n", mpd_connection_get_error_message(mpd.conn));
        }
        // 清除错误信息
        if (!mpd_run_clearerror(mpd.conn)) {
            fprintf(stderr, "Failed to clear error: %s\n", mpd_connection_get_error_message(mpd.conn));
        }
        mpd_disconnect();
    }else
        fprintf(stderr, "mpd.conn is NULL\n");

}
